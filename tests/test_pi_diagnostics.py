#!/usr/bin/env python3
"""test_pi_diagnostics.py

Kapsamlı tanılama testi - Pi tarafındaki tüm sorunları tespit eder.
Bu script'i Pi'da çalıştırın: python3 test_pi_diagnostics.py

Tests:
1. Camera initialization and dual-stream configuration
2. YUV420 to BGR conversion correctness
3. JPEG encoding with proper headers (FFD8...FFD9)
4. Serial port detection and communication
5. Framed protocol packet generation
6. Frame timing and throughput analysis
"""

import sys
import os
import time
import io
from datetime import datetime

# Test sonuçları
test_results = []

def log_test(name: str, passed: bool, details: str = ""):
    status = "✅ PASS" if passed else "❌ FAIL"
    test_results.append((name, passed, details))
    print(f"{status}: {name}")
    if details:
        print(f"       {details}")

def log_info(msg: str):
    print(f"ℹ️  {msg}")

def log_warn(msg: str):
    print(f"⚠️  {msg}")

print("=" * 60)
print("Pi Diagnostic Test Suite")
print("=" * 60)
print()

# ============================================================
# TEST 1: Python version and dependencies
# ============================================================
print("\n--- TEST 1: Python & Dependencies ---")

py_version = sys.version_info
log_test("Python version >= 3.7", 
         py_version >= (3, 7), 
         f"Python {py_version.major}.{py_version.minor}.{py_version.micro}")

try:
    import numpy as np
    log_test("NumPy import", True, f"Version: {np.__version__}")
except ImportError as e:
    log_test("NumPy import", False, str(e))
    print("   Install with: pip3 install numpy")

try:
    import cv2
    log_test("OpenCV import", True, f"Version: {cv2.__version__}")
except ImportError as e:
    log_test("OpenCV import", False, str(e))
    print("   Install with: pip3 install opencv-python")

try:
    import serial
    log_test("PySerial import", True, f"Version: {serial.__version__}")
except ImportError as e:
    log_test("PySerial import", False, str(e))
    print("   Install with: pip3 install pyserial")

try:
    from picamera2 import Picamera2
    log_test("Picamera2 import", True)
except ImportError as e:
    log_test("Picamera2 import", False, str(e))
    print("   Install with: pip3 install picamera2")

# ============================================================
# TEST 2: Camera Detection and Initialization
# ============================================================
print("\n--- TEST 2: Camera Detection ---")

picam2 = None
try:
    from picamera2 import Picamera2
    
    # List available cameras
    try:
        cameras = Picamera2.global_camera_info()
        log_test("Camera enumeration", True, f"Found {len(cameras)} camera(s)")
        for i, cam in enumerate(cameras):
            log_info(f"  Camera {i}: {cam}")
    except Exception as e:
        log_test("Camera enumeration", False, str(e))
    
    # Try to create Picamera2 instance
    picam2 = Picamera2()
    log_test("Picamera2 instance creation", True)
    
    # Check sensor modes
    try:
        sensor_modes = picam2.sensor_modes
        log_test("Sensor modes available", len(sensor_modes) > 0, f"{len(sensor_modes)} modes")
        for i, mode in enumerate(sensor_modes[:3]):  # Show first 3
            log_info(f"  Mode {i}: {mode}")
    except Exception as e:
        log_test("Sensor modes available", False, str(e))
        
except Exception as e:
    log_test("Picamera2 initialization", False, str(e))
    picam2 = None

# ============================================================
# TEST 3: Dual-Stream Configuration (Main 4K + Lores 480x320)
# ============================================================
print("\n--- TEST 3: Dual-Stream Configuration ---")

RES_WIDTH = 480
RES_HEIGHT = 320
dual_stream_ok = False

if picam2:
    try:
        # Configure dual stream: main=4K, lores=480x320 (both YUV420)
        config = picam2.create_video_configuration(
            main={"size": (3840, 2160), "format": "YUV420"},
            lores={"size": (RES_WIDTH, RES_HEIGHT), "format": "YUV420"},
        )
        picam2.configure(config)
        log_test("Dual-stream config (main=4K, lores=480x320)", True)
        dual_stream_ok = True
        
        # Log configuration details
        log_info(f"  Main stream: {config['main']}")
        log_info(f"  Lores stream: {config['lores']}")
        
    except Exception as e:
        log_test("Dual-stream config", False, str(e))
        
        # Try fallback to single stream
        try:
            config = picam2.create_video_configuration(
                main={"size": (RES_WIDTH, RES_HEIGHT), "format": "YUV420"},
            )
            picam2.configure(config)
            log_warn("Fallback to single 480x320 stream succeeded")
        except Exception as e2:
            log_test("Fallback single-stream config", False, str(e2))

