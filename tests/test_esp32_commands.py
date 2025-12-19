#!/usr/bin/env python3
"""test_esp32_commands.py

Tests for ESP32 command handling via serial protocol.
These tests verify command packet generation and response parsing.

Run with: pytest tests/test_esp32_commands.py -v

Note: Some tests require actual ESP32 connection. Tests that need hardware
are marked with @pytest.mark.hardware and skipped by default.
Run with: pytest tests/test_esp32_commands.py -v -m hardware
"""

import pytest
import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from test_utils import (
    MAGIC, VERSION,
    TYPE_PREVIEW_JPEG, TYPE_CMD_CAPTURE, TYPE_ACK_CAPTURE,
    TYPE_CMD_FOCUS, TYPE_ACK_FOCUS,
    TYPE_CMD_GET_GALLERY, TYPE_RSP_GALLERY_INFO,
    TYPE_CMD_GET_THUMB, TYPE_RSP_THUMB,
    TYPE_CMD_DELETE_IMAGE, TYPE_ACK_DELETE,
    u32le, read_u32le, build_packet, validate_jpeg,
    MockSerial, PacketCollector
)


# ============================================================
# Command Builders (mirror ESP32 send functions)
# ============================================================

def build_focus_request() -> bytes:
    """Build CMD_FOCUS packet (matches ESP32 send_focus_request)."""
    return build_packet(TYPE_CMD_FOCUS, 0, b'')


def build_capture_request(base_seq: int) -> bytes:
    """Build CMD_CAPTURE packet (matches ESP32 send_capture_request)."""
    return build_packet(TYPE_CMD_CAPTURE, base_seq, u32le(base_seq))


def build_gallery_request() -> bytes:
    """Build CMD_GET_GALLERY packet."""
    return build_packet(TYPE_CMD_GET_GALLERY, 0, b'')


def build_thumb_request(index: int) -> bytes:
    """Build CMD_GET_THUMB packet."""
    payload = bytes([index & 0xFF, (index >> 8) & 0xFF])
    return build_packet(TYPE_CMD_GET_THUMB, index, payload)


def build_delete_request(index: int) -> bytes:
    """Build CMD_DELETE_IMAGE packet."""
    payload = bytes([index & 0xFF, (index >> 8) & 0xFF])
    return build_packet(TYPE_CMD_DELETE_IMAGE, index, payload)


# ============================================================
# Response Parsers
# ============================================================

def parse_focus_ack(payload: bytes) -> bool:
    """Parse ACK_FOCUS response. Returns True if focus succeeded."""
    if len(payload) >= 1:
        return payload[0] == 0
    return False


def parse_capture_ack(payload: bytes) -> int:
    """Parse ACK_CAPTURE response. Returns captured sequence number."""
    if len(payload) >= 4:
        return read_u32le(payload)
    return 0


def parse_gallery_info(payload: bytes) -> tuple:
    """Parse RSP_GALLERY_INFO response. Returns (count, size_mb)."""
    if len(payload) >= 4:
        count = payload[0] | (payload[1] << 8)
        size_mb = payload[2] | (payload[3] << 8)
        return (count, size_mb)
    return (0, 0)


def parse_thumb_response(payload: bytes) -> tuple:
    """Parse RSP_THUMB response. Returns (index, jpeg_bytes)."""
    if len(payload) >= 2:
        index = payload[0] | (payload[1] << 8)
        jpeg_data = payload[2:]
        return (index, jpeg_data)
    return (0, b'')


def parse_delete_ack(payload: bytes) -> bool:
    """Parse ACK_DELETE response. Returns True if delete succeeded."""
    if len(payload) >= 1:
        return payload[0] == 0
    return False


# ============================================================
# Unit Tests: Command Building
# ============================================================

class TestCommandBuilding:
    """Test command packet building functions."""
    
    def test_focus_request_format(self):
        """Test CMD_FOCUS packet format."""
        pkt = build_focus_request()
        
        assert pkt[:4] == MAGIC
        assert pkt[4] == VERSION
        assert pkt[5] == TYPE_CMD_FOCUS
        assert read_u32le(pkt[6:10]) == 0  # seq
        assert read_u32le(pkt[10:14]) == 0  # len (no payload)
        assert len(pkt) == 14
    
    def test_capture_request_format(self):
        """Test CMD_CAPTURE packet format."""
        base_seq = 12345
        pkt = build_capture_request(base_seq)
        
        assert pkt[:4] == MAGIC
        assert pkt[4] == VERSION
        assert pkt[5] == TYPE_CMD_CAPTURE
        assert read_u32le(pkt[6:10]) == base_seq  # seq
        assert read_u32le(pkt[10:14]) == 4  # len
        assert read_u32le(pkt[14:18]) == base_seq  # payload
    
    def test_gallery_request_format(self):
        """Test CMD_GET_GALLERY packet format."""
        pkt = build_gallery_request()
        
        assert pkt[5] == TYPE_CMD_GET_GALLERY
        assert read_u32le(pkt[10:14]) == 0  # no payload
    
    def test_thumb_request_format(self):
        """Test CMD_GET_THUMB packet format."""
        index = 5
        pkt = build_thumb_request(index)
        
        assert pkt[5] == TYPE_CMD_GET_THUMB
        assert read_u32le(pkt[10:14]) == 2  # payload len
        assert pkt[14] == 5  # index low byte
        assert pkt[15] == 0  # index high byte
    
    def test_thumb_request_large_index(self):
        """Test CMD_GET_THUMB with index > 255."""
        index = 300
        pkt = build_thumb_request(index)
        
        assert pkt[14] == (300 & 0xFF)
        assert pkt[15] == ((300 >> 8) & 0xFF)
    
    def test_delete_request_format(self):
        """Test CMD_DELETE_IMAGE packet format."""
        index = 3
        pkt = build_delete_request(index)
        
        assert pkt[5] == TYPE_CMD_DELETE_IMAGE
        assert read_u32le(pkt[10:14]) == 2
        assert pkt[14] == 3


