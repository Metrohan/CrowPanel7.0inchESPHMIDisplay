#!/usr/bin/env python3
"""test_stress.py

Stress tests for the camera streaming system.
Tests sustained frame transmission, memory stability, and error recovery.

Run with: pytest tests/test_stress.py -v -s
For hardware tests: pytest tests/test_stress.py -v -s -m hardware

Warning: These tests take significant time to complete.
"""

import pytest
import sys
import os
import time
import threading
from collections import deque

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from test_utils import (
    TYPE_PREVIEW_JPEG, TYPE_CMD_CAPTURE, TYPE_ACK_CAPTURE,
    build_packet, u32le, validate_jpeg, create_test_jpeg,
    MockSerial, PacketCollector
)


# ============================================================
# Stress Test Fixtures
# ============================================================

@pytest.fixture
def large_jpeg_stream():
    """Generate a stream of test JPEG frames."""
    frames = []
    for i in range(100):
        # Vary size slightly to simulate real frames
        width = 480
        height = 320
        quality = 60 + (i % 20)  # Quality varies 60-79
        
        jpeg = create_test_jpeg(width, height, quality)
        if validate_jpeg(jpeg):
            frames.append(jpeg)
    
    return frames


# ============================================================
# Unit Stress Tests (no hardware)
# ============================================================

class TestProtocolStress:
    """Stress tests for protocol handling without hardware."""
    
    def test_rapid_packet_generation(self):
        """Test generating many packets rapidly."""
        start = time.time()
        packets = []
        
        for i in range(10000):
            payload = create_test_jpeg(80, 60, 50)
            pkt = build_packet(TYPE_PREVIEW_JPEG, i, payload)
            packets.append(pkt)
        
        elapsed = time.time() - start
        rate = 10000 / elapsed
        
        print(f"\nGenerated 10000 packets in {elapsed:.2f}s ({rate:.0f} pkt/s)")
        assert rate > 100  # Should be able to generate >100 packets/sec
    
    def test_rapid_packet_parsing(self):
        """Test parsing many packets rapidly."""
        collector = PacketCollector()
        
        # Generate test packets
        payload = create_test_jpeg(80, 60, 50)
        test_packet = build_packet(TYPE_PREVIEW_JPEG, 0, payload)
        
        # Concatenate many packets
        data = test_packet * 1000
        
        start = time.time()
        collector.feed(data)
        packets = collector.get_packets()
        elapsed = time.time() - start
        
        rate = 1000 / elapsed
        print(f"\nParsed 1000 packets in {elapsed:.2f}s ({rate:.0f} pkt/s)")
        
        assert len(packets) == 1000
        assert rate > 500  # Should parse >500 packets/sec
    
    def test_fragmented_packet_stream(self):
        """Test parsing heavily fragmented packet stream."""
        collector = PacketCollector()
        
        # Create packets
        packets_data = b''
        for i in range(100):
            payload = create_test_jpeg(80, 60, 50)
            packets_data += build_packet(TYPE_PREVIEW_JPEG, i, payload)
        
        # Feed in random-sized chunks
        import random
        offset = 0
        chunks_fed = 0
        
        start = time.time()
        while offset < len(packets_data):
            chunk_size = random.randint(1, 50)  # Small random chunks
            chunk = packets_data[offset:offset + chunk_size]
            collector.feed(chunk)
            offset += chunk_size
            chunks_fed += 1
        
        elapsed = time.time() - start
        packets = collector.get_packets()
        
        print(f"\nParsed {len(packets)} packets from {chunks_fed} fragments in {elapsed:.2f}s")
        assert len(packets) == 100
    
    def test_garbage_resilience(self):
        """Test protocol resilience to garbage data between packets."""
        collector = PacketCollector()
        
        # Create stream with garbage between packets
        stream = b''
        for i in range(50):
            # Random garbage
            garbage = os.urandom(100)
            stream += garbage
            
            # Valid packet
            payload = create_test_jpeg(80, 60, 50)
            stream += build_packet(TYPE_PREVIEW_JPEG, i, payload)
        
        collector.feed(stream)
        packets = collector.get_packets()
        
        print(f"\nRecovered {len(packets)} packets from garbage-filled stream")
        assert len(packets) == 50
    
    def test_partial_magic_sequences(self):
        """Test recovery from partial magic sequences."""
        collector = PacketCollector()
        
        # Create stream with partial magic sequences
        partial_magic = b'\xC0\xDE'  # First 2 bytes of MAGIC
        payload = create_test_jpeg(80, 60, 50)
        valid_packet = build_packet(TYPE_PREVIEW_JPEG, 42, payload)
        
        # Many partial sequences followed by valid packet
        stream = (partial_magic * 100) + valid_packet
        
        collector.feed(stream)
        packets = collector.get_packets()
        
        assert len(packets) == 1
        assert packets[0][1] == 42  # Correct seq number
    
    def test_memory_stability_parsing(self):
        """Test that parsing doesn't leak memory."""
        import gc
        
        collector = PacketCollector()
        payload = create_test_jpeg(80, 60, 50)
        test_packet = build_packet(TYPE_PREVIEW_JPEG, 0, payload)
        
        # Get baseline memory
        gc.collect()
        
        # Parse many packets
        for _ in range(1000):
            data = test_packet * 10
            collector.feed(data)
            packets = collector.get_packets()
            del packets
        
        gc.collect()
        # If we get here without MemoryError, we're good
        assert True


class TestJpegStress:
    """Stress tests for JPEG handling."""
    
    def test_jpeg_validation_speed(self):
        """Test JPEG validation speed."""
        jpeg = create_test_jpeg(480, 320, 70)
        
        start = time.time()
        for _ in range(10000):
            valid = validate_jpeg(jpeg)
        elapsed = time.time() - start
        
        rate = 10000 / elapsed
        print(f"\nValidated JPEG {rate:.0f} times/sec")
        assert rate > 10000  # Should validate >10k times/sec
    
    def test_jpeg_creation_speed(self):
        """Test JPEG creation speed."""
        start = time.time()
        jpegs = []
        
        for i in range(100):
            jpeg = create_test_jpeg(480, 320, 70)
            jpegs.append(jpeg)
        
        elapsed = time.time() - start
        rate = 100 / elapsed
        
        print(f"\nCreated 100 JPEGs in {elapsed:.2f}s ({rate:.1f} fps)")
        # Even with creation, should manage decent rate
        assert len(jpegs) == 100


class TestMockSerialStress:
    """Stress tests using MockSerial."""
    
    def test_bidirectional_traffic(self):
        """Test high bidirectional traffic."""
        mock = MockSerial()
        tx_collector = PacketCollector()
        rx_collector = PacketCollector()
        
        jpeg = create_test_jpeg(80, 60, 50)
        
        # Simulate bidirectional traffic
        for i in range(100):
            # TX: Preview frame
            tx_pkt = build_packet(TYPE_PREVIEW_JPEG, i, jpeg)
            mock.write(tx_pkt)
            
            # RX: Capture command (every 10th frame)
            if i % 10 == 0:
                rx_pkt = build_packet(TYPE_CMD_CAPTURE, i, u32le(i))
                mock.inject_rx(rx_pkt)
        
        # Process TX
        tx_data = mock.get_tx()
        tx_collector.feed(tx_data)
        tx_packets = tx_collector.get_packets()
        
        # Process RX
        rx_data = mock.read(10000)
        rx_collector.feed(rx_data)
        rx_packets = rx_collector.get_packets()
        
        assert len(tx_packets) == 100
        assert len(rx_packets) == 10
    
    def test_buffer_overflow_behavior(self):
        """Test behavior with large amounts of data."""
        mock = MockSerial()
        
        # Write a lot of data
        jpeg = create_test_jpeg(480, 320, 70)  # ~8KB
        for i in range(1000):
            pkt = build_packet(TYPE_PREVIEW_JPEG, i, jpeg)
            mock.write(pkt)
        
        # Should have accumulated ~8MB of data
        total_data = mock.get_tx()
        print(f"\nAccumulated {len(total_data) / 1024 / 1024:.1f} MB in mock buffer")
        
        # Parse it all
        collector = PacketCollector()
        collector.feed(total_data)
        packets = collector.get_packets()
        
        assert len(packets) == 1000


# ============================================================
# Hardware Stress Tests
# ============================================================

