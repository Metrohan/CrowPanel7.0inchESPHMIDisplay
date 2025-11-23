#include <Wire.h>
#include <SPI.h>
#include <lvgl.h>
#include "gfx_conf.h"
#include "ui.h"
#include <TJpg_Decoder.h>
#include <WiFi.h>

#define BAUD_RATE 2000000
#define BACKLIGHT_PIN 38

// JPEG buffer - maksimum 80KB JPEG bekle
#define MAX_JPEG_SIZE (80 * 1024)
uint8_t jpeg_buffer[MAX_JPEG_SIZE];
volatile bool frame_ready = false;
size_t jpeg_size = 0;

// Eski değişkenler (UI kodunuz bunları kullanıyor olabilir)
bool capture_requested = false;
#define IMG_W 160
#define IMG_H 120
#define RAW_BUFFER_SIZE (IMG_W * IMG_H * 2)
uint8_t frame_buffer[RAW_BUFFER_SIZE];

// LVGL image descriptor
lv_img_dsc_t jpeg_img_dsc;
uint8_t *decoded_rgb565 = nullptr;
size_t decoded_width = 0;
size_t decoded_height = 0;

// ---------- LVGL / Display ----------
LGFX tft;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// ---------- WiFi TCP ----------
#define PORT 5000
WiFiServer server(PORT);
WiFiClient client;

// ---------- Chunked protocol ----------
#define MAGIC1 0xFF
#define MAGIC2 0xAA
#define MAX_CHUNK 8192

static size_t expected_frame_size = 0;
static uint16_t num_chunks = 0;
static bool *received_chunks = nullptr;
static uint16_t received_count = 0;

// ---------- CRC ----------
uint16_t crc16_ccitt(const uint8_t *data, size_t len, uint16_t seed = 0xFFFF) {
  uint16_t crc = seed;
  for (size_t i = 0; i < len; ++i) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; ++j) crc = (crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1);
  }
  return crc;
}

void send_ack(uint16_t seq) {
  if(client && client.connected()) {
    client.printf("ACK:%u\n", seq);
    client.flush();
  }
}

void send_nack(uint16_t seq) {
  if(client && client.connected()) {
    client.printf("NACK:%u\n", seq);
    client.flush();
  }
}

void prepare_chunk_map(size_t total_size) {
  if (received_chunks) {
    free(received_chunks);
    received_chunks = nullptr;
  }
  expected_frame_size = total_size;
  num_chunks = (expected_frame_size + MAX_CHUNK - 1) / MAX_CHUNK;
  received_chunks = (bool*)calloc(num_chunks, sizeof(bool));
  received_count = 0;
  Serial.printf("[INFO] Expecting %u chunks (%u bytes)\n", num_chunks, total_size);
}