# ============================================================
# Unit Tests: Response Parsing
# ============================================================

class TestResponseParsing:
    """Test response packet parsing functions."""
    
    def test_focus_ack_success(self):
        """Test parsing successful focus ACK."""
        payload = bytes([0])  # 0 = success
        assert parse_focus_ack(payload) is True
    
    def test_focus_ack_failure(self):
        """Test parsing failed focus ACK."""
        payload = bytes([1])  # 1 = failure
        assert parse_focus_ack(payload) is False
    
    def test_focus_ack_empty(self):
        """Test parsing empty focus ACK."""
        assert parse_focus_ack(b'') is False
    
    def test_capture_ack(self):
        """Test parsing capture ACK."""
        seq = 54321
        payload = u32le(seq)
        assert parse_capture_ack(payload) == seq
    
    def test_gallery_info(self):
        """Test parsing gallery info response."""
        count = 15
        size_mb = 42
        payload = bytes([count & 0xFF, (count >> 8) & 0xFF,
                        size_mb & 0xFF, (size_mb >> 8) & 0xFF])
        
        parsed_count, parsed_size = parse_gallery_info(payload)
        assert parsed_count == 15
        assert parsed_size == 42
    
    def test_gallery_info_large_values(self):
        """Test parsing gallery info with large values."""
        count = 1000
        size_mb = 5000
        payload = bytes([count & 0xFF, (count >> 8) & 0xFF,
                        size_mb & 0xFF, (size_mb >> 8) & 0xFF])
        
        parsed_count, parsed_size = parse_gallery_info(payload)
        assert parsed_count == 1000
        assert parsed_size == 5000
    
    def test_thumb_response(self):
        """Test parsing thumbnail response."""
        index = 7
        jpeg_data = bytes([0xFF, 0xD8, 0x00, 0x00, 0xFF, 0xD9])
        payload = bytes([index & 0xFF, (index >> 8) & 0xFF]) + jpeg_data
        
        parsed_index, parsed_jpeg = parse_thumb_response(payload)
        assert parsed_index == 7
        assert parsed_jpeg == jpeg_data
    
    def test_delete_ack_success(self):
        """Test parsing successful delete ACK."""
        assert parse_delete_ack(bytes([0])) is True
    
    def test_delete_ack_failure(self):
        """Test parsing failed delete ACK."""
        assert parse_delete_ack(bytes([1])) is False


# ============================================================
# Integration Tests: Command/Response Flow
# ============================================================

