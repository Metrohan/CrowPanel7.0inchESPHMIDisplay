#!/usr/bin/env python3
"""run_tests.py

Test runner script for CrowPanel test suite.
Provides easy access to different test categories.

Usage:
    python3 tests/run_tests.py              # Run all unit tests
    python3 tests/run_tests.py --quick      # Run quick verification only
    python3 tests/run_tests.py --protocol   # Run protocol tests only
    python3 tests/run_tests.py --gallery    # Run gallery tests only
    python3 tests/run_tests.py --commands   # Run ESP32 command tests only
    python3 tests/run_tests.py --stress     # Run stress tests (slow)
    python3 tests/run_tests.py --hardware   # Run hardware tests (requires ESP32)
    python3 tests/run_tests.py --all        # Run everything including hardware
"""

import sys
import os
import subprocess

# Add tests directory to path
TEST_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(TEST_DIR)


def run_pytest(args: list) -> int:
    """Run pytest with given arguments."""
    cmd = [sys.executable, "-m", "pytest"] + args
    print(f"Running: {' '.join(cmd)}")
    print("=" * 60)
    return subprocess.call(cmd, cwd=PROJECT_DIR)


def main():
    args = sys.argv[1:]
    
    if not args or args[0] == "--help":
        print(__doc__)
        return 0
    
    if args[0] == "--quick":
        # Quick verification
        return run_pytest([
            "tests/test_quick_verify.py",
            "-v"
        ])
    
    elif args[0] == "--protocol":
        # Protocol parser tests
        return run_pytest([
            "tests/test_protocol_parser.py",
            "-v"
        ])
    
    elif args[0] == "--gallery":
        # Gallery management tests
        return run_pytest([
            "tests/test_gallery_management.py",
            "-v"
        ])
    
    elif args[0] == "--commands":
        # ESP32 command tests (without hardware)
        return run_pytest([
            "tests/test_esp32_commands.py",
            "-v", "-m", "not hardware"
        ])
    
    elif args[0] == "--stress":
        # Stress tests (slow, no hardware)
        return run_pytest([
            "tests/test_stress.py",
            "-v", "-s", "-m", "not hardware"
        ])
    
    elif args[0] == "--hardware":
        # Hardware tests only
        return run_pytest([
            "tests/",
            "-v", "-s", "-m", "hardware"
        ])
    
    elif args[0] == "--all":
        # All tests including hardware
        return run_pytest([
            "tests/",
            "-v", "-s"
        ])
    
    else:
        # Default: run all unit tests (no hardware, no slow)
        return run_pytest([
            "tests/",
            "-v",
            "-m", "not hardware and not slow"
        ])


if __name__ == "__main__":
    sys.exit(main())
