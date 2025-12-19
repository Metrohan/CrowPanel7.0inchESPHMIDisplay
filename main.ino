#include <Wire.h>
#include <SPI.h>
#include <lvgl.h>
#include "gfx_conf.h"
#include "ui.h"
#include <TJpg_Decoder.h>
#include <SPIFFS.h>
#include "ilablogo.c"

#define BAUD_RATE 2000000
#define BACKLIGHT_PIN 38

// JPEG buffer - PSRAM'da
#define MAX_JPEG_SIZE (120 * 1024)  // Upper bound for JPEG size (kept at 120KB)
// Artık Pi tarafı doğru RGB JPEG gönderiyor, bu yüzden R/B swap KAPALI
#define CAMERA_SWAP_RB 0  // 1=Swap R/B channels, 0=Direct copy (we want direct)
uint8_t* jpeg_buffer = nullptr;
volatile bool frame_ready = false;
volatile bool processing_frame = false;
size_t jpeg_size = 0;

// Frame skip sayacı
uint32_t frame_skip_counter = 0;
const uint32_t MAX_SKIP_FRAMES = 5;

// Capture state - DEBOUNCE REMOVED for immediate capture
bool capture_requested = false;
bool capture_immediate = false;  // New: capture next frame immediately
uint32_t capture_counter = 0;
bool last_frame_valid = false;  // Track if last decode was successful

// Non-blocking captures
uint8_t* capture_buffer_static = nullptr;
size_t capture_size = 0;
volatile bool capture_write_pending = false;
size_t capture_write_offset = 0;
File capture_file;
uint32_t capture_start_ms = 0;
const size_t CAPTURE_CHUNK_SIZE = 4096;
// Removed: static uint32_t last_capture_ms = 0;  // No longer needed - debounce removed

// UI frame counter
uint32_t ui_total_frames_counter = 0;

// LVGL image descriptor
lv_img_dsc_t jpeg_img_dsc;

// RGB565 buffer - 480x320 için PSRAM (Pi sender ile uyumlu)
#define DECODE_WIDTH 480
#define DECODE_HEIGHT 320
#define DECODE_BUFFER_SIZE (DECODE_WIDTH * DECODE_HEIGHT * 2)
static uint8_t* decoded_rgb565_static = nullptr;
size_t decoded_width = 0;
size_t decoded_height = 0;

// ---------- LVGL / Display ----------
LGFX tft;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// ---------- Serial Protocol ----------
namespace Proto {
    static constexpr uint8_t MAGIC[4] = {0xC0, 0xDE, 0xCA, 0xFE};
    static constexpr uint8_t VERSION = 1;

    enum Type : uint8_t {
        PREVIEW_JPEG = 0x01,
        CMD_CAPTURE  = 0x10,
        ACK_CAPTURE  = 0x11,
        CMD_FOCUS    = 0x20,
        ACK_FOCUS    = 0x21,
        // Gallery commands
        CMD_GET_GALLERY   = 0x50,  // Request gallery info
        RSP_GALLERY_INFO  = 0x51,  // Response: count(u16) + total_size_mb(u16)
        CMD_GET_THUMB     = 0x52,  // Request thumbnail: index(u16)
        RSP_THUMB         = 0x53,  // Response: index(u16) + jpeg_data
        CMD_DELETE_IMAGE  = 0x54,  // Delete image: index(u16)
        ACK_DELETE        = 0x55,  // Delete result: status(u8)
    };

    static constexpr size_t HEADER_SIZE = 4 /*magic*/ + 1 /*ver*/ + 1 /*type*/ + 4 /*seq*/ + 4 /*len*/;
}

static uint32_t last_preview_seq_rx = 0;
static uint32_t last_preview_seq_displayed = 0;
static uint32_t last_capture_ack_seq = 0;

// ---------- Gallery Data ----------
#define GALLERY_THUMB_W 80
#define GALLERY_THUMB_H 60
#define GALLERY_MAX_THUMBS 8
#define GALLERY_THUMB_BUF_SIZE (GALLERY_THUMB_W * GALLERY_THUMB_H * 2)  // RGB565

