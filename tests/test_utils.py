#!/usr/bin/env python3
"""test_utils.py

Shared test utilities for CrowPanel test suite.
"""

import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Protocol constants
MAGIC = b"\xC0\xDE\xCA\xFE"
VERSION = 1

TYPE_PREVIEW_JPEG = 0x01
TYPE_CMD_CAPTURE = 0x10
TYPE_ACK_CAPTURE = 0x11
TYPE_CMD_FOCUS = 0x20
TYPE_ACK_FOCUS = 0x21
TYPE_CMD_GET_GALLERY = 0x50
TYPE_RSP_GALLERY_INFO = 0x51
TYPE_CMD_GET_THUMB = 0x52
TYPE_RSP_THUMB = 0x53
TYPE_CMD_DELETE_IMAGE = 0x54
TYPE_ACK_DELETE = 0x55


def u32le(v: int) -> bytes:
    """Convert integer to 4-byte little-endian."""
    return int(v & 0xFFFFFFFF).to_bytes(4, "little")


def read_u32le(b: bytes) -> int:
    """Read 4-byte little-endian integer."""
    return int.from_bytes(b[:4], "little", signed=False)


def build_packet(ptype: int, seq: int, payload: bytes) -> bytes:
    """Build a complete protocol packet."""
    header = MAGIC + bytes([VERSION, ptype]) + u32le(seq) + u32le(len(payload))
    return header + payload


def validate_jpeg(data: bytes) -> bool:
    """Check if data is a valid JPEG (has correct header/footer)."""
    if len(data) < 4:
        return False
    return (data[0] == 0xFF and data[1] == 0xD8 and 
            data[-2] == 0xFF and data[-1] == 0xD9)


def create_test_jpeg(width: int = 80, height: int = 60, quality: int = 70) -> bytes:
    """Create a test JPEG image."""
    try:
        import cv2
        import numpy as np
        
        # Create gradient image for visual verification
        img = np.zeros((height, width, 3), dtype=np.uint8)
        for y in range(height):
            img[y, :, 0] = int(255 * y / height)  # Blue gradient
        for x in range(width):
            img[:, x, 2] = int(255 * x / width)  # Red gradient
        
        result, encoded = cv2.imencode(".jpg", img, [int(cv2.IMWRITE_JPEG_QUALITY), quality])
        if result:
            return encoded.tobytes()
    except ImportError:
        pass
    
    # Minimal fallback JPEG
    return bytes([0xFF, 0xD8, 0xFF, 0xD9])


def decode_jpeg_size(jpeg_data: bytes) -> tuple:
    """Extract width and height from JPEG data (without full decode)."""
    try:
        import cv2
        import numpy as np
        
        arr = np.frombuffer(jpeg_data, dtype=np.uint8)
        img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
        if img is not None:
            return (img.shape[1], img.shape[0])  # (width, height)
    except:
        pass
    return (0, 0)


class MockSerial:
    """Mock serial port for testing without hardware."""
    
    def __init__(self):
        self.rx_buffer = bytearray()
        self.tx_buffer = bytearray()
        self.is_open = True
    
    @property
    def in_waiting(self) -> int:
        return len(self.rx_buffer)
    
    def read(self, size: int = 1) -> bytes:
        data = bytes(self.rx_buffer[:size])
        self.rx_buffer = self.rx_buffer[size:]
        return data
    
    def write(self, data: bytes) -> int:
        self.tx_buffer.extend(data)
        return len(data)
    
    def flush(self):
        pass
    
    def reset_input_buffer(self):
        self.rx_buffer.clear()
    
    def reset_output_buffer(self):
        self.tx_buffer.clear()
    
    def close(self):
        self.is_open = False
    
    # Test helpers
    def inject_rx(self, data: bytes):
        """Inject data into RX buffer (simulates incoming data)."""
        self.rx_buffer.extend(data)
    
    def get_tx(self) -> bytes:
        """Get all transmitted data and clear buffer."""
        data = bytes(self.tx_buffer)
        self.tx_buffer.clear()
        return data


class PacketCollector:
    """Collect and parse packets from raw byte stream."""
    
    def __init__(self):
        self.packets = []
        self._buffer = bytearray()
        self._state = "FIND_MAGIC"
        self._magic_idx = 0
        self._header = bytearray(10)  # ver + type + seq + len
        self._header_idx = 0
        self._payload = bytearray()
        self._payload_len = 0
    
    def feed(self, data: bytes):
        """Feed raw bytes and extract complete packets."""
        for byte in data:
            if self._state == "FIND_MAGIC":
                if byte == MAGIC[self._magic_idx]:
                    self._magic_idx += 1
                    if self._magic_idx == 4:
                        self._state = "READ_HEADER"
                        self._header_idx = 0
                else:
                    self._magic_idx = 1 if byte == MAGIC[0] else 0
            
            elif self._state == "READ_HEADER":
                self._header[self._header_idx] = byte
                self._header_idx += 1
                if self._header_idx >= 10:
                    ver = self._header[0]
                    ptype = self._header[1]
                    seq = read_u32le(self._header[2:6])
                    self._payload_len = read_u32le(self._header[6:10])
                    
                    if ver != VERSION or self._payload_len > 500000:
                        # Invalid, reset
                        self._state = "FIND_MAGIC"
                        self._magic_idx = 0
                    elif self._payload_len == 0:
                        # No payload, packet complete
                        self.packets.append((ptype, seq, b''))
                        self._state = "FIND_MAGIC"
                        self._magic_idx = 0
                    else:
                        self._payload = bytearray()
                        self._state = "READ_PAYLOAD"
            
            elif self._state == "READ_PAYLOAD":
                self._payload.append(byte)
                if len(self._payload) >= self._payload_len:
                    ptype = self._header[1]
                    seq = read_u32le(self._header[2:6])
                    self.packets.append((ptype, seq, bytes(self._payload)))
                    self._state = "FIND_MAGIC"
                    self._magic_idx = 0
    
    def get_packets(self) -> list:
        """Get all collected packets and clear buffer."""
        pkts = self.packets.copy()
        self.packets.clear()
        return pkts


def format_packet_info(ptype: int, seq: int, payload: bytes) -> str:
    """Format packet info for logging."""
    type_names = {
        TYPE_PREVIEW_JPEG: "PREVIEW_JPEG",
        TYPE_CMD_CAPTURE: "CMD_CAPTURE",
        TYPE_ACK_CAPTURE: "ACK_CAPTURE",
        TYPE_CMD_FOCUS: "CMD_FOCUS",
        TYPE_ACK_FOCUS: "ACK_FOCUS",
        TYPE_CMD_GET_GALLERY: "CMD_GET_GALLERY",
        TYPE_RSP_GALLERY_INFO: "RSP_GALLERY_INFO",
        TYPE_CMD_GET_THUMB: "CMD_GET_THUMB",
        TYPE_RSP_THUMB: "RSP_THUMB",
        TYPE_CMD_DELETE_IMAGE: "CMD_DELETE_IMAGE",
        TYPE_ACK_DELETE: "ACK_DELETE",
    }
    name = type_names.get(ptype, f"UNKNOWN(0x{ptype:02X})")
    return f"{name} seq={seq} len={len(payload)}"
