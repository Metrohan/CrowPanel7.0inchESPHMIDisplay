#!/usr/bin/env python3
"""test_serial_loopback.py

Serial bağlantı testi - ESP32 ile haberleşmeyi doğrular.
ESP32'nin açık ve bağlı olması gerekir.

Bu script:
1. ESP32'ye test paketi gönderir
2. ESP32'den gelen yanıtları dinler
3. Protokol uyumluluğunu doğrular
"""

import serial
import serial.tools.list_ports
import time
import sys

MAGIC = b"\xC0\xDE\xCA\xFE"
VERSION = 1
TYPE_PREVIEW_JPEG = 0x01
TYPE_CMD_CAPTURE = 0x10
TYPE_ACK_CAPTURE = 0x11

BAUD_RATE = 2000000

def u32le(v: int) -> bytes:
    return int(v & 0xFFFFFFFF).to_bytes(4, "little")

def find_esp32_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if ("USB" in port.description or 
            "ACM" in port.device or 
            "ESP" in port.description or
            port.vid == 0x303A):
            return port.device
    return "/dev/ttyUSB0"

def create_test_jpeg():
    """Create a minimal valid JPEG for testing."""
    # This is a minimal 1x1 red pixel JPEG
    # We'll use a slightly larger test pattern for 8x8
    import cv2
    import numpy as np
    
    # Create 8x8 test pattern
    img = np.zeros((8, 8, 3), dtype=np.uint8)
    img[:, :, 2] = 255  # Red channel
    
    # Encode as JPEG
    result, jpeg_array = cv2.imencode(".jpg", img, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
    if result:
        return jpeg_array.tobytes()
    return None

def create_480x320_test_jpeg():
    """Create a 480x320 test pattern JPEG."""
    import cv2
    import numpy as np
    
    # Create test pattern with color bars
    img = np.zeros((320, 480, 3), dtype=np.uint8)
    
    # Vertical color bars
    colors = [
        (255, 255, 255),  # White
        (255, 255, 0),    # Yellow
        (0, 255, 255),    # Cyan
        (0, 255, 0),      # Green
        (255, 0, 255),    # Magenta
        (255, 0, 0),      # Red
        (0, 0, 255),      # Blue
        (0, 0, 0),        # Black
    ]
    
    bar_width = 480 // len(colors)
    for i, color in enumerate(colors):
        x_start = i * bar_width
        x_end = (i + 1) * bar_width if i < len(colors) - 1 else 480
        img[:, x_start:x_end] = color
    
    # Add text
    cv2.putText(img, "ESP32 TEST", (150, 160), cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 0, 0), 3)
    cv2.putText(img, "ESP32 TEST", (150, 160), cv2.FONT_HERSHEY_SIMPLEX, 1.5, (255, 255, 255), 2)
    
    # Encode as JPEG
    result, jpeg_array = cv2.imencode(".jpg", img, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
    if result:
        return jpeg_array.tobytes()
    return None

def send_packet(ser, ptype, seq, payload):
    header = MAGIC + bytes([VERSION, ptype]) + u32le(seq) + u32le(len(payload))
    ser.write(header)
    if payload:
        ser.write(payload)
    ser.flush()
    return len(header) + len(payload)

def read_responses(ser, timeout=1.0):
    """Read and display any responses from ESP32."""
    end_time = time.time() + timeout
    responses = []
    
    while time.time() < end_time:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            responses.append(data)
            try:
                text = data.decode('utf-8', errors='replace')
                # Print each line separately
                for line in text.split('\n'):
                    line = line.strip()
                    if line:
                        print(f"  [ESP32] {line}")
            except:
                print(f"  [ESP32 binary] {data[:64].hex()}")
        else:
            time.sleep(0.01)
    
    return b''.join(responses)

def main():
    print("=" * 60)
    print("Serial Loopback / Protocol Test")
    print("=" * 60)
    
    port = find_esp32_port()
    print(f"\n[INFO] Using port: {port}")
    print(f"[INFO] Baud rate: {BAUD_RATE}")
    
    try:
        ser = serial.Serial(
            port=port,
            baudrate=BAUD_RATE,
            timeout=2,
            write_timeout=2,
        )
        print("[OK] Serial port opened")
    except Exception as e:
        print(f"[FAIL] Cannot open serial port: {e}")
        sys.exit(1)
    
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    time.sleep(0.5)
    
    # Read initial ESP32 output
    print("\n--- Initial ESP32 output ---")
    read_responses(ser, timeout=1.0)
    
    # TEST 1: Send small test JPEG
    print("\n--- TEST 1: Send 8x8 test JPEG ---")
    try:
        jpeg_small = create_test_jpeg()
        if jpeg_small:
            print(f"[INFO] Small JPEG size: {len(jpeg_small)} bytes")
            print(f"[INFO] First bytes: {jpeg_small[:8].hex().upper()}")
            
            sent = send_packet(ser, TYPE_PREVIEW_JPEG, 1, jpeg_small)
            print(f"[INFO] Sent {sent} bytes (header + payload)")
            
            print("[INFO] Waiting for ESP32 response...")
            read_responses(ser, timeout=2.0)
        else:
            print("[WARN] Could not create test JPEG (OpenCV not available?)")
    except Exception as e:
        print(f"[ERROR] Test 1 failed: {e}")
    
    # TEST 2: Send 480x320 test JPEG
    print("\n--- TEST 2: Send 480x320 test JPEG ---")
    try:
        jpeg_full = create_480x320_test_jpeg()
        if jpeg_full:
            print(f"[INFO] Full JPEG size: {len(jpeg_full)} bytes")
            print(f"[INFO] First bytes: {jpeg_full[:8].hex().upper()}")
            
            sent = send_packet(ser, TYPE_PREVIEW_JPEG, 2, jpeg_full)
            print(f"[INFO] Sent {sent} bytes (header + payload)")
            
            print("[INFO] Waiting for ESP32 response...")
            read_responses(ser, timeout=2.0)
        else:
            print("[WARN] Could not create 480x320 JPEG")
    except Exception as e:
        print(f"[ERROR] Test 2 failed: {e}")
    
    # TEST 3: Send multiple frames in succession
    print("\n--- TEST 3: Send 5 frames rapidly ---")
    try:
        jpeg_full = create_480x320_test_jpeg()
        if jpeg_full:
            for seq in range(3, 8):
                sent = send_packet(ser, TYPE_PREVIEW_JPEG, seq, jpeg_full)
                print(f"[INFO] Sent frame {seq}")
                time.sleep(0.15)  # ~6 FPS
            
            print("[INFO] Waiting for ESP32 stats...")
            read_responses(ser, timeout=3.0)
    except Exception as e:
        print(f"[ERROR] Test 3 failed: {e}")
    
    # TEST 4: Send capture command
    print("\n--- TEST 4: Send capture command ---")
    try:
        base_seq = 7  # Capture after seq 7
        sent = send_packet(ser, TYPE_CMD_CAPTURE, base_seq, u32le(base_seq))
        print(f"[INFO] Sent CMD_CAPTURE (base_seq={base_seq})")
        
        print("[INFO] Waiting for ESP32 response...")
        read_responses(ser, timeout=2.0)
    except Exception as e:
        print(f"[ERROR] Test 4 failed: {e}")
    
    # TEST 5: Stress test - continuous stream for 5 seconds
    print("\n--- TEST 5: 5-second stream test ---")
    try:
        jpeg_full = create_480x320_test_jpeg()
        if jpeg_full:
            start_time = time.time()
            seq = 10
            frames_sent = 0
            
            while time.time() - start_time < 5.0:
                sent = send_packet(ser, TYPE_PREVIEW_JPEG, seq, jpeg_full)
                seq += 1
                frames_sent += 1
                
                # Target 8 FPS
                elapsed = time.time() - start_time
                expected_time = frames_sent / 8.0
                if elapsed < expected_time:
                    time.sleep(expected_time - elapsed)
            
            elapsed = time.time() - start_time
            actual_fps = frames_sent / elapsed
            print(f"[INFO] Sent {frames_sent} frames in {elapsed:.1f}s = {actual_fps:.1f} FPS")
            
            print("[INFO] Final ESP32 stats:")
            read_responses(ser, timeout=2.0)
    except Exception as e:
        print(f"[ERROR] Test 5 failed: {e}")
    
    print("\n" + "=" * 60)
    print("Test complete!")
    print("=" * 60)
    print("\nCheck the ESP32 serial output for:")
    print("  - [STAT] lines showing rx, shown, drop_busy counts")
    print("  - [ERROR] lines if JPEG decoding failed")
    print("  - [WARN] lines if frame dimensions were wrong")
    print("  - [DBG] lines showing frame processing state")
    
    ser.close()

if __name__ == "__main__":
    main()