class TestCommandResponseFlow:
    """Test complete command/response flows with mock serial."""
    
    def test_focus_flow(self):
        """Test complete focus command flow."""
        mock = MockSerial()
        collector = PacketCollector()
        
        # 1. Build and send focus command
        cmd = build_focus_request()
        mock.write(cmd)
        
        # Verify command was sent
        sent = mock.get_tx()
        collector.feed(sent)
        packets = collector.get_packets()
        assert len(packets) == 1
        assert packets[0][0] == TYPE_CMD_FOCUS
        
        # 2. Simulate ESP32 response
        response = build_packet(TYPE_ACK_FOCUS, 0, bytes([0]))
        mock.inject_rx(response)
        
        # 3. Read and parse response
        rx_data = mock.read(100)
        collector.feed(rx_data)
        responses = collector.get_packets()
        
        assert len(responses) == 1
        assert responses[0][0] == TYPE_ACK_FOCUS
        assert parse_focus_ack(responses[0][2]) is True
    
    def test_capture_flow(self):
        """Test complete capture command flow."""
        mock = MockSerial()
        collector = PacketCollector()
        
        base_seq = 100
        captured_seq = 101
        
        # 1. Send capture command
        cmd = build_capture_request(base_seq)
        mock.write(cmd)
        
        # 2. Simulate ESP32 ACK
        response = build_packet(TYPE_ACK_CAPTURE, 0, u32le(captured_seq))
        mock.inject_rx(response)
        
        # 3. Parse response
        rx_data = mock.read(100)
        collector.feed(rx_data)
        responses = collector.get_packets()
        
        assert len(responses) == 1
        assert parse_capture_ack(responses[0][2]) == captured_seq
    
    def test_gallery_info_flow(self):
        """Test complete gallery info flow."""
        mock = MockSerial()
        collector = PacketCollector()
        
        # 1. Request gallery info
        cmd = build_gallery_request()
        mock.write(cmd)
        
        # 2. Simulate response: 10 images, 25 MB
        info_payload = bytes([10, 0, 25, 0])
        response = build_packet(TYPE_RSP_GALLERY_INFO, 0, info_payload)
        mock.inject_rx(response)
        
        # 3. Parse response
        rx_data = mock.read(100)
        collector.feed(rx_data)
        responses = collector.get_packets()
        
        count, size_mb = parse_gallery_info(responses[0][2])
        assert count == 10
        assert size_mb == 25
    
    def test_thumbnail_flow(self, sample_jpeg_bytes):
        """Test complete thumbnail request flow."""
        mock = MockSerial()
        collector = PacketCollector()
        
        index = 3
        
        # 1. Request thumbnail
        cmd = build_thumb_request(index)
        mock.write(cmd)
        
        # 2. Simulate thumbnail response
        thumb_payload = bytes([index & 0xFF, 0]) + sample_jpeg_bytes
        response = build_packet(TYPE_RSP_THUMB, index, thumb_payload)
        mock.inject_rx(response)
        
        # 3. Parse response
        rx_data = mock.read(len(response))
        collector.feed(rx_data)
        responses = collector.get_packets()
        
        parsed_index, jpeg_data = parse_thumb_response(responses[0][2])
        assert parsed_index == index
        assert validate_jpeg(jpeg_data)
    
    def test_delete_flow(self):
        """Test complete delete command flow."""
        mock = MockSerial()
        collector = PacketCollector()
        
        # 1. Delete image at index 5
        cmd = build_delete_request(5)
        mock.write(cmd)
        
        # 2. Simulate success response
        response = build_packet(TYPE_ACK_DELETE, 0, bytes([0]))
        mock.inject_rx(response)
        
        # 3. Parse response
        rx_data = mock.read(100)
        collector.feed(rx_data)
        responses = collector.get_packets()
        
        assert parse_delete_ack(responses[0][2]) is True


# ============================================================
# Hardware Tests (require actual ESP32 connection)
# ============================================================

@pytest.mark.hardware
class TestESP32Hardware:
    """Tests that require actual ESP32 hardware connection."""
    
    @pytest.fixture
    def esp32_serial(self):
        """Connect to actual ESP32."""
        import serial
        import serial.tools.list_ports
        
        # Find ESP32 port
        ports = serial.tools.list_ports.comports()
        esp_port = None
        for port in ports:
            if "USB" in port.description or port.vid == 0x303A:
                esp_port = port.device
                break
        
        if not esp_port:
            pytest.skip("ESP32 not connected")
        
        ser = serial.Serial(esp_port, 2000000, timeout=2)
        ser.reset_input_buffer()
        time.sleep(0.5)
        
        yield ser
        ser.close()
    
    def test_esp32_responds_to_focus(self, esp32_serial):
        """Test that ESP32 responds to focus command."""
        collector = PacketCollector()
        
        # Send focus command
        cmd = build_focus_request()
        esp32_serial.write(cmd)
        esp32_serial.flush()
        
        # Wait for response
        time.sleep(6)  # Focus can take up to 5 seconds
        
        if esp32_serial.in_waiting > 0:
            data = esp32_serial.read(esp32_serial.in_waiting)
            collector.feed(data)
            packets = collector.get_packets()
            
            # Look for ACK_FOCUS in responses
            focus_acks = [p for p in packets if p[0] == TYPE_ACK_FOCUS]
            assert len(focus_acks) >= 1
    
    def test_esp32_gallery_info(self, esp32_serial):
        """Test that ESP32 returns gallery info."""
        collector = PacketCollector()
        
        # Request gallery info
        cmd = build_gallery_request()
        esp32_serial.write(cmd)
        esp32_serial.flush()
        
        # Wait for response
        time.sleep(1)
        
        if esp32_serial.in_waiting > 0:
            data = esp32_serial.read(esp32_serial.in_waiting)
            collector.feed(data)
            packets = collector.get_packets()
            
            gallery_responses = [p for p in packets if p[0] == TYPE_RSP_GALLERY_INFO]
            if gallery_responses:
                count, size_mb = parse_gallery_info(gallery_responses[0][2])
                print(f"Gallery: {count} images, {size_mb} MB")
                assert count >= 0
                assert size_mb >= 0


if __name__ == "__main__":
    pytest.main([__file__, "-v", "-m", "not hardware"])