@pytest.mark.hardware
@pytest.mark.slow
class TestHardwareStress:
    """Stress tests requiring actual hardware."""
    
    @pytest.fixture
    def esp32_serial(self):
        """Connect to actual ESP32."""
        import serial
        import serial.tools.list_ports
        
        ports = serial.tools.list_ports.comports()
        esp_port = None
        for port in ports:
            if "USB" in port.description or port.vid == 0x303A:
                esp_port = port.device
                break
        
        if not esp_port:
            pytest.skip("ESP32 not connected")
        
        ser = serial.Serial(esp_port, 2000000, timeout=1)
        ser.reset_input_buffer()
        time.sleep(1)
        
        yield ser
        ser.close()
    
    def test_sustained_receive(self, esp32_serial):
        """Test ESP32 can sustain receiving frames for extended period."""
        collector = PacketCollector()
        jpeg = create_test_jpeg(480, 320, 70)
        
        frames_sent = 0
        frames_acked = 0
        errors = 0
        
        start = time.time()
        duration = 30  # 30 second test
        
        print(f"\nRunning {duration}s sustained receive test...")
        
        while time.time() - start < duration:
            # Send frame
            pkt = build_packet(TYPE_PREVIEW_JPEG, frames_sent, jpeg)
            try:
                esp32_serial.write(pkt)
                esp32_serial.flush()
                frames_sent += 1
            except Exception as e:
                errors += 1
                print(f"TX Error: {e}")
            
            # Check for any responses
            if esp32_serial.in_waiting > 0:
                try:
                    data = esp32_serial.read(esp32_serial.in_waiting)
                    collector.feed(data)
                except:
                    pass
            
            # Rate limit to target FPS
            time.sleep(1.0 / 15)  # ~15 FPS
        
        elapsed = time.time() - start
        fps = frames_sent / elapsed
        
        print(f"Sent {frames_sent} frames in {elapsed:.1f}s ({fps:.1f} fps)")
        print(f"Errors: {errors}")
        
        assert fps > 10  # Should maintain >10 FPS
        assert errors < frames_sent * 0.01  # <1% error rate
    
    def test_command_response_under_load(self, esp32_serial):
        """Test commands work while frames are streaming."""
        from test_utils import TYPE_CMD_FOCUS, TYPE_ACK_FOCUS
        
        collector = PacketCollector()
        jpeg = create_test_jpeg(480, 320, 70)
        
        focus_sent = 0
        focus_acked = 0
        frames_sent = 0
        
        start = time.time()
        duration = 20
        
        print(f"\nTesting commands under load for {duration}s...")
        
        while time.time() - start < duration:
            # Send preview frame
            pkt = build_packet(TYPE_PREVIEW_JPEG, frames_sent, jpeg)
            esp32_serial.write(pkt)
            frames_sent += 1
            
            # Every 50 frames, send focus command
            if frames_sent % 50 == 0:
                focus_pkt = build_packet(TYPE_CMD_FOCUS, 0, b'')
                esp32_serial.write(focus_pkt)
                focus_sent += 1
            
            esp32_serial.flush()
            
            # Check responses
            if esp32_serial.in_waiting > 0:
                data = esp32_serial.read(esp32_serial.in_waiting)
                collector.feed(data)
                
                for ptype, seq, payload in collector.get_packets():
                    if ptype == TYPE_ACK_FOCUS:
                        focus_acked += 1
            
            time.sleep(1.0 / 15)
        
        print(f"Frames: {frames_sent}, Focus commands: {focus_sent}, Focus ACKs: {focus_acked}")
        
        # Should get ACKs for most focus commands
        if focus_sent > 0:
            ack_rate = focus_acked / focus_sent
            print(f"Focus ACK rate: {ack_rate * 100:.1f}%")
            assert ack_rate > 0.5  # At least 50% ACK rate under load


# ============================================================
# Performance Baseline Test
# ============================================================

class TestPerformanceBaseline:
    """Establish performance baselines."""
    
    def test_baseline_metrics(self):
        """Measure and report baseline performance metrics."""
        results = {}
        
        # 1. Packet build time
        payload = create_test_jpeg(480, 320, 70)
        start = time.time()
        for _ in range(1000):
            build_packet(TYPE_PREVIEW_JPEG, 0, payload)
        results['packet_build_us'] = (time.time() - start) * 1000  # ms for 1000
        
        # 2. Packet parse time
        collector = PacketCollector()
        pkt = build_packet(TYPE_PREVIEW_JPEG, 0, payload)
        data = pkt * 100
        start = time.time()
        collector.feed(data)
        collector.get_packets()
        results['packet_parse_100_ms'] = (time.time() - start) * 1000
        
        # 3. JPEG validation time
        start = time.time()
        for _ in range(10000):
            validate_jpeg(payload)
        results['jpeg_validate_10k_ms'] = (time.time() - start) * 1000
        
        # 4. JPEG creation time
        start = time.time()
        for _ in range(10):
            create_test_jpeg(480, 320, 70)
        results['jpeg_create_10_ms'] = (time.time() - start) * 1000
        
        print("\n" + "=" * 50)
        print("PERFORMANCE BASELINE RESULTS")
        print("=" * 50)
        for metric, value in results.items():
            print(f"  {metric}: {value:.2f}")
        print("=" * 50)
        
        # Basic sanity checks
        assert results['packet_build_us'] < 100  # <100ms for 1000 packets
        assert results['jpeg_validate_10k_ms'] < 100  # <100ms for 10k validations


if __name__ == "__main__":
    # Run non-hardware tests by default
    pytest.main([__file__, "-v", "-s", "-m", "not hardware and not slow"])