# ============================================================
# TEST 4: Camera Start and Frame Capture
# ============================================================
print("\n--- TEST 4: Camera Start & Frame Capture ---")

frame_lores = None
frame_main = None

if picam2:
    try:
        picam2.start()
        time.sleep(1)  # Warm-up
        log_test("Camera start", True)
        
        # Capture a single request
        request = picam2.capture_request()
        try:
            # Get lores frame
            frame_lores = request.make_array("lores")
            if frame_lores is not None:
                log_test("Lores frame capture", True, 
                        f"Shape: {frame_lores.shape}, dtype: {frame_lores.dtype}")
            else:
                log_test("Lores frame capture", False, "make_array returned None")
            
            # Get main frame (if dual stream)
            if dual_stream_ok:
                frame_main = request.make_array("main")
                if frame_main is not None:
                    log_test("Main (4K) frame capture", True,
                            f"Shape: {frame_main.shape}, dtype: {frame_main.dtype}")
                else:
                    log_test("Main (4K) frame capture", False, "make_array returned None")
        finally:
            request.release()
            
    except Exception as e:
        log_test("Camera start / capture", False, str(e))

# ============================================================
# TEST 5: YUV420 to BGR Conversion
# ============================================================
print("\n--- TEST 5: YUV420 to BGR Conversion ---")

frame_bgr = None

if frame_lores is not None:
    try:
        import cv2
        import numpy as np
        
        log_info(f"Input frame shape: {frame_lores.shape}, ndim: {frame_lores.ndim}")
        
        if frame_lores.ndim == 2:
            # Planar YUV420 (I420) - height is 1.5x actual
            h_buf, w_buf = frame_lores.shape
            actual_h = int(h_buf * 2 / 3)
            log_info(f"YUV420 planar detected: buffer={w_buf}x{h_buf}, actual={w_buf}x{actual_h}")
            
            frame_bgr = cv2.cvtColor(frame_lores, cv2.COLOR_YUV2BGR_I420)
            log_test("YUV420 to BGR conversion", True, f"Output shape: {frame_bgr.shape}")
            
        elif frame_lores.ndim == 3:
            if frame_lores.shape[2] == 3:
                frame_bgr = cv2.cvtColor(frame_lores, cv2.COLOR_RGB2BGR)
                log_test("RGB to BGR conversion", True, f"Output shape: {frame_bgr.shape}")
            else:
                log_test("Unknown 3D format", False, f"Channels: {frame_lores.shape[2]}")
        else:
            log_test("Frame format detection", False, f"Unknown ndim: {frame_lores.ndim}")
        
        # Check output dimensions
        if frame_bgr is not None:
            out_h, out_w = frame_bgr.shape[:2]
            if (out_w, out_h) == (RES_WIDTH, RES_HEIGHT):
                log_test("Output dimensions match 480x320", True)
            else:
                log_warn(f"Output {out_w}x{out_h} != expected {RES_WIDTH}x{RES_HEIGHT}")
                # Resize to expected
                frame_bgr = cv2.resize(frame_bgr, (RES_WIDTH, RES_HEIGHT))
                log_test("Resize to 480x320", True, f"Final shape: {frame_bgr.shape}")
                
    except Exception as e:
        log_test("YUV420 conversion", False, str(e))
        import traceback
        traceback.print_exc()

# ============================================================
# TEST 6: JPEG Encoding (Header Validation)
# ============================================================
print("\n--- TEST 6: JPEG Encoding ---")

jpeg_bytes = None

if frame_bgr is not None:
    try:
        import cv2
        
        # Encode at quality 70
        quality = 70
        result, jpeg_array = cv2.imencode(".jpg", frame_bgr, 
                                          [int(cv2.IMWRITE_JPEG_QUALITY), quality])
        
        if result:
            jpeg_bytes = jpeg_array.tobytes()
            log_test("JPEG encoding succeeded", True, f"Size: {len(jpeg_bytes)} bytes")
            
            # Validate JPEG header (FFD8) and footer (FFD9)
            if len(jpeg_bytes) >= 4:
                header = jpeg_bytes[:2]
                footer = jpeg_bytes[-2:]
                
                header_ok = (header[0] == 0xFF and header[1] == 0xD8)
                footer_ok = (footer[0] == 0xFF and footer[1] == 0xD9)
                
                log_test("JPEG header (FFD8)", header_ok, 
                        f"Got: {header.hex().upper()}")
                log_test("JPEG footer (FFD9)", footer_ok,
                        f"Got: {footer.hex().upper()}")
                
                # Also check SOF marker exists (Frame header)
                # SOF0=FFC0, SOF1=FFC1, SOF2=FFC2
                sof_found = any(
                    jpeg_bytes[i:i+2] in [b'\xFF\xC0', b'\xFF\xC1', b'\xFF\xC2']
                    for i in range(len(jpeg_bytes) - 1)
                )
                log_test("JPEG SOF marker present", sof_found)
                
                # Show first 16 bytes
                log_info(f"First 16 bytes: {jpeg_bytes[:16].hex().upper()}")
            else:
                log_test("JPEG size check", False, "Too small")
        else:
            log_test("JPEG encoding", False, "imencode returned False")
            
    except Exception as e:
        log_test("JPEG encoding", False, str(e))

# ============================================================
# TEST 7: Serial Port Detection
# ============================================================
print("\n--- TEST 7: Serial Port Detection ---")

try:
    import serial.tools.list_ports
    
    ports = serial.tools.list_ports.comports()
    log_test("Port enumeration", True, f"Found {len(ports)} port(s)")
    
    esp32_port = None
    for port in ports:
        is_esp = ("USB" in port.description or 
                  "ACM" in port.device or 
                  "ESP" in port.description or
                  port.vid == 0x303A)
        marker = " <-- ESP32?" if is_esp else ""
        log_info(f"  {port.device}: {port.description} (VID={port.vid}){marker}")
        if is_esp and esp32_port is None:
            esp32_port = port.device
    
    if esp32_port:
        log_test("ESP32 port auto-detected", True, esp32_port)
    else:
        log_test("ESP32 port auto-detected", False, "Not found, will use /dev/ttyUSB0")
        esp32_port = "/dev/ttyUSB0"
        
except Exception as e:
    log_test("Serial port detection", False, str(e))
    esp32_port = "/dev/ttyUSB0"

# ============================================================
# TEST 8: Serial Connection
# ============================================================
print("\n--- TEST 8: Serial Connection ---")

ser = None
BAUD_RATE = 2000000

if esp32_port:
    try:
        import serial
        
        ser = serial.Serial(
            port=esp32_port,
            baudrate=BAUD_RATE,
            timeout=2,
            write_timeout=2,
        )
        
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        log_test("Serial port open", True, f"{esp32_port} @ {BAUD_RATE} baud")
        
        # Wait briefly for ESP32 response
        time.sleep(0.5)
        
        # Check if ESP32 sends any data
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            try:
                text = data.decode('utf-8', errors='replace')
                log_info(f"ESP32 response: {text.strip()[:100]}")
            except:
                log_info(f"ESP32 binary response: {data[:32].hex()}")
                
    except serial.SerialException as e:
        log_test("Serial port open", False, str(e))
        log_warn("Make sure ESP32 is connected and no other program is using the port")
    except Exception as e:
        log_test("Serial connection", False, str(e))

# ============================================================
# TEST 9: Framed Protocol Packet Generation
# ============================================================
print("\n--- TEST 9: Framed Protocol Packet ---")

MAGIC = b"\xC0\xDE\xCA\xFE"
VERSION = 1
TYPE_PREVIEW_JPEG = 0x01

def u32le(v: int) -> bytes:
    return int(v & 0xFFFFFFFF).to_bytes(4, "little")

if jpeg_bytes:
    try:
        seq = 1
        payload = jpeg_bytes
        
        header = MAGIC + bytes([VERSION, TYPE_PREVIEW_JPEG]) + u32le(seq) + u32le(len(payload))
        packet = header + payload
        
        log_test("Packet header generation", True, f"Header size: {len(header)} bytes")
        log_info(f"  Magic: {header[:4].hex().upper()}")
        log_info(f"  Version: {header[4]}")
        log_info(f"  Type: {header[5]} (PREVIEW_JPEG)")
        log_info(f"  Seq: {int.from_bytes(header[6:10], 'little')}")
        log_info(f"  Len: {int.from_bytes(header[10:14], 'little')}")
        log_info(f"  Total packet: {len(packet)} bytes")
        
        # Verify packet structure
        expected_size = 14 + len(jpeg_bytes)
        log_test("Packet size correct", len(packet) == expected_size,
                f"Expected {expected_size}, got {len(packet)}")
        
    except Exception as e:
        log_test("Packet generation", False, str(e))

# ============================================================
# TEST 10: Send Test Packet (if serial connected)
# ============================================================
print("\n--- TEST 10: Send Test Packet ---")