// ---------- TJpg Decoder Callback ----------
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (decoded_rgb565 && x >= 0 && y >= 0) {
    for (int row = 0; row < h; row++) {
      uint32_t offset = ((y + row) * decoded_width + x);
      if (offset + w <= decoded_width * decoded_height) {
        uint16_t* dest = (uint16_t*)(decoded_rgb565 + offset * 2);
        memcpy(dest, bitmap + row * w, w * 2);
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

// ---------- Process TCP Stream ----------
void process_tcp_stream() {
    if(!client || !client.connected()) {
        client = server.available();
        if(client && client.connected()) {
            Serial.println("[INFO] Client connected");
        }
    }
    if(!client || !client.connected()) return;

    static uint8_t magic_state = 0;
    static uint8_t header_buffer[10];
    static int header_idx = 0;
    static uint16_t chunk_len = 0;
    static uint16_t seq = 0;
    static uint16_t sent_crc = 0;
    static int bytes_to_read = 0;

    while(client.available() > 0) {
        uint8_t b = client.read();

        // State 0: Waiting for first magic byte (0xFF)
        if(magic_state == 0) {
            if(b == MAGIC1) {
                magic_state = 1;
            }
            continue;
        }

        // State 1: Waiting for second magic byte (0xAA)
        if(magic_state == 1) {
            if(b == MAGIC2) {
                magic_state = 2;
                header_idx = 0;
            } else {
                magic_state = 0;
            }
            continue;
        }

        // State 2: Reading header (at least 6 bytes)
        if(magic_state == 2) {
            header_buffer[header_idx++] = b;

            // İlk 6 byte'ı kontrol et
            if(header_idx == 6) {
                chunk_len = (header_buffer[0] << 8) | header_buffer[1];
                seq = (header_buffer[2] << 8) | header_buffer[3];
                sent_crc = (header_buffer[4] << 8) | header_buffer[5];

                if(chunk_len == 0 || chunk_len > MAX_CHUNK) {
                    Serial.printf("[ERROR] Invalid chunk_len: %u\n", chunk_len);
                    send_nack(seq);
                    magic_state = 0;
                    continue;
                }

                // Eğer seq == 0 ise 4 byte daha bekle
                if(seq == 0) {
                    bytes_to_read = 4; // Total size için 4 byte daha
                    continue;
                }

                // Seq != 0, payload'a geç
                magic_state = 3;
            } else if(seq == 0 && header_idx == 10) {
                // Total size oku
                size_t total_frame_size = ((uint32_t)header_buffer[6] << 24) | 
                                          ((uint32_t)header_buffer[7] << 16) | 
                                          ((uint32_t)header_buffer[8] << 8) | 
                                          ((uint32_t)header_buffer[9]);
                
                Serial.printf("[INFO] New frame, total: %u bytes\n", total_frame_size);
                prepare_chunk_map(total_frame_size);
                jpeg_size = total_frame_size;
                
                magic_state = 3; // Payload'a geç
            }
            continue;
        }

        // State 3: Reading payload
        if(magic_state == 3) {
            // Payload'ı oku
            uint8_t *payload = (uint8_t*)malloc(chunk_len);
            if(!payload) {
                Serial.println("[ERROR] malloc failed");
                send_nack(seq);
                magic_state = 0;
                continue;
            }

            // İlk byte'ı ekle (zaten okumuştuk)
            payload[0] = b;
            size_t read_count = 1;

            // Kalan payload'ı oku
            unsigned long timeout_start = millis();
            while(read_count < chunk_len) {
                if(millis() - timeout_start > 3000) {
                    Serial.printf("[TIMEOUT] Read %u/%u bytes\n", read_count, chunk_len);
                    free(payload);
                    send_nack(seq);
                    magic_state = 0;
                    break;
                }

                if(client.available() > 0) {
                    int n = client.read(payload + read_count, chunk_len - read_count);
                    if(n > 0) {
                        read_count += n;
                        timeout_start = millis(); // Reset timeout
                    }
                }
                yield();
            }

            if(read_count != chunk_len) {
                free(payload);
                magic_state = 0;
                continue;
            }

            // CRC check
            uint16_t calc_crc = crc16_ccitt(payload, chunk_len);
            if(calc_crc != sent_crc) {
                Serial.printf("[CRC FAIL] seq=%u calc=%04X sent=%04X\n", seq, calc_crc, sent_crc);
                
                // Debug: İlk 16 byte'ı göster
                Serial.print("[DEBUG] First 16 bytes: ");
                for(int i = 0; i < 16 && i < chunk_len; i++) {
                    Serial.printf("%02X ", payload[i]);
                }
                Serial.println();
                
                send_nack(seq);
                free(payload);
                magic_state = 0;
                continue;
            }

            // Buffer'a kopyala
            uint32_t pos = (uint32_t)seq * MAX_CHUNK;
            if(pos + chunk_len <= MAX_JPEG_SIZE) {
                memcpy(jpeg_buffer + pos, payload, chunk_len);
                
                if(received_chunks && seq < num_chunks && !received_chunks[seq]) {
                    received_chunks[seq] = true;
                    received_count++;
                }
                
                send_ack(seq);
                Serial.printf("[OK] Chunk %u/%u (%u bytes)\n", seq + 1, num_chunks, chunk_len);
            } else {
                Serial.printf("[ERROR] Overflow pos=%u len=%u\n", pos, chunk_len);
                send_nack(seq);
            }

            free(payload);

            // Frame complete?
            if(received_chunks && received_count >= num_chunks) {
                frame_ready = true;
                Serial.printf("[COMPLETE] Frame %u bytes\n", jpeg_size);
                free(received_chunks);
                received_chunks = nullptr;
                received_count = 0;
            }

            // Reset state machine
            magic_state = 0;
            header_idx = 0;
        }
    }
}

// ---------- Setup ----------
void setup() {
    Serial.begin(BAUD_RATE);
    delay(200);
    Serial.println("ESP32_READY");

    // WiFi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_CAM_AP", "12345678");
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    server.begin();
    server.setNoDelay(true);
    
    Serial.print("[INFO] AP IP: ");
    Serial.println(WiFi.softAPIP());

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

    Serial.println("[READY] Waiting for frames...");
}

// ---------- Loop ----------
void loop() {
    lv_timer_handler();
    process_tcp_stream();

    if(frame_ready) {
        Serial.println("[DECODE] Starting JPEG decode...");
        
        // JPEG decode et
        uint16_t w = 0, h = 0;
        if(TJpgDec.getJpgSize(&w, &h, jpeg_buffer, jpeg_size) == JDR_OK) {
            Serial.printf("[INFO] JPEG size: %ux%u\n", w, h);
            
            // RGB565 buffer ayır
            if(decoded_rgb565) {
                free(decoded_rgb565);
                decoded_rgb565 = nullptr;
            }
            
            size_t buffer_size = w * h * 2;
            decoded_rgb565 = (uint8_t*)malloc(buffer_size);
            
            if(decoded_rgb565) {
                decoded_width = w;
                decoded_height = h;
                memset(decoded_rgb565, 0, buffer_size);
                
                // Decode et
                if(TJpgDec.drawJpg(0, 0, jpeg_buffer, jpeg_size) == JDR_OK) {
                    // LVGL image descriptor güncelle
                    jpeg_img_dsc.header.always_zero = 0;
                    jpeg_img_dsc.header.w = w;
                    jpeg_img_dsc.header.h = h;
                    jpeg_img_dsc.data_size = buffer_size;
                    jpeg_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
                    jpeg_img_dsc.data = decoded_rgb565;
                    
                    // UI güncelle
                    lv_img_set_src(ui_camera_view, &jpeg_img_dsc);
                    lv_obj_invalidate(ui_camera_view);
                    
                    Serial.println("[SUCCESS] Frame displayed");
                } else {
                    Serial.println("[ERROR] JPEG decode failed");
                    free(decoded_rgb565);
                    decoded_rgb565 = nullptr;
                }
            } else {
                Serial.printf("[ERROR] RGB565 malloc failed (need %u bytes)\n", buffer_size);
            }
        } else {
            Serial.printf("[ERROR] Invalid JPEG (size: %u bytes)\n", jpeg_size);
        }
        
        frame_ready = false;
    }

    if(capture_requested) {
        Serial.println("[INFO] Capture requested (not implemented)");
        capture_requested = false;
    }

    delay(5);
}