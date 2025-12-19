#!/usr/bin/env python3
"""pi_usb_sender.py

USB Serial üzerinden JPEG frame gönderici
Raspberry Pi -> ESP32 (Type-C USB bağlantısı)

Protocol (framed, binary, resyncable):
    MAGIC(4) + VER(1) + TYPE(1) + SEQ(u32le) + LEN(u32le) + PAYLOAD

Types:
    0x01 PREVIEW_JPEG: payload is JPEG bytes
    0x10 CMD_CAPTURE : payload u32le base_seq (capture next seq > base_seq)
    0x11 ACK_CAPTURE : payload u32le captured_seq
"""

import serial
import serial.tools.list_ports
import time
import io
import os
from datetime import datetime
from PIL import Image
import cv2
import numpy as np

# Picamera2 kütüphanesi
from picamera2 import Picamera2


MAGIC = b"\xC0\xDE\xCA\xFE"
VERSION = 1

TYPE_PREVIEW_JPEG = 0x01
TYPE_CMD_CAPTURE = 0x10
TYPE_ACK_CAPTURE = 0x11
TYPE_CMD_FOCUS = 0x20
TYPE_ACK_FOCUS = 0x21
# Gallery commands
TYPE_CMD_GET_GALLERY = 0x50
TYPE_RSP_GALLERY_INFO = 0x51
TYPE_CMD_GET_THUMB = 0x52
TYPE_RSP_THUMB = 0x53
TYPE_CMD_DELETE_IMAGE = 0x54
TYPE_ACK_DELETE = 0x55


def u32le(v: int) -> bytes:
    return int(v & 0xFFFFFFFF).to_bytes(4, "little")


def read_u32le(b: bytes) -> int:
    return int.from_bytes(b, "little", signed=False)


def send_packet(ser: serial.Serial, ptype: int, seq: int, payload: bytes) -> None:
    header = MAGIC + bytes([VERSION, ptype]) + u32le(seq) + u32le(len(payload))
    ser.write(header)
    if payload:
        ser.write(payload)
    ser.flush()


class PacketParser:
    def __init__(self):
        self.state = "FIND_MAGIC"
        self.magic_idx = 0
        self.header_rest = bytearray(1 + 1 + 4 + 4)  # ver,type,seq,len
        self.header_idx = 0
        self.ver = 0
        self.ptype = 0
        self.seq = 0
        self.length = 0
        self.payload = bytearray()

    def feed(self, data: bytes):
        """Yield (ptype, seq, payload_bytes) packets."""
        for byte in data:
            if self.state == "FIND_MAGIC":
                if byte == MAGIC[self.magic_idx]:
                    self.magic_idx += 1
                    if self.magic_idx == 4:
                        self.state = "READ_HEADER"
                        self.header_idx = 0
                else:
                    self.magic_idx = 1 if byte == MAGIC[0] else 0
                continue

            if self.state == "READ_HEADER":
                self.header_rest[self.header_idx] = byte
                self.header_idx += 1
                if self.header_idx >= len(self.header_rest):
                    self.ver = self.header_rest[0]
                    self.ptype = self.header_rest[1]
                    self.seq = read_u32le(self.header_rest[2:6])
                    self.length = read_u32le(self.header_rest[6:10])
                    self.payload = bytearray()
                    if self.ver != VERSION or self.length > 5_000_000:
                        # invalid; resync
                        self.state = "FIND_MAGIC"
                        self.magic_idx = 0
                    else:
                        self.state = "READ_PAYLOAD"
                continue

            # READ_PAYLOAD
            self.payload.append(byte)
            if len(self.payload) >= self.length:
                pkt = (self.ptype, self.seq, bytes(self.payload))
                self.state = "FIND_MAGIC"
                self.magic_idx = 0
                yield pkt

# --- SERIAL AYARLARI ---
SERIAL_PORT = "/dev/ttyUSB0"
BAUD_RATE = 2000000
# 480x320 icin guvenli FPS (bant genisligi icin uygun)
# Test sonuclarina gore ~8KB/frame, 2Mbps = teorik 31 FPS
# Pratik olarak 12-15 FPS guvenli
TARGET_FPS = 12  # 12 FPS hedef

# 480x320 + orta kalite icin yeterli ust limit
MAX_FRAME_SIZE = 50000  # 50KB maksimum frame boyutu
CAPTURE_DIR = "captures"

# Create captures directory
if not os.path.exists(CAPTURE_DIR):
    os.makedirs(CAPTURE_DIR)


# --- GALLERY MANAGEMENT ---
def get_gallery_info() -> tuple:
    """Get gallery info: (image_count, total_size_mb)"""
    try:
        files = sorted(
            [f for f in os.listdir(CAPTURE_DIR) if f.endswith('.jpg')],
            key=lambda x: os.path.getmtime(os.path.join(CAPTURE_DIR, x)),
            reverse=True  # Newest first
        )
        total_size = sum(
            os.path.getsize(os.path.join(CAPTURE_DIR, f)) 
            for f in files
        )
        size_mb = int(total_size / (1024 * 1024))
        return (len(files), size_mb, files)
    except Exception as e:
        print(f"[GALLERY] Error getting info: {e}")
        return (0, 0, [])


def generate_thumbnail(filepath: str, width: int = 80, height: int = 60) -> bytes:
    """Generate a thumbnail JPEG for the given image."""
    try:
        img = cv2.imread(filepath)
        if img is None:
            raise Exception(f"Could not read image: {filepath}")
        
        # Resize to thumbnail
        thumb = cv2.resize(img, (width, height), interpolation=cv2.INTER_AREA)
        
        # Encode as JPEG with moderate quality
        result, jpeg_array = cv2.imencode(".jpg", thumb, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
        if not result:
            raise Exception("Thumbnail encoding failed")
        
        return jpeg_array.tobytes()
    except Exception as e:
        print(f"[GALLERY] Thumbnail generation error: {e}")
        return b''


def delete_image(index: int, files: list) -> bool:
    """Delete image at given index. Returns True on success."""
    try:
        if index < 0 or index >= len(files):
            print(f"[GALLERY] Invalid index {index}, have {len(files)} files")
            return False
        
        filepath = os.path.join(CAPTURE_DIR, files[index])
        if os.path.exists(filepath):
            os.remove(filepath)
            print(f"[GALLERY] Deleted: {filepath}")
            return True
        else:
            print(f"[GALLERY] File not found: {filepath}")
            return False
    except Exception as e:
        print(f"[GALLERY] Delete error: {e}")
        return False


# Cached gallery file list (updated on gallery info request)
gallery_files_cache = []


# --- KAMERA AYARLARI ---
# Preview icin 480x320'e geri don (2Mbps seriye uygun)
RES_WIDTH = 480
RES_HEIGHT = 320


def find_esp32_port():
    """ESP32 USB portunu otomatik bul"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "USB" in port.description or "ACM" in port.device or "ESP" in port.description:
            print(f"[INFO] Found potential ESP32 port: {port.device} - {port.description}")
            return port.device
        if port.vid == 0x303A:
            print(f"[INFO] Found Espressif device: {port.device}")
            return port.device
    
    print("[WARN] ESP32 port not auto-detected, using default")
    return SERIAL_PORT


def setup_camera():
    """Picamera2'yi IMX519 için yapılandırır"""
    try:
        picam2 = Picamera2()

        # Dual stream: main=4K for capture, lores=480x320 for preview stream
        # NOTE: Picamera2/libcamera requires lores to be YUV.
        config = picam2.create_video_configuration(
            main={"size": (3840, 2160), "format": "YUV420"},
            lores={"size": (RES_WIDTH, RES_HEIGHT), "format": "YUV420"},
        )
        picam2.configure(config)
        picam2.start()

        # Try to lock camera FPS near TARGET_FPS to reduce ISP load
        try:
            frame_us = int(1_000_000 / TARGET_FPS)
            picam2.set_controls({"FrameDurationLimits": (frame_us, frame_us)})
        except Exception as fps_err:
            print(f"[WARN] Could not set FrameDurationLimits: {fps_err}")
        
        # Autofocus ayarı - Continuous AF mode
        try:
            picam2.set_controls({"AfMode": 2, "AfRange": 0})
            print(f"[INFO] Camera started. Resolution: {RES_WIDTH}x{RES_HEIGHT}, Autofocus: CONTINUOUS")
        except Exception as af_err:
            print(f"[WARN] Autofocus not available: {af_err}")
            print(f"[INFO] Camera started. Resolution: {RES_WIDTH}x{RES_HEIGHT}")
        
        return picam2
    except Exception as e:
        print(f"[FATAL] Camera init failed: {e}")
        exit(1)


def trigger_autofocus(picam2) -> bool:
    """Trigger one-shot autofocus and wait for completion."""
    try:
        print("[FOCUS] Starting autofocus...")
        
        # For IMX519 with libcamera:
        # AfMode: 0=Manual, 1=Auto (one-shot), 2=Continuous
        # AfTrigger: 0=Start (when AfMode=1)
        # AfState: 0=Idle, 1=Scanning, 2=Focused, 3=Failed
        
        # First, switch to Auto mode and trigger
        picam2.set_controls({"AfMode": 1})
        time.sleep(0.1)
        picam2.set_controls({"AfTrigger": 0})
        
        # Wait for focus to complete (max 5 seconds)
        for i in range(50):
            time.sleep(0.1)
            try:
                metadata = picam2.capture_metadata()
                af_state = metadata.get("AfState", -1)
                lens_pos = metadata.get("LensPosition", 0)
                
                # Log every 10 iterations
                if i % 10 == 0:
                    print(f"[FOCUS] Checking... AfState={af_state}, LensPos={lens_pos:.2f}")
                
                if af_state == 2:  # Focused
                    print(f"[FOCUS] SUCCESS! Final lens position: {lens_pos:.2f}")
                    picam2.set_controls({"AfMode": 2})  # Back to continuous
                    return True
                elif af_state == 3:  # Failed - retry
                    print("[FOCUS] AF failed, retrying...")
                    picam2.set_controls({"AfTrigger": 0})
            except Exception as e:
                if i == 0:
                    print(f"[FOCUS] Metadata warning: {e}")
        
        print("[FOCUS] Timeout - AF did not complete")
        picam2.set_controls({"AfMode": 2})
        return False
        
    except Exception as e:
        print(f"[FOCUS] Error: {e}")
        try:
            picam2.set_controls({"AfMode": 2})
        except:
            pass
        return False


def connect_serial():
    """USB Serial bağlantısı kur"""
    port = find_esp32_port()
    
    print(f"[INFO] Connecting to {port} at {BAUD_RATE} baud...")
    
    ser = serial.Serial(
        port=port,
        baudrate=BAUD_RATE,
        timeout=2,
        write_timeout=2,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE
    )
    
    # Buffer'ları temizle
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    time.sleep(0.5)
    
    # ESP32'nin hazır olmasını bekle
    start_time = time.time()
    while time.time() - start_time < 3:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            print(f"[ESP32] {line}")
            if "READY" in line or "ESP32" in line:
                break
    
    print("[INFO] Serial connected!")
    return ser


def encode_preview_jpeg(frame_bgr: np.ndarray) -> bytes:
    """Encode 480x320 BGR preview into bounded-size JPEG."""
    quality = 70
    for _ in range(3):
        result, jpeg_array = cv2.imencode(".jpg", frame_bgr, [int(cv2.IMWRITE_JPEG_QUALITY), quality])
        if not result:
            raise Exception("JPEG encoding failed")
        jpeg_bytes = jpeg_array.tobytes()
        if len(jpeg_bytes) <= MAX_FRAME_SIZE:
            return jpeg_bytes
        quality = max(40, quality - 10)
    return jpeg_bytes


def save_hq_jpeg(frame_bgr_4k: np.ndarray) -> str:
    """Save a 4K BGR frame as high-quality JPEG and return filepath."""
    result, jpeg_array = cv2.imencode(".jpg", frame_bgr_4k, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
    if not result:
        raise Exception("HQ JPEG encoding failed")
    jpeg_bytes = jpeg_array.tobytes()
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    save_path = os.path.join(CAPTURE_DIR, f"hq_capture_{timestamp}.jpg")
    with open(save_path, "wb") as f:
        f.write(jpeg_bytes)
    print(f"\n[HQ CAPTURE] Saved: {save_path} ({len(jpeg_bytes)} bytes, {frame_bgr_4k.shape[1]}x{frame_bgr_4k.shape[0]})")
    return save_path


def send_preview_frame(ser: serial.Serial, seq: int, jpeg_bytes: bytes) -> bool:
    try:
        if len(jpeg_bytes) > MAX_FRAME_SIZE:
            print(f"\n[ERROR] Preview frame too large: {len(jpeg_bytes)} bytes (max: {MAX_FRAME_SIZE})")
            return False
        send_packet(ser, TYPE_PREVIEW_JPEG, seq, jpeg_bytes)
        return True
    except serial.SerialTimeoutException:
        print("\n[ERROR] Serial timeout during send")
        return False
    except Exception as e:
        print(f"\n[ERROR] Send failed: {e}")
        return False


def main():
    print("=" * 50)
    print("USB Serial JPEG Streamer (Fixed)")
    print("Raspberry Pi -> ESP32 (Type-C)")
    print("=" * 50)
    
    # Kamerayı başlat
    print("\n[INIT] Starting camera...")
    picam2 = setup_camera()
    time.sleep(2)  # Kamera ısınma süresi
    
    consecutive_errors = 0
    max_consecutive_errors = 5
    
    parser = PacketParser()
    pending_capture_base_seq = None

    while True:
        try:
            ser = connect_serial()
            frame_num = 0
            fps_samples = []
            
            print("\n[STREAM] Starting video stream...")
            print(f"[STREAM] Target FPS: {TARGET_FPS}, Max frame size: {MAX_FRAME_SIZE} bytes")
            print(f"[STREAM] Resolution: {RES_WIDTH}x{RES_HEIGHT}")
            
            while True:
                start_time = time.time()

                # Consume any inbound framed packets (commands)
                if ser.in_waiting > 0:
                    try:
                        data = ser.read(min(ser.in_waiting, 4096))
                        for ptype, seq, payload in parser.feed(data):
                            if ptype == TYPE_CMD_CAPTURE and len(payload) == 4:
                                base = read_u32le(payload)
                                pending_capture_base_seq = base
                                print(f"\n[COMMAND] CAPTURE requested (base_seq={base})")
                            elif ptype == TYPE_CMD_FOCUS:
                                print(f"\n[COMMAND] FOCUS requested")
                                success = trigger_autofocus(picam2)
                                send_packet(ser, TYPE_ACK_FOCUS, 0, bytes([0 if success else 1]))
                            elif ptype == TYPE_CMD_GET_GALLERY:
                                print(f"\n[COMMAND] GET_GALLERY requested")
                                global gallery_files_cache
                                count, size_mb, files = get_gallery_info()
                                gallery_files_cache = files
                                # Send response: count(u16) + size_mb(u16)
                                resp = bytes([count & 0xFF, (count >> 8) & 0xFF,
                                             size_mb & 0xFF, (size_mb >> 8) & 0xFF])
                                send_packet(ser, TYPE_RSP_GALLERY_INFO, 0, resp)
                                print(f"[GALLERY] Sent info: {count} images, {size_mb} MB")
                            elif ptype == TYPE_CMD_GET_THUMB and len(payload) == 2:
                                index = payload[0] | (payload[1] << 8)
                                print(f"\n[COMMAND] GET_THUMB #{index}")
                                if index < len(gallery_files_cache):
                                    filepath = os.path.join(CAPTURE_DIR, gallery_files_cache[index])
                                    thumb_jpeg = generate_thumbnail(filepath)
                                    if thumb_jpeg:
                                        # Send response: index(u16) + jpeg_data
                                        resp = bytes([index & 0xFF, (index >> 8) & 0xFF]) + thumb_jpeg
                                        send_packet(ser, TYPE_RSP_THUMB, index, resp)
                                        print(f"[GALLERY] Sent thumbnail #{index} ({len(thumb_jpeg)} bytes)")
                                    else:
                                        print(f"[GALLERY] Failed to generate thumbnail #{index}")
                                else:
                                    print(f"[GALLERY] Invalid thumbnail index {index}")
                            elif ptype == TYPE_CMD_DELETE_IMAGE and len(payload) == 2:
                                index = payload[0] | (payload[1] << 8)
                                print(f"\n[COMMAND] DELETE_IMAGE #{index}")
                                success = delete_image(index, gallery_files_cache)
                                send_packet(ser, TYPE_ACK_DELETE, 0, bytes([0 if success else 1]))
                    except Exception as cmd_err:
                        print(f"\n[WARN] Command processing error: {cmd_err}")

                try:
                    # Get one request so preview + 4K capture can come from the same frame
                    request = picam2.capture_request()
                    try:
                        frame_lores = request.make_array("lores")
                        if frame_lores is None:
                            raise Exception("lores frame is None")
                        
                        # YUV420 planar buffer with stride padding
                        # Picamera2 uses stride (e.g., 512) which may be larger than width (480)
                        # Buffer shape: (height * 1.5, stride) for I420
                        # We need to crop out the padding before conversion
                        
                        h_buf, w_buf = frame_lores.shape[:2]
                        
                        if frame_lores.ndim == 2:
                            # Planar YUV420 (I420): Y plane + U plane + V plane
                            # Total height = actual_height * 1.5
                            # actual_height = h_buf * 2 / 3
                            actual_h = int(h_buf * 2 // 3)  # 320
                            stride = w_buf  # 512
                            actual_w = RES_WIDTH  # 480
                            
                            # Crop stride padding from each plane
                            # Y plane: rows 0 to actual_h, cols 0 to actual_w
                            y_plane = frame_lores[0:actual_h, 0:actual_w]
                            
                            # U plane: rows actual_h to actual_h + actual_h/4
                            # U is (actual_h/2) x (stride/2), we need (actual_h/2) x (actual_w/2)
                            u_start = actual_h
                            u_h = actual_h // 2  # 160
                            u_plane_full = frame_lores[u_start:u_start + u_h, :]
                            # U plane has stride/2 width in the buffer, crop to actual_w/2
                            u_plane = u_plane_full[:, 0:actual_w // 2]
                            
                            # V plane: rows actual_h + actual_h/4 to end
                            v_start = actual_h + u_h
                            v_plane_full = frame_lores[v_start:v_start + u_h, :]
                            v_plane = v_plane_full[:, 0:actual_w // 2]
                            
                            # Reconstruct I420 buffer without stride padding
                            # New buffer: (actual_h * 1.5, actual_w)
                            new_h = actual_h + actual_h // 2  # 480
                            cropped_yuv = np.zeros((new_h, actual_w), dtype=np.uint8)
                            cropped_yuv[0:actual_h, :] = y_plane
                            cropped_yuv[actual_h:actual_h + u_h, 0:actual_w//2] = u_plane
                            cropped_yuv[actual_h + u_h:, 0:actual_w//2] = v_plane
                            
                            # Now convert - but I420 UV planes are interleaved differently
                            # Actually for I420, U and V are full-width in the planar layout
                            # Let's use a simpler approach: convert with stride, then crop result
                            frame_lores_bgr_raw = cv2.cvtColor(frame_lores, cv2.COLOR_YUV2BGR_I420)
                            # Result is (actual_h, stride, 3) - crop width
                            frame_lores_bgr = frame_lores_bgr_raw[:, 0:actual_w, :]
                            
                        elif frame_lores.ndim == 3 and frame_lores.shape[2] == 3:
                            # Already 3-channel (RGB/BGR)
                            frame_lores_bgr = cv2.cvtColor(frame_lores, cv2.COLOR_RGB2BGR)
                            # Crop if needed
                            if frame_lores_bgr.shape[1] > RES_WIDTH:
                                frame_lores_bgr = frame_lores_bgr[:, 0:RES_WIDTH, :]
                        else:
                            raise Exception(f"Unknown lores format: shape={frame_lores.shape}, ndim={frame_lores.ndim}")
                        
                        # Final size check (should be 480x320 now)
                        out_h, out_w = frame_lores_bgr.shape[:2]
                        if (out_w, out_h) != (RES_WIDTH, RES_HEIGHT):
                            frame_lores_bgr = cv2.resize(frame_lores_bgr, (RES_WIDTH, RES_HEIGHT))

                        jpeg_bytes = encode_preview_jpeg(frame_lores_bgr)

                        frame_num += 1
                        seq = frame_num

                        # If a capture is pending, capture on the next seq strictly greater than base
                        if pending_capture_base_seq is not None and seq > pending_capture_base_seq:
                            frame_main = request.make_array("main")
                            if frame_main is None:
                                raise Exception("main frame is None")
                            # main is YUV420 with stride; convert and crop
                            if frame_main.ndim == 2:
                                h_main, w_main = frame_main.shape[:2]
                                actual_h_main = int(h_main * 2 // 3)  # 2160 for 4K
                                frame_main_bgr_raw = cv2.cvtColor(frame_main, cv2.COLOR_YUV2BGR_I420)
                                # Crop stride padding (3840 from possibly larger stride)
                                actual_w_main = 3840
                                if frame_main_bgr_raw.shape[1] > actual_w_main:
                                    frame_main_bgr = frame_main_bgr_raw[:, 0:actual_w_main, :]
                                else:
                                    frame_main_bgr = frame_main_bgr_raw
                            else:
                                frame_main_bgr = cv2.cvtColor(frame_main, cv2.COLOR_RGB2BGR)
                            save_hq_jpeg(frame_main_bgr)
                            send_packet(ser, TYPE_ACK_CAPTURE, 0, u32le(seq))
                            pending_capture_base_seq = None

                        if send_preview_frame(ser, seq, jpeg_bytes):
                            consecutive_errors = 0
                            elapsed = time.time() - start_time
                            fps = 1.0 / elapsed if elapsed > 0 else 0
                            fps_samples.append(fps)
                            if len(fps_samples) > 30:
                                fps_samples.pop(0)
                            avg_fps = sum(fps_samples) / len(fps_samples)
                            print(
                                f"\r[STREAM] Seq {seq} | Size: {len(jpeg_bytes):,} bytes | FPS: {fps:.1f} (avg: {avg_fps:.1f})     ",
                                end="",
                            )

                            # Frame rate limit
                            frame_time = time.time() - start_time
                            target_frame_time = 1.0 / TARGET_FPS
                            if frame_time < target_frame_time:
                                time.sleep(target_frame_time - frame_time)
                        else:
                            consecutive_errors += 1
                            print(f"\n[ERROR] Frame failed ({consecutive_errors}/{max_consecutive_errors})")
                    finally:
                        request.release()

                    if consecutive_errors >= max_consecutive_errors:
                        print("\n[ERROR] Too many consecutive errors, reconnecting...")
                        ser.close()
                        time.sleep(2)
                        break

                except Exception as e:
                    print(f"\n[ERROR] Frame capture/send error: {e}")
                    consecutive_errors += 1
                    if consecutive_errors >= max_consecutive_errors:
                        break
                    time.sleep(0.1)
                    
        except KeyboardInterrupt:
            print("\n\n[EXIT] Stopping...")
            try:
                picam2.stop()
                ser.close()
            except:
                pass
            break
        except serial.SerialException as e:
            print(f"\n[ERROR] Serial error: {e}")
            time.sleep(2)
        except Exception as e:
            print(f"\n[ERROR] {e}")
            time.sleep(2)


if __name__ == "__main__":
    main()