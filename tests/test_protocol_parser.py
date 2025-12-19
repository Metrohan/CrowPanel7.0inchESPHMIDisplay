#!/usr/bin/env python3
"""test_protocol_parser.py

Unit tests for the PacketParser class and protocol functions.
Tests packet building, parsing, and edge cases.

Run with: pytest tests/test_protocol_parser.py -v
"""

import pytest
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from test_utils import (
    MAGIC, VERSION, TYPE_PREVIEW_JPEG, TYPE_CMD_CAPTURE, TYPE_ACK_CAPTURE,
    TYPE_CMD_FOCUS, TYPE_ACK_FOCUS, TYPE_CMD_GET_GALLERY, TYPE_RSP_GALLERY_INFO,
    u32le, read_u32le, build_packet, validate_jpeg, PacketCollector
)


class TestProtocolHelpers:
    """Test protocol helper functions."""
    
    def test_u32le_zero(self):
        """Test u32le with zero."""
        assert u32le(0) == b'\x00\x00\x00\x00'
    
    def test_u32le_small(self):
        """Test u32le with small value."""
        assert u32le(1) == b'\x01\x00\x00\x00'
        assert u32le(255) == b'\xff\x00\x00\x00'
    
    def test_u32le_large(self):
        """Test u32le with larger values."""
        assert u32le(0x12345678) == b'\x78\x56\x34\x12'
        assert u32le(0xFFFFFFFF) == b'\xff\xff\xff\xff'
    
    def test_u32le_overflow(self):
        """Test u32le with values > 32-bit (should mask)."""
        assert u32le(0x1_0000_0001) == b'\x01\x00\x00\x00'
    
    def test_read_u32le_roundtrip(self):
        """Test u32le and read_u32le are inverse operations."""
        for val in [0, 1, 255, 65535, 0x12345678, 0xFFFFFFFF]:
            assert read_u32le(u32le(val)) == val
    
    def test_build_packet_no_payload(self):
        """Test building packet with no payload."""
        pkt = build_packet(TYPE_CMD_FOCUS, 0, b'')
        
        assert pkt[:4] == MAGIC
        assert pkt[4] == VERSION
        assert pkt[5] == TYPE_CMD_FOCUS
        assert read_u32le(pkt[6:10]) == 0  # seq
        assert read_u32le(pkt[10:14]) == 0  # len
        assert len(pkt) == 14
    
    def test_build_packet_with_payload(self):
        """Test building packet with payload."""
        payload = b'\x01\x02\x03\x04'
        pkt = build_packet(TYPE_CMD_CAPTURE, 42, payload)
        
        assert pkt[:4] == MAGIC
        assert pkt[4] == VERSION
        assert pkt[5] == TYPE_CMD_CAPTURE
        assert read_u32le(pkt[6:10]) == 42  # seq
        assert read_u32le(pkt[10:14]) == 4  # len
        assert pkt[14:] == payload
    
    def test_build_packet_large_payload(self):
        """Test building packet with large payload."""
        payload = bytes(range(256)) * 10  # 2560 bytes
        pkt = build_packet(TYPE_PREVIEW_JPEG, 100, payload)
        
        assert len(pkt) == 14 + len(payload)
        assert read_u32le(pkt[10:14]) == len(payload)
        assert pkt[14:] == payload


