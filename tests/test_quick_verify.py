#!/usr/bin/env python3
"""test_quick_verify.py

Hızlı doğrulama scripti - sistemin temel işlevlerini hızlıca kontrol eder.
Tam test için diğer test_*.py scriptlerini kullanın.

Usage:
    python3 test_quick_verify.py
"""

import sys
import os

def check(name, condition, details=""):
    status = "✅" if condition else "❌"
    print(f"{status} {name}" + (f" ({details})" if details else ""))
    return condition

print("=" * 50)
print("Quick System Verification")
print("=" * 50)

all_ok = True

# 1. Python version
all_ok &= check("Python 3.7+", sys.version_info >= (3, 7), f"{sys.version_info.major}.{sys.version_info.minor}")

# 2. Required modules
modules = [("numpy", "NumPy"), ("cv2", "OpenCV"), ("serial", "PySerial"), ("picamera2", "Picamera2")]
for mod, name in modules:
    try:
        __import__(mod)
        all_ok &= check(f"{name} installed", True)
    except ImportError:
        all_ok &= check(f"{name} installed", False, "pip3 install " + mod)

# 3. Camera available
try:
    from picamera2 import Picamera2
    cams = Picamera2.global_camera_info()
    all_ok &= check("Camera detected", len(cams) > 0, f"{len(cams)} camera(s)")
except Exception as e:
    all_ok &= check("Camera detected", False, str(e))

# 4. Serial port available
try:
    import serial.tools.list_ports
    ports = [p for p in serial.tools.list_ports.comports() 
             if "USB" in p.description or "ACM" in p.device or p.vid == 0x303A]
    all_ok &= check("ESP32 serial port", len(ports) > 0, ports[0].device if ports else "not found")
except Exception as e:
    all_ok &= check("ESP32 serial port", False, str(e))

# 5. Test JPEG encode/decode roundtrip
try:
    import cv2
    import numpy as np
    
    # Create test image
    img = np.zeros((320, 480, 3), dtype=np.uint8)
    img[:, :, 2] = 255  # Red
    
    # Encode
    result, enc = cv2.imencode(".jpg", img, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
    
    if result:
        # Check JPEG validity
        data = enc.tobytes()
        valid = (data[0] == 0xFF and data[1] == 0xD8 and 
                 data[-2] == 0xFF and data[-1] == 0xD9)
        all_ok &= check("JPEG encode/validate", valid, f"{len(data)} bytes, header/footer OK" if valid else "invalid")
        
        # Decode
        dec = cv2.imdecode(enc, cv2.IMREAD_COLOR)
        all_ok &= check("JPEG decode", dec is not None and dec.shape == (320, 480, 3))
    else:
        all_ok &= check("JPEG encode", False)
except Exception as e:
    all_ok &= check("JPEG roundtrip", False, str(e))

# Summary
print("\n" + "=" * 50)
if all_ok:
    print("🎉 All checks passed! System ready.")
    print("\nNext steps:")
    print("1. Flash main.ino to ESP32")
    print("2. Run: python3 test_serial_loopback.py")
    print("3. Check ESP32 serial output for [DECODE]/[OK]/[ERROR]")
else:
    print("⚠️  Some checks failed. Fix issues above.")
print("=" * 50)