uint16_t gallery_image_count = 0;
uint16_t gallery_total_size_mb = 0;
uint8_t gallery_current_page = 0;
static bool gallery_info_received = false;
static bool gallery_thumb_loading = false;
static uint8_t gallery_thumb_index = 0;

// Thumbnail buffers in PSRAM (8 slots x 80x60 RGB565)
static uint8_t* gallery_thumb_buffers[GALLERY_MAX_THUMBS] = {nullptr};
bool gallery_thumb_valid[GALLERY_MAX_THUMBS] = {false};
lv_img_dsc_t gallery_thumb_dsc[GALLERY_MAX_THUMBS];

// Decoded thumbnail buffer (for JPEG decode)
static uint8_t* gallery_thumb_decode_buf = nullptr;

static enum {
    RX_FIND_MAGIC,
    RX_READ_HEADER_REST,
    RX_READ_PAYLOAD,
} rx_state = RX_FIND_MAGIC;

static uint8_t magic_idx = 0;
static uint8_t header_rest[Proto::HEADER_SIZE - 4];
static size_t header_rest_idx = 0;
static uint8_t rx_type = 0;
static uint32_t rx_seq = 0;
static uint32_t rx_len = 0;
static uint32_t payload_received = 0;
static bool discard_payload = false;
static uint8_t small_payload[16];
static uint32_t stat_frames_rx = 0;
static uint32_t stat_frames_shown = 0;
static uint32_t stat_frames_dropped_busy = 0;
static unsigned long stat_last_log_ms = 0;

static inline uint32_t read_u32_le(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void write_u32_le(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void proto_reset_to_find_magic() {
    rx_state = RX_FIND_MAGIC;
    magic_idx = 0;
    header_rest_idx = 0;
    rx_type = 0;
    rx_seq = 0;
    rx_len = 0;
    payload_received = 0;
    discard_payload = false;
}

static void on_capture_ack(uint32_t captured_seq) {
    last_capture_ack_seq = captured_seq;
    if(ui_response_label) {
        char msg[64];
        snprintf(msg, sizeof(msg), "4K FOTO CEKILDI (seq=%lu)", (unsigned long)captured_seq);
        lv_label_set_text(ui_response_label, msg);
        lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x4CAF50), 0);
    }
}

static void on_focus_ack(uint8_t status) {
    if(ui_response_label) {
        if(status == 0) {
            lv_label_set_text(ui_response_label, "Odaklandi!");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x4CAF50), 0);  // Green
        } else {
            lv_label_set_text(ui_response_label, "Odaklama Basarisiz");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFF5722), 0);  // Orange
        }
    }
}

void send_focus_request() {
    uint8_t pkt[Proto::HEADER_SIZE];

    memcpy(pkt, Proto::MAGIC, 4);
    pkt[4] = Proto::VERSION;
    pkt[5] = Proto::CMD_FOCUS;
    write_u32_le(&pkt[6], 0);  // seq (not used)
    write_u32_le(&pkt[10], 0); // len = 0 (no payload)

    Serial.write(pkt, sizeof(pkt));
    Serial.flush();
}

void send_capture_request() {
    // Request capture aligned to the next preview frame after the currently displayed one.
    uint32_t base_seq = last_preview_seq_displayed;
    uint8_t pkt[Proto::HEADER_SIZE + 4];

    memcpy(pkt, Proto::MAGIC, 4);
    pkt[4] = Proto::VERSION;
    pkt[5] = Proto::CMD_CAPTURE;
    write_u32_le(&pkt[6], base_seq);
    write_u32_le(&pkt[10], 4);
    write_u32_le(&pkt[14], base_seq);

    Serial.write(pkt, sizeof(pkt));
    Serial.flush();
}