class TestPacketCollector:
    """Test PacketCollector class."""
    
    def test_collect_single_packet_no_payload(self):
        """Test collecting a single packet without payload."""
        collector = PacketCollector()
        pkt = build_packet(TYPE_CMD_FOCUS, 0, b'')
        
        collector.feed(pkt)
        packets = collector.get_packets()
        
        assert len(packets) == 1
        ptype, seq, payload = packets[0]
        assert ptype == TYPE_CMD_FOCUS
        assert seq == 0
        assert payload == b''
    
    def test_collect_single_packet_with_payload(self):
        """Test collecting a single packet with payload."""
        collector = PacketCollector()
        test_payload = b'\xDE\xAD\xBE\xEF'
        pkt = build_packet(TYPE_ACK_CAPTURE, 123, test_payload)
        
        collector.feed(pkt)
        packets = collector.get_packets()
        
        assert len(packets) == 1
        ptype, seq, payload = packets[0]
        assert ptype == TYPE_ACK_CAPTURE
        assert seq == 123
        assert payload == test_payload
    
    def test_collect_multiple_packets(self):
        """Test collecting multiple packets in sequence."""
        collector = PacketCollector()
        
        pkt1 = build_packet(TYPE_CMD_FOCUS, 1, b'')
        pkt2 = build_packet(TYPE_ACK_FOCUS, 1, b'\x00')
        pkt3 = build_packet(TYPE_CMD_GET_GALLERY, 2, b'')
        
        collector.feed(pkt1 + pkt2 + pkt3)
        packets = collector.get_packets()
        
        assert len(packets) == 3
        assert packets[0][0] == TYPE_CMD_FOCUS
        assert packets[1][0] == TYPE_ACK_FOCUS
        assert packets[2][0] == TYPE_CMD_GET_GALLERY
    
    def test_collect_fragmented_packet(self):
        """Test collecting packet received in fragments."""
        collector = PacketCollector()
        test_payload = b'Hello, ESP32!'
        pkt = build_packet(TYPE_PREVIEW_JPEG, 42, test_payload)
        
        # Feed in small chunks
        for i in range(0, len(pkt), 3):
            collector.feed(pkt[i:i+3])
        
        packets = collector.get_packets()
        assert len(packets) == 1
        assert packets[0][2] == test_payload
    
    def test_collect_with_garbage_prefix(self):
        """Test collecting packet with garbage bytes before magic."""
        collector = PacketCollector()
        garbage = b'\x00\x01\x02\x03\xFF\xFE\xFD'
        pkt = build_packet(TYPE_CMD_CAPTURE, 5, u32le(100))
        
        collector.feed(garbage + pkt)
        packets = collector.get_packets()
        
        assert len(packets) == 1
        assert packets[0][0] == TYPE_CMD_CAPTURE
        assert packets[0][1] == 5
    
    def test_collect_with_partial_magic(self):
        """Test resync after partial magic sequence."""
        collector = PacketCollector()
        
        # Partial magic followed by real packet
        partial = b'\xC0\xDE'  # First 2 bytes of magic
        pkt = build_packet(TYPE_CMD_FOCUS, 0, b'')
        
        collector.feed(partial + pkt)
        packets = collector.get_packets()
        
        assert len(packets) == 1
        assert packets[0][0] == TYPE_CMD_FOCUS
    
    def test_collect_invalid_version_discarded(self):
        """Test that packets with wrong version are discarded."""
        collector = PacketCollector()
        
        # Build packet manually with wrong version
        bad_pkt = MAGIC + bytes([99, TYPE_CMD_FOCUS]) + u32le(0) + u32le(0)
        good_pkt = build_packet(TYPE_CMD_FOCUS, 1, b'')
        
        collector.feed(bad_pkt + good_pkt)
        packets = collector.get_packets()
        
        # Should only get the good packet
        assert len(packets) == 1
        assert packets[0][1] == 1  # seq from good packet
    
    def test_collect_oversized_payload_discarded(self):
        """Test that packets with oversized payload length are discarded."""
        collector = PacketCollector()
        
        # Build packet manually with huge payload length
        bad_pkt = MAGIC + bytes([VERSION, TYPE_PREVIEW_JPEG]) + u32le(0) + u32le(10_000_000)
        good_pkt = build_packet(TYPE_CMD_FOCUS, 2, b'')
        
        collector.feed(bad_pkt + good_pkt)
        packets = collector.get_packets()
        
        # Should only get the good packet
        assert len(packets) == 1
        assert packets[0][1] == 2
    
    def test_get_packets_clears_buffer(self):
        """Test that get_packets() clears internal buffer."""
        collector = PacketCollector()
        pkt = build_packet(TYPE_CMD_FOCUS, 0, b'')
        
        collector.feed(pkt)
        packets1 = collector.get_packets()
        packets2 = collector.get_packets()
        
        assert len(packets1) == 1
        assert len(packets2) == 0


class TestJpegValidation:
    """Test JPEG validation function."""
    
    def test_validate_jpeg_valid(self):
        """Test validation of valid JPEG data."""
        # Minimal valid JPEG structure
        valid_jpeg = bytes([0xFF, 0xD8, 0x00, 0x00, 0xFF, 0xD9])
        assert validate_jpeg(valid_jpeg) is True
    
    def test_validate_jpeg_invalid_header(self):
        """Test validation fails for wrong header."""
        invalid = bytes([0x00, 0xD8, 0x00, 0x00, 0xFF, 0xD9])
        assert validate_jpeg(invalid) is False
    
    def test_validate_jpeg_invalid_footer(self):
        """Test validation fails for wrong footer."""
        invalid = bytes([0xFF, 0xD8, 0x00, 0x00, 0xFF, 0x00])
        assert validate_jpeg(invalid) is False
    
    def test_validate_jpeg_too_short(self):
        """Test validation fails for data too short."""
        assert validate_jpeg(b'\xFF\xD8') is False
        assert validate_jpeg(b'\xFF') is False
        assert validate_jpeg(b'') is False


class TestPacketParserIntegration:
    """Integration tests using PacketParser from pi_usb_sender.py"""
    
    @pytest.fixture
    def parser(self):
        """Get PacketParser from pi_usb_sender."""
        try:
            from pi_usb_sender import PacketParser
            return PacketParser()
        except ImportError:
            pytest.skip("pi_usb_sender.py not available")
    
    def test_parser_preview_jpeg(self, parser, sample_jpeg_bytes):
        """Test parsing PREVIEW_JPEG packet."""
        pkt = build_packet(TYPE_PREVIEW_JPEG, 100, sample_jpeg_bytes)
        
        packets = list(parser.feed(pkt))
        
        assert len(packets) == 1
        ptype, seq, payload = packets[0]
        assert ptype == TYPE_PREVIEW_JPEG
        assert seq == 100
        assert payload == sample_jpeg_bytes
    
    def test_parser_gallery_info_response(self, parser):
        """Test parsing RSP_GALLERY_INFO packet."""
        # count=5, size_mb=10
        payload = bytes([5, 0, 10, 0])
        pkt = build_packet(TYPE_RSP_GALLERY_INFO, 0, payload)
        
        packets = list(parser.feed(pkt))
        
        assert len(packets) == 1
        ptype, seq, payload = packets[0]
        assert ptype == TYPE_RSP_GALLERY_INFO
        
        count = payload[0] | (payload[1] << 8)
        size_mb = payload[2] | (payload[3] << 8)
        assert count == 5
        assert size_mb == 10
    
    def test_parser_byte_by_byte(self, parser):
        """Test parser handles byte-by-byte feeding."""
        payload = b'test'
        pkt = build_packet(TYPE_CMD_CAPTURE, 42, u32le(100))
        
        packets = []
        for byte in pkt:
            packets.extend(parser.feed(bytes([byte])))
        
        assert len(packets) == 1
        assert packets[0][0] == TYPE_CMD_CAPTURE


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