if ser and jpeg_bytes:
    try:
        seq = 1
        header = MAGIC + bytes([VERSION, TYPE_PREVIEW_JPEG]) + u32le(seq) + u32le(len(jpeg_bytes))
        
        # Send header
        written_h = ser.write(header)
        ser.flush()
        
        # Send payload
        written_p = ser.write(jpeg_bytes)
        ser.flush()
        
        total_written = written_h + written_p
        expected = len(header) + len(jpeg_bytes)
        
        log_test("Packet send", total_written == expected,
                f"Sent {total_written}/{expected} bytes")
        
        # Wait a bit for ESP32 to process
        time.sleep(0.3)
        
        # Check for any response
        if ser.in_waiting > 0:
            resp = ser.read(min(ser.in_waiting, 256))
            log_info(f"ESP32 response after send: {resp[:64]}")
            
    except Exception as e:
        log_test("Packet send", False, str(e))
elif not ser:
    log_warn("Serial not connected, skipping send test")
elif not jpeg_bytes:
    log_warn("No JPEG data, skipping send test")

# ============================================================
# TEST 11: Throughput Estimation
# ============================================================
print("\n--- TEST 11: Throughput Analysis ---")

if jpeg_bytes:
    jpeg_size = len(jpeg_bytes)
    header_size = 14
    packet_size = header_size + jpeg_size
    
    # At 2Mbps, theoretical max
    bits_per_frame = packet_size * 8
    theoretical_fps = (2_000_000 / bits_per_frame)
    
    log_info(f"JPEG size: {jpeg_size:,} bytes")
    log_info(f"Packet size (with header): {packet_size:,} bytes")
    log_info(f"Bits per frame: {bits_per_frame:,}")
    log_info(f"Theoretical max FPS at 2Mbps: {theoretical_fps:.1f}")
    
    # Target 8 FPS
    required_bps = packet_size * 8 * 8
    log_info(f"Required bandwidth for 8 FPS: {required_bps:,} bps ({required_bps/1_000_000:.2f} Mbps)")
    
    fits_bandwidth = required_bps < 2_000_000
    log_test("8 FPS fits in 2Mbps bandwidth", fits_bandwidth,
            f"{'OK' if fits_bandwidth else 'EXCEEDS'}: {required_bps/1_000_000:.2f} Mbps")

# ============================================================
# TEST 12: Save Test JPEG
# ============================================================
print("\n--- TEST 12: Save Test Files ---")

if jpeg_bytes:
    try:
        test_dir = "test_output"
        os.makedirs(test_dir, exist_ok=True)
        
        # Save JPEG
        jpeg_path = os.path.join(test_dir, "test_frame.jpg")
        with open(jpeg_path, "wb") as f:
            f.write(jpeg_bytes)
        log_test("Save test JPEG", True, jpeg_path)
        
        # Save raw BGR frame as PNG for comparison
        if frame_bgr is not None:
            import cv2
            png_path = os.path.join(test_dir, "test_frame_bgr.png")
            cv2.imwrite(png_path, frame_bgr)
            log_test("Save BGR frame as PNG", True, png_path)
            
        # Save packet dump
        packet_path = os.path.join(test_dir, "test_packet.bin")
        packet = MAGIC + bytes([VERSION, TYPE_PREVIEW_JPEG]) + u32le(1) + u32le(len(jpeg_bytes)) + jpeg_bytes
        with open(packet_path, "wb") as f:
            f.write(packet)
        log_test("Save test packet", True, packet_path)
        
    except Exception as e:
        log_test("Save test files", False, str(e))

# ============================================================
# Cleanup
# ============================================================
print("\n--- Cleanup ---")

if ser:
    try:
        ser.close()
        log_info("Serial port closed")
    except:
        pass

if picam2:
    try:
        picam2.stop()
        log_info("Camera stopped")
    except:
        pass

# ============================================================
# Summary
# ============================================================
print("\n" + "=" * 60)
print("TEST SUMMARY")
print("=" * 60)

passed = sum(1 for _, p, _ in test_results if p)
failed = sum(1 for _, p, _ in test_results if not p)
total = len(test_results)

print(f"\nTotal: {total} tests")
print(f"Passed: {passed} ✅")
print(f"Failed: {failed} ❌")

if failed > 0:
    print("\n❌ FAILED TESTS:")
    for name, p, details in test_results:
        if not p:
            print(f"   - {name}: {details}")

print("\n" + "=" * 60)

if failed == 0:
    print("🎉 All tests passed! Pi side looks OK.")
    print("   If ESP32 still shows rx>0 but shown=0, the problem is on ESP32 side.")
else:
    print("⚠️  Some tests failed. Fix the issues above before testing further.")

print("=" * 60)