// ---------- Gallery Functions ----------
void send_gallery_request() {
    uint8_t pkt[Proto::HEADER_SIZE];
    memcpy(pkt, Proto::MAGIC, 4);
    pkt[4] = Proto::VERSION;
    pkt[5] = Proto::CMD_GET_GALLERY;
    write_u32_le(&pkt[6], 0);
    write_u32_le(&pkt[10], 0);
    Serial.write(pkt, sizeof(pkt));
    Serial.flush();
    Serial.println("[GALLERY] Requested gallery info");
}

void send_thumb_request(uint16_t index) {
    uint8_t pkt[Proto::HEADER_SIZE + 2];
    memcpy(pkt, Proto::MAGIC, 4);
    pkt[4] = Proto::VERSION;
    pkt[5] = Proto::CMD_GET_THUMB;
    write_u32_le(&pkt[6], index);
    write_u32_le(&pkt[10], 2);
    pkt[14] = (uint8_t)(index & 0xFF);
    pkt[15] = (uint8_t)((index >> 8) & 0xFF);
    Serial.write(pkt, sizeof(pkt));
    Serial.flush();
    gallery_thumb_loading = true;
    gallery_thumb_index = index % GALLERY_MAX_THUMBS;
    Serial.printf("[GALLERY] Requested thumbnail #%d\n", index);
}

void send_delete_request(uint16_t index) {
    uint8_t pkt[Proto::HEADER_SIZE + 2];
    memcpy(pkt, Proto::MAGIC, 4);
    pkt[4] = Proto::VERSION;
    pkt[5] = Proto::CMD_DELETE_IMAGE;
    write_u32_le(&pkt[6], index);
    write_u32_le(&pkt[10], 2);
    pkt[14] = (uint8_t)(index & 0xFF);
    pkt[15] = (uint8_t)((index >> 8) & 0xFF);
    Serial.write(pkt, sizeof(pkt));
    Serial.flush();
    Serial.printf("[GALLERY] Requested delete image #%d\n", index);
}

// Forward declarations for UI update functions (defined in ui.h)
extern void update_gallery_info_ui(uint16_t count, uint16_t size_mb);
extern void update_gallery_thumb_ui(uint8_t slot);
extern void on_delete_result(uint8_t status);

static void on_gallery_info(uint16_t count, uint16_t size_mb) {
    gallery_image_count = count;
    gallery_total_size_mb = size_mb;
    gallery_info_received = true;
    Serial.printf("[GALLERY] Info received: %d images, %d MB\n", count, size_mb);
    update_gallery_info_ui(count, size_mb);
}

// TJpg Decoder callback for thumbnail (80x60)
static uint8_t* thumb_decode_target = nullptr;
static uint16_t thumb_decode_w = 0;
static uint16_t thumb_decode_h = 0;

bool thumb_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if(!thumb_decode_target || x < 0 || y < 0) return true;
    for (int row = 0; row < h; row++) {
        uint32_t dest_y = y + row;
        if(dest_y >= thumb_decode_h) break;
        uint32_t offset = (dest_y * thumb_decode_w + x);
        if((offset + w) <= (thumb_decode_w * thumb_decode_h)) {
            uint16_t* dest = (uint16_t*)(thumb_decode_target + offset * 2);
            memcpy(dest, bitmap + (row * w), w * sizeof(uint16_t));
        }
    }
    return true;
}

static void on_thumbnail_received(uint16_t index, const uint8_t* jpeg_data, size_t jpeg_len) {
    // Use slot = index % 8 for circular buffer
    uint8_t slot = index % GALLERY_MAX_THUMBS;
    
    if(!gallery_thumb_buffers[slot]) {
        gallery_thumb_buffers[slot] = (uint8_t*)ps_malloc(GALLERY_THUMB_BUF_SIZE);
        if(!gallery_thumb_buffers[slot]) {
            Serial.printf("[GALLERY] Failed to allocate thumb buffer %d\n", slot);
            return;
        }
    }
    
    // Decode JPEG to RGB565
    thumb_decode_target = gallery_thumb_buffers[slot];
    thumb_decode_w = GALLERY_THUMB_W;
    thumb_decode_h = GALLERY_THUMB_H;
    
    // Use TJpgDec
    TJpgDec.setCallback(thumb_output);
    TJpgDec.setJpgScale(1);
    
    JRESULT res = TJpgDec.drawJpg(0, 0, jpeg_data, jpeg_len);
    
    // Restore main callback
    TJpgDec.setCallback(tft_output);
    
    if(res == JDR_OK) {
        gallery_thumb_valid[slot] = true;
        
        // Setup LVGL image descriptor
        gallery_thumb_dsc[slot].header.cf = LV_IMG_CF_TRUE_COLOR;
        gallery_thumb_dsc[slot].header.always_zero = 0;
        gallery_thumb_dsc[slot].header.reserved = 0;
        gallery_thumb_dsc[slot].header.w = GALLERY_THUMB_W;
        gallery_thumb_dsc[slot].header.h = GALLERY_THUMB_H;
        gallery_thumb_dsc[slot].data_size = GALLERY_THUMB_BUF_SIZE;
        gallery_thumb_dsc[slot].data = gallery_thumb_buffers[slot];
        
        Serial.printf("[GALLERY] Thumbnail #%d decoded to slot %d\n", index, slot);
        
        // Update UI - pass the visible position (0-3) for current page
        uint8_t page_start = gallery_current_page * 4;
        if(index >= page_start && index < page_start + 4) {
            uint8_t visible_pos = index - page_start;
            update_gallery_thumb_ui(visible_pos);
        }
    } else {
        Serial.printf("[GALLERY] Thumbnail decode failed: %d\n", res);
    }
    
    gallery_thumb_loading = false;
}

static void on_delete_ack(uint8_t status) {
    if(status == 0) {
        Serial.println("[GALLERY] Image deleted successfully");
        // Request updated gallery info
        send_gallery_request();
    } else {
        Serial.println("[GALLERY] Image delete failed");
    }
    on_delete_result(status);
}

// ---------- TJpg Decoder Callback (RGB565, no swap) ----------
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (decoded_rgb565_static && x >= 0 && y >= 0 && w > 0 && h > 0) {
    for (int row = 0; row < h; row++) {
      uint32_t dest_y = y + row;
      if(dest_y >= decoded_height) break;
      
      uint32_t offset = (dest_y * decoded_width + x);
      uint32_t max_offset = decoded_width * decoded_height;
      
      if (offset < max_offset && (offset + w) <= max_offset) {
        uint16_t* dest = (uint16_t*)(decoded_rgb565_static + offset * 2);
#if CAMERA_SWAP_RB
        // Swap red and blue channels (RGB565: RRRRRGGG GGGBBBBB -> BBBBBGGG GGGRRRRR)
        const uint16_t* src = bitmap + (row * w);
        for (int col = 0; col < w; col++) {
          uint16_t p = src[col];
          // Keep green (bits 5-10), swap red (11-15) with blue (0-4)
          p = (p & 0x07E0) | ((p & 0x001F) << 11) | ((p & 0xF800) >> 11);
          dest[col] = p;
        }
#else
        // Direct copy without channel swap
        memcpy(dest, bitmap + (row * w), w * sizeof(uint16_t));
#endif
      }
    }
  }
  return true;
}

// ---------- LVGL / Display Functions ----------
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    tft.pushImageDMA(area->x1, area->y1,
                     area->x2 - area->x1 + 1, area->y2 - area->y1 + 1,
                     (lgfx::rgb565_t *)color_p);
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    uint16_t x, y;
    bool touched = tft.getTouch(&x, &y);
    if(touched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void set_brightness(int value) {
    value = constrain(value, 0, 255);
    analogWrite(BACKLIGHT_PIN, value);
}

void slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int brightness_val = (int)lv_slider_get_value(slider);
    set_brightness(brightness_val);
}

