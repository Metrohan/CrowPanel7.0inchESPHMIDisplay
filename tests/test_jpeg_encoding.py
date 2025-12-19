#!/usr/bin/env python3
"""test_jpeg_encoding.py

JPEG encoding test - Picamera2 frame'lerinin doğru encode edildiğini doğrular.
Kamera bağlı Pi'da çalıştırın.

Bu script:
1. Kameradan frame yakalar
2. YUV420 -> BGR dönüşümü yapar
3. JPEG encode eder
4. JPEG header/footer doğrular
5. Decode edip görsel karşılaştırma yapar
"""

import os
import sys
import time
import numpy as np

print("=" * 60)
print("JPEG Encoding Test")
print("=" * 60)

# ============================================================
# Imports
# ============================================================
try:
    import cv2
    print(f"[OK] OpenCV {cv2.__version__}")
except ImportError:
    print("[FAIL] OpenCV not installed: pip3 install opencv-python")
    sys.exit(1)

try:
    from picamera2 import Picamera2
    print("[OK] Picamera2 imported")
except ImportError:
    print("[FAIL] Picamera2 not installed")
    sys.exit(1)

# ============================================================
# Configuration
# ============================================================
RES_WIDTH = 480
RES_HEIGHT = 320
QUALITY_LEVELS = [40, 50, 60, 70, 80, 90]
OUTPUT_DIR = "jpeg_test_output"

os.makedirs(OUTPUT_DIR, exist_ok=True)

# ============================================================
# Camera Setup
# ============================================================
print("\n--- Camera Setup ---")

picam2 = Picamera2()

# Test different configurations
configs_to_test = [
    {
        "name": "Dual YUV420 (Main 4K + Lores 480x320)",
        "config": picam2.create_video_configuration(
            main={"size": (3840, 2160), "format": "YUV420"},
            lores={"size": (RES_WIDTH, RES_HEIGHT), "format": "YUV420"},
        ),
        "use_lores": True,
    },
    {
        "name": "Single YUV420 480x320",
        "config": picam2.create_video_configuration(
            main={"size": (RES_WIDTH, RES_HEIGHT), "format": "YUV420"},
        ),
        "use_lores": False,
    },
]

def test_config(cfg_info):
    name = cfg_info["name"]
    config = cfg_info["config"]
    use_lores = cfg_info["use_lores"]
    
    print(f"\n{'='*50}")
    print(f"Testing: {name}")
    print(f"{'='*50}")
    
    try:
        picam2.configure(config)
        picam2.start()
        time.sleep(1)
        
        # Capture frame
        request = picam2.capture_request()
        
        try:
            stream_name = "lores" if use_lores else "main"
            frame_yuv = request.make_array(stream_name)
            
            if frame_yuv is None:
                print(f"[FAIL] {stream_name} frame is None")
                return False
            
            print(f"[INFO] Raw frame: shape={frame_yuv.shape}, dtype={frame_yuv.dtype}")
            
            # Convert YUV420 to BGR
            if frame_yuv.ndim == 2:
                h_buf, w_buf = frame_yuv.shape
                actual_h = int(h_buf * 2 / 3)
                print(f"[INFO] YUV420 planar: buffer {w_buf}x{h_buf} -> actual {w_buf}x{actual_h}")
                
                frame_bgr = cv2.cvtColor(frame_yuv, cv2.COLOR_YUV2BGR_I420)
            elif frame_yuv.ndim == 3 and frame_yuv.shape[2] == 3:
                frame_bgr = cv2.cvtColor(frame_yuv, cv2.COLOR_RGB2BGR)
            else:
                print(f"[FAIL] Unknown format: ndim={frame_yuv.ndim}")
                return False
            
            print(f"[INFO] BGR frame: shape={frame_bgr.shape}")
            
            # Resize if needed
            out_h, out_w = frame_bgr.shape[:2]
            if (out_w, out_h) != (RES_WIDTH, RES_HEIGHT):
                print(f"[WARN] Resizing from {out_w}x{out_h} to {RES_WIDTH}x{RES_HEIGHT}")
                frame_bgr = cv2.resize(frame_bgr, (RES_WIDTH, RES_HEIGHT))
            
            # Save BGR as PNG (lossless reference)
            png_path = os.path.join(OUTPUT_DIR, f"{name.replace(' ', '_')}_bgr.png")
            cv2.imwrite(png_path, frame_bgr)
            print(f"[OK] Saved BGR as PNG: {png_path}")
            
            # Test JPEG encoding at different quality levels
            print(f"\n--- JPEG Quality Tests ---")
            
            for quality in QUALITY_LEVELS:
                result, jpeg_array = cv2.imencode(
                    ".jpg", frame_bgr, 
                    [int(cv2.IMWRITE_JPEG_QUALITY), quality]
                )
                
                if not result:
                    print(f"[FAIL] Q{quality}: encoding failed")
                    continue
                
                jpeg_bytes = jpeg_array.tobytes()
                size_kb = len(jpeg_bytes) / 1024
                
                # Validate header/footer
                header_ok = (jpeg_bytes[0] == 0xFF and jpeg_bytes[1] == 0xD8)
                footer_ok = (jpeg_bytes[-2] == 0xFF and jpeg_bytes[-1] == 0xD9)
                
                # Check SOF marker
                sof_found = False
                for i in range(len(jpeg_bytes) - 1):
                    if jpeg_bytes[i:i+2] in [b'\xFF\xC0', b'\xFF\xC1', b'\xFF\xC2']:
                        sof_found = True
                        break
                
                status = "✅" if (header_ok and footer_ok and sof_found) else "❌"
                
                print(f"  {status} Q{quality}: {size_kb:.1f} KB | header={header_ok} footer={footer_ok} SOF={sof_found}")
                
                # Save JPEG
                jpeg_path = os.path.join(OUTPUT_DIR, f"{name.replace(' ', '_')}_q{quality}.jpg")
                with open(jpeg_path, "wb") as f:
                    f.write(jpeg_bytes)
                
                # Decode and verify dimensions
                decoded = cv2.imdecode(jpeg_array, cv2.IMREAD_COLOR)
                if decoded is not None:
                    dec_h, dec_w = decoded.shape[:2]
                    if (dec_w, dec_h) == (RES_WIDTH, RES_HEIGHT):
                        print(f"       Decode OK: {dec_w}x{dec_h}")
                    else:
                        print(f"       Decode WARN: {dec_w}x{dec_h} (expected {RES_WIDTH}x{RES_HEIGHT})")
                else:
                    print(f"       Decode FAIL: cv2.imdecode returned None")
            
            return True
            
        finally:
            request.release()
            picam2.stop()
            
    except Exception as e:
        print(f"[FAIL] {e}")
        import traceback
        traceback.print_exc()
        try:
            picam2.stop()
        except:
            pass
        return False

# ============================================================
# Run Tests
# ============================================================
results = []

for cfg in configs_to_test:
    try:
        success = test_config(cfg)
        results.append((cfg["name"], success))
    except Exception as e:
        print(f"[ERROR] Config test failed: {e}")
        results.append((cfg["name"], False))

# ============================================================
# Binary Dump Test
# ============================================================
print(f"\n{'='*50}")
print("Binary Header Analysis")
print(f"{'='*50}")

# Find a JPEG file to analyze
jpeg_files = [f for f in os.listdir(OUTPUT_DIR) if f.endswith('.jpg')]

if jpeg_files:
    test_file = os.path.join(OUTPUT_DIR, jpeg_files[0])
    
    with open(test_file, "rb") as f:
        data = f.read()
    
    print(f"\nFile: {test_file}")
    print(f"Size: {len(data)} bytes")
    
    # Show first 32 bytes
    print(f"\nFirst 32 bytes (hex):")
    for i in range(0, min(32, len(data)), 16):
        hex_str = ' '.join(f'{b:02X}' for b in data[i:i+16])
        ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in data[i:i+16])
        print(f"  {i:04X}: {hex_str}  |{ascii_str}|")
    
    # Show last 16 bytes
    print(f"\nLast 16 bytes (hex):")
    start = max(0, len(data) - 16)
    hex_str = ' '.join(f'{b:02X}' for b in data[start:])
    print(f"  {start:04X}: {hex_str}")
    
    # Parse JPEG markers
    print(f"\nJPEG Markers found:")
    i = 0
    while i < len(data) - 1:
        if data[i] == 0xFF and data[i+1] != 0x00:
            marker = data[i:i+2]
            marker_hex = marker.hex().upper()
            
            marker_names = {
                'FFD8': 'SOI (Start of Image)',
                'FFD9': 'EOI (End of Image)',
                'FFC0': 'SOF0 (Baseline DCT)',
                'FFC1': 'SOF1 (Extended Sequential)',
                'FFC2': 'SOF2 (Progressive)',
                'FFC4': 'DHT (Huffman Table)',
                'FFDB': 'DQT (Quantization Table)',
                'FFDA': 'SOS (Start of Scan)',
                'FFE0': 'APP0 (JFIF)',
                'FFE1': 'APP1 (EXIF)',
            }
            
            name = marker_names.get(marker_hex, 'Unknown')
            print(f"  {i:04X}: {marker_hex} - {name}")
            
            i += 2
        else:
            i += 1

# ============================================================
# Summary
# ============================================================
print(f"\n{'='*60}")
print("SUMMARY")
print(f"{'='*60}")

for name, success in results:
    status = "✅ PASS" if success else "❌ FAIL"
    print(f"  {status}: {name}")

print(f"\nTest files saved to: {OUTPUT_DIR}/")
print("\nNext steps:")
print("1. Check the saved JPEG files open correctly on your computer")
print("2. Compare PNG (lossless) vs JPEG at different quality levels")
print("3. If all tests pass here but ESP32 still fails, the issue is in transmission or ESP32 decoding")
