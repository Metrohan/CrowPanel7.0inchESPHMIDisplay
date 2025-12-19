#!/usr/bin/env python3
"""test_autofocus.py

IMX519 autofocus test - doğrudan Pi'da çalıştır.
Bu script kameranın AF özelliklerini test eder.

Usage: python3 test_autofocus.py
"""

import time
import sys

print("=" * 50)
print("IMX519 Autofocus Test")
print("=" * 50)

try:
    from picamera2 import Picamera2
except ImportError:
    print("[FAIL] Picamera2 not installed")
    sys.exit(1)

# Initialize camera
print("\n[1] Initializing camera...")
picam2 = Picamera2()

# Use simple configuration
config = picam2.create_video_configuration(
    main={"size": (640, 480), "format": "YUV420"}
)
picam2.configure(config)
picam2.start()
time.sleep(1)
print("[OK] Camera started")

# Check available controls
print("\n[2] Checking AF controls...")
controls = picam2.camera_controls

af_available = False
for key in ['AfMode', 'AfTrigger', 'AfState', 'LensPosition']:
    if key in controls:
        print(f"  ✅ {key}: {controls[key]}")
        af_available = True
    else:
        print(f"  ❌ {key}: NOT AVAILABLE")

if not af_available:
    print("\n[FAIL] No AF controls found! This camera may not support autofocus.")
    picam2.stop()
    sys.exit(1)

# Get initial metadata
print("\n[3] Initial AF state...")
try:
    meta = picam2.capture_metadata()
    print(f"  AfMode: {meta.get('AfMode', 'N/A')}")
    print(f"  AfState: {meta.get('AfState', 'N/A')}")
    print(f"  LensPosition: {meta.get('LensPosition', 'N/A')}")
except Exception as e:
    print(f"  Error reading metadata: {e}")

# Test Continuous AF
print("\n[4] Testing Continuous AF (AfMode=2)...")
try:
    picam2.set_controls({"AfMode": 2})
    print("  Set AfMode=2 (Continuous)")
    time.sleep(2)
    
    meta = picam2.capture_metadata()
    print(f"  AfState: {meta.get('AfState', 'N/A')}")
    print(f"  LensPosition: {meta.get('LensPosition', 'N/A')}")
except Exception as e:
    print(f"  Error: {e}")

# Test One-shot AF
print("\n[5] Testing One-shot AF (AfMode=1 + AfTrigger)...")
try:
    # Switch to Auto mode
    picam2.set_controls({"AfMode": 1})
    print("  Set AfMode=1 (Auto/One-shot)")
    time.sleep(0.2)
    
    # Trigger AF
    picam2.set_controls({"AfTrigger": 0})
    print("  Triggered AF (AfTrigger=0)")
    
    # Monitor AF progress
    print("  Monitoring AF progress...")
    start_time = time.time()
    success = False
    
    for i in range(60):  # Max 6 seconds
        time.sleep(0.1)
        meta = picam2.capture_metadata()
        af_state = meta.get("AfState", -1)
        lens_pos = meta.get("LensPosition", 0)
        
        # Print every 0.5 seconds
        if i % 5 == 0:
            state_names = {0: "Idle", 1: "Scanning", 2: "Focused", 3: "Failed"}
            state_name = state_names.get(af_state, f"Unknown({af_state})")
            print(f"    {i*0.1:.1f}s: State={state_name}, LensPos={lens_pos:.3f}")
        
        if af_state == 2:  # Focused
            elapsed = time.time() - start_time
            print(f"\n  ✅ AF SUCCEEDED in {elapsed:.1f}s! LensPos={lens_pos:.3f}")
            success = True
            break
        elif af_state == 3:  # Failed
            print(f"\n  ❌ AF FAILED at LensPos={lens_pos:.3f}")
            # Retry once
            print("  Retrying...")
            picam2.set_controls({"AfTrigger": 0})
    
    if not success:
        print("\n  ⚠️ AF TIMEOUT - did not complete in 6 seconds")
        
except Exception as e:
    print(f"  Error: {e}")
    import traceback
    traceback.print_exc()

# Test Manual focus positions
print("\n[6] Testing Manual focus (AfMode=0)...")
try:
    picam2.set_controls({"AfMode": 0})
    print("  Set AfMode=0 (Manual)")
    time.sleep(0.2)
    
    # Try different lens positions
    test_positions = [0.0, 2.0, 5.0, 10.0, 15.0]
    
    for pos in test_positions:
        try:
            picam2.set_controls({"LensPosition": pos})
            time.sleep(0.3)
            meta = picam2.capture_metadata()
            actual_pos = meta.get("LensPosition", -1)
            print(f"    Set {pos:.1f} -> Actual: {actual_pos:.2f}")
        except Exception as e:
            print(f"    Set {pos:.1f} -> Error: {e}")
            
except Exception as e:
    print(f"  Error: {e}")

# Cleanup
print("\n[7] Cleanup...")
try:
    picam2.set_controls({"AfMode": 2})  # Back to continuous
    print("  Set AfMode=2 (Continuous)")
except:
    pass

picam2.stop()
print("  Camera stopped")

print("\n" + "=" * 50)
print("Test complete!")
print("=" * 50)
print("\nIf AF worked, the 'Focused' state should have been reached.")
print("If not, check that your IMX519 module has a working AF motor.")