// ---------- Process USB Serial Frames (Framed Protocol, magic-resync) ----------
void process_serial_stream() {
    static unsigned long last_progress_ms = 0;

    // Cap work per loop so LVGL stays responsive
    size_t budget = 4096;
    uint8_t dump[64];

    while(budget > 0 && Serial.available() > 0) {
        // Bulk-read payload when we know length
        if(rx_state == RX_READ_PAYLOAD) {
            size_t remaining = (rx_len > payload_received) ? (rx_len - payload_received) : 0;
            if(remaining == 0) {
                proto_reset_to_find_magic();
                continue;
            }

            size_t avail = (size_t)Serial.available();
            size_t to_read = min(remaining, min(avail, budget));
            if(to_read == 0) break;
            last_progress_ms = millis();

            if(discard_payload) {
                // Discard quickly
                while(to_read > 0) {
                    size_t chunk = min(to_read, sizeof(dump));
                    size_t n = Serial.readBytes(dump, chunk);
                    if(n == 0) break;
                    payload_received += n;
                    budget -= n;
                    to_read -= n;
                }
            } else if(rx_type == Proto::PREVIEW_JPEG || rx_type == Proto::RSP_THUMB) {
                // Large payloads go to jpeg_buffer
                size_t n = Serial.readBytes(jpeg_buffer + payload_received, to_read);
                if(n == 0) break;
                payload_received += n;
                budget -= n;
            } else {
                // Small payload (e.g. ACK)
                while(to_read > 0) {
                    size_t space = (payload_received < sizeof(small_payload)) ? (sizeof(small_payload) - payload_received) : 0;
                    if(space > 0) {
                        size_t chunk = min(to_read, space);
                        size_t n = Serial.readBytes(small_payload + payload_received, chunk);
                        if(n == 0) break;
                        payload_received += n;
                        budget -= n;
                        to_read -= n;
                    } else {
                        size_t chunk = min(to_read, sizeof(dump));
                        size_t n = Serial.readBytes(dump, chunk);
                        if(n == 0) break;
                        payload_received += n;
                        budget -= n;
                        to_read -= n;
                    }
                }
            }

            if(payload_received >= rx_len) {
                if(!discard_payload) {
                    if(rx_type == Proto::PREVIEW_JPEG) {
                        frame_skip_counter = 0;
                        frame_ready = true;
                        stat_frames_rx++;
                    } else if(rx_type == Proto::ACK_CAPTURE && rx_len == 4) {
                        uint32_t captured_seq = read_u32_le(&small_payload[0]);
                        on_capture_ack(captured_seq);
                    } else if(rx_type == Proto::ACK_FOCUS && rx_len == 1) {
                        on_focus_ack(small_payload[0]);
                    } else if(rx_type == Proto::RSP_GALLERY_INFO && rx_len == 4) {
                        uint16_t count = (uint16_t)(small_payload[0] | (small_payload[1] << 8));
                        uint16_t size_mb = (uint16_t)(small_payload[2] | (small_payload[3] << 8));
                        on_gallery_info(count, size_mb);
                    } else if(rx_type == Proto::RSP_THUMB && rx_len > 2) {
                        // RSP_THUMB payload: index(u16) + jpeg_data
                        // Payload is in jpeg_buffer (large payload handler)
                        uint16_t index = (uint16_t)(jpeg_buffer[0] | (jpeg_buffer[1] << 8));
                        on_thumbnail_received(index, jpeg_buffer + 2, rx_len - 2);
                    } else if(rx_type == Proto::ACK_DELETE && rx_len == 1) {
                        on_delete_ack(small_payload[0]);
                    }
                }
                proto_reset_to_find_magic();
            }
            continue;
        }

        // Header/magic scanning is byte-oriented (small)
        int b = Serial.read();
        if(b < 0) break;
        budget--;
        uint8_t byte = (uint8_t)b;

        if(rx_state == RX_FIND_MAGIC) {
            if(byte == Proto::MAGIC[magic_idx]) {
                magic_idx++;
                if(magic_idx == 4) {
                    rx_state = RX_READ_HEADER_REST;
                    header_rest_idx = 0;
                    last_progress_ms = millis();
                }
            } else {
                magic_idx = (byte == Proto::MAGIC[0]) ? 1 : 0;
            }
            continue;
        }

        if(rx_state == RX_READ_HEADER_REST) {
            header_rest[header_rest_idx++] = byte;
            if(header_rest_idx >= sizeof(header_rest)) {
                uint8_t ver = header_rest[0];
                rx_type = header_rest[1];
                rx_seq = read_u32_le(&header_rest[2]);
                rx_len = read_u32_le(&header_rest[6]);

                bool ok = (ver == Proto::VERSION);
                if(ok) {
                    if(rx_type == Proto::PREVIEW_JPEG) {
                        ok = (rx_len > 0 && rx_len <= MAX_JPEG_SIZE);
                    } else if(rx_type == Proto::ACK_CAPTURE) {
                        ok = (rx_len == 4);
                    } else {
                        ok = (rx_len <= MAX_JPEG_SIZE);
                    }
                }

                payload_received = 0;
                discard_payload = !ok;
                if(ok && rx_type == Proto::PREVIEW_JPEG) {
                    if(processing_frame || frame_ready) {
                        discard_payload = true;
                        stat_frames_dropped_busy++;
                    } else {
                        jpeg_size = rx_len;
                        last_preview_seq_rx = rx_seq;
                    }
                }

                if(!ok) {
                    proto_reset_to_find_magic();
                    continue;
                }

                rx_state = RX_READ_PAYLOAD;
                last_progress_ms = millis();
            }
            continue;
        }
    }

    // Timeout safety (mid-packet)
    if(rx_state != RX_FIND_MAGIC && (millis() - last_progress_ms) > 1000) {
        proto_reset_to_find_magic();
    }
}

// ---------- Setup ----------
void setup() {
    Serial.setRxBufferSize(16384);
    Serial.begin(2000000);
    delay(500);
    Serial.println("ESP32_READY");

    // PSRAM kontrolü
    if(!psramFound()) {
        Serial.println("[ERROR] PSRAM Required!");
        while(1) delay(1000);
    }
    
    // PSRAM'dan buffer ayır
    decoded_rgb565_static = (uint8_t*)ps_malloc(DECODE_BUFFER_SIZE);
    jpeg_buffer = (uint8_t*)ps_malloc(MAX_JPEG_SIZE);
    capture_buffer_static = (uint8_t*)ps_malloc(MAX_JPEG_SIZE);
    
    if(!decoded_rgb565_static || !jpeg_buffer || !capture_buffer_static) {
        Serial.println("[ERROR] PSRAM Allocation Failed!");
        while(1) delay(1000);
    }
    
    Serial.printf("[INFO] Buffers in PSRAM - %dx%d mode (JPEG:%dKB, RGB:%dKB, Capture:%dKB)\n", 
                  DECODE_WIDTH, DECODE_HEIGHT, 
                  MAX_JPEG_SIZE/1024, DECODE_BUFFER_SIZE/1024, MAX_JPEG_SIZE/1024);

    // SPIFFS
    if(!SPIFFS.begin(true)) {
        Serial.println("[ERROR] SPIFFS Failed");
    }

    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    pinMode(BACKLIGHT_PIN, OUTPUT);
    set_brightness(200);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();

    // TJpg_Decoder setup
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    TJpgDec.setSwapBytes(false);

    // Detailed diagnostics at startup
    Serial.println("\n========================================");
    Serial.println("ESP32 CrowPanel Diagnostic Info");
    Serial.println("========================================");
    Serial.printf("  Decode resolution: %dx%d\n", DECODE_WIDTH, DECODE_HEIGHT);
    Serial.printf("  Max JPEG size: %d KB\n", MAX_JPEG_SIZE / 1024);
    Serial.printf("  RGB565 buffer: %d KB\n", DECODE_BUFFER_SIZE / 1024);
    Serial.printf("  Serial baud: %d\n", BAUD_RATE);
    Serial.printf("  PSRAM total: %d KB\n", ESP.getPsramSize() / 1024);
    Serial.printf("  PSRAM free: %d KB\n", ESP.getFreePsram() / 1024);
    Serial.printf("  Heap free: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.println("========================================");
    Serial.println("Protocol: MAGIC(C0DECAFE) + VER(1) + TYPE + SEQ + LEN + PAYLOAD");
    Serial.println("Expected: TYPE=0x01 (PREVIEW_JPEG), Size 480x320");
    Serial.println("========================================\n");
    Serial.println("[READY] Waiting for frames...");
}

// ---------- Loop ----------
void loop() {
    lv_timer_handler();
    
    // Always consume serial to avoid RX buffer overflow; framed parser discards if busy.
    process_serial_stream();

    // DEBUG: Comprehensive frame state logging
    static unsigned long last_dbg_ms = 0;
    static uint32_t dbg_frame_ready_count = 0;
    
    if(frame_ready) {
        dbg_frame_ready_count++;
    }
    
    if(millis() - last_dbg_ms > 1000) {
        last_dbg_ms = millis();
        Serial.printf("[DBG] ready=%d proc=%d pending=%d jsize=%u seq=%lu ready_cnt=%lu\n",
                      frame_ready, processing_frame, capture_write_pending, 
                      (unsigned)jpeg_size, (unsigned long)last_preview_seq_rx,
                      (unsigned long)dbg_frame_ready_count);
        
        // Show first 8 bytes of jpeg_buffer if we have data
        if(jpeg_size > 0) {
            Serial.printf("[DBG] jpeg_buf[0:8] = %02X %02X %02X %02X %02X %02X %02X %02X\n",
                          jpeg_buffer[0], jpeg_buffer[1], jpeg_buffer[2], jpeg_buffer[3],
                          jpeg_buffer[4], jpeg_buffer[5], jpeg_buffer[6], jpeg_buffer[7]);
        }
        dbg_frame_ready_count = 0;
    }

    // Frame decode ve göster
    if(frame_ready && !processing_frame && !capture_write_pending) {
        processing_frame = true;
        frame_ready = false;
        
        // Log that we're attempting decode
        Serial.printf("[DECODE] Attempting: size=%u first4=[%02X %02X %02X %02X]\n",
                      (unsigned)jpeg_size,
                      jpeg_size > 0 ? jpeg_buffer[0] : 0,
                      jpeg_size > 1 ? jpeg_buffer[1] : 0,
                      jpeg_size > 2 ? jpeg_buffer[2] : 0,
                      jpeg_size > 3 ? jpeg_buffer[3] : 0);
        
        uint16_t w = 0, h = 0;
        JRESULT jr = TJpgDec.getJpgSize(&w, &h, jpeg_buffer, jpeg_size);
        if(decoded_rgb565_static && jr == JDR_OK) {
            
            // Buffer boyutunu kontrol et
            if(w == DECODE_WIDTH && h == DECODE_HEIGHT) {
                decoded_width = w;
                decoded_height = h;
                
                // Decode et
                JRESULT draw_result = TJpgDec.drawJpg(0, 0, jpeg_buffer, jpeg_size);
                if(draw_result == JDR_OK) {
                    last_frame_valid = true;  // Mark frame as valid

                    // Record which preview seq was actually displayed
                    last_preview_seq_displayed = last_preview_seq_rx;
                    stat_frames_shown++;
                    
                    // Log successful decode (every 10th frame to avoid spam)
                    if(stat_frames_shown % 10 == 1) {
                        Serial.printf("[OK] Frame %lu decoded: %dx%d\n", 
                                     (unsigned long)stat_frames_shown, w, h);
                    }
                    
                    // NOT: Capture butonu artik yalnizca Pi'ya 4K capture komutu gonderiyor.
                    // ESP32 tarafinda JPEG kopyalama / SPIFFS'e yazma yapmiyoruz;
                    // boylece ekran glitch'i ve gecikmeli kare kaydetme tamamen kalkıyor.
                    
                    // LVGL image descriptor güncelle
                    jpeg_img_dsc.header.always_zero = 0;
                    jpeg_img_dsc.header.w = w;
                    jpeg_img_dsc.header.h = h;
                    jpeg_img_dsc.data_size = w * h * 2;
                    jpeg_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
                    jpeg_img_dsc.data = decoded_rgb565_static;
                    
                    // ÖNEMLI: UI image'ı güncelle
                    // No cache used (LV_IMG_CACHE_DEF_SIZE=0), but invalidate for safety
                    lv_img_cache_invalidate_src(&jpeg_img_dsc);
                    lv_img_set_src(ui_camera_view, &jpeg_img_dsc);
                    
                    // UI frame counter güncelle
                    notify_frame_received_for_ui();
                } else {
                    Serial.printf("[ERROR] drawJpg failed: result=%d size=%u\n",
                                 (int)draw_result, (unsigned)jpeg_size);
                }
            } else {
                Serial.printf("[WARN] Wrong size: %dx%d (expected %dx%d)\n", 
                             w, h, DECODE_WIDTH, DECODE_HEIGHT);
                last_frame_valid = false;  // Mark frame as invalid
                // Clear capture flag on bad decode
                if(capture_requested) {
                    capture_requested = false;
                    lv_label_set_text(ui_response_label, "Bad Frame");
                    lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFF5722), 0);
                }
            }
        } else {
            last_frame_valid = false;  // Mark frame as invalid
            Serial.printf("[ERROR] getJpgSize failed: jr=%d size=%u first4=[%02X %02X %02X %02X]\n",
                          (int)jr, (unsigned)jpeg_size,
                          jpeg_size > 0 ? jpeg_buffer[0] : 0,
                          jpeg_size > 1 ? jpeg_buffer[1] : 0,
                          jpeg_size > 2 ? jpeg_buffer[2] : 0,
                          jpeg_size > 3 ? jpeg_buffer[3] : 0);
            // Clear capture flag on decode failure
            if(capture_requested) {
                capture_requested = false;
                lv_label_set_text(ui_response_label, "Decode Failed");
                lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFF5722), 0);
            }
        }
        
        processing_frame = false;
    }

    // Incremental writer
    if(capture_write_pending) {
        size_t remaining = capture_size - capture_write_offset;
        size_t to_write = remaining > CAPTURE_CHUNK_SIZE ? CAPTURE_CHUNK_SIZE : remaining;
        if(to_write > 0) {
            size_t written = capture_file.write(capture_buffer_static + capture_write_offset, to_write);
            if(written == to_write) {
                capture_write_offset += written;
                uint8_t percent = (uint8_t)((capture_write_offset * 100UL) / capture_size);
                static uint8_t last_percent_shown = 0;
                if(percent - last_percent_shown >= 10 || percent == 100) {  // Update every 10%
                    char prog[24];
                    snprintf(prog, sizeof(prog), "Saving %u%%...", percent);
                    lv_label_set_text(ui_response_label, prog);
                    lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFFB300), 0);
                    last_percent_shown = percent;
                }
                if(capture_write_offset >= capture_size) {
                    capture_file.close();
                    lv_label_set_text(ui_response_label, "Photo Saved!");
                    lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x4CAF50), 0);
                    capture_size = 0;
                    capture_write_pending = false;
                }
            } else {
                capture_file.close();
                Serial.printf("[ERROR] Write failed: %zu bytes expected, got %zu\n", to_write, written);
                lv_label_set_text(ui_response_label, "Write Error");
                lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFF5722), 0);
                capture_size = 0;
                capture_write_pending = false;
            }
        } else {
            // Unexpected: to_write is 0 but still pending
            capture_file.close();
            Serial.println("[ERROR] Capture write finished with 0 bytes to write");
            capture_write_pending = false;
        }
    }

    // Periodic stats (lightweight, every 2s)
    unsigned long now = millis();
    if(now - stat_last_log_ms > 2000) {
        stat_last_log_ms = now;
        Serial.printf("[STAT] rx=%lu shown=%lu drop_busy=%lu\n",
                      (unsigned long)stat_frames_rx,
                      (unsigned long)stat_frames_shown,
                      (unsigned long)stat_frames_dropped_busy);
    }

    delay(1);
}