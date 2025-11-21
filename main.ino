#include <Wire.h>
#include <SPI.h>
#include <lvgl.h>
#include "gfx_conf.h" // Kendi LGFX yapılandırmanız
#include "ui.h" // UI bileşenleri ve boyut tanımları

// ==========================================
// USB SERİ AYARLARI
// ==========================================
#define BAUD_RATE 1000000 // Yüksek hızlı USB için
#define BACKLIGHT_PIN 38  // Ekran Arka Işık Pini

// Görüntü boyutları (ui.h'dan gelir)
#define RAW_BUFFER_SIZE (IMG_W * IMG_H * 2)

// Gelen görüntüyü tutacak bellek (RAW Buffer)
uint8_t frame_buffer[RAW_BUFFER_SIZE];
int buffer_pos = 0;

// Protokol Durum Makinesi
enum ReadState { WAIT_HEADER_1, WAIT_HEADER_2, READ_DATA };
ReadState currentState = WAIT_HEADER_1;
extern bool capture_requested; 
bool capture_requested = false; 

// ==========================================
// NESNELER ve CALLBACK'LERİN PROTOTİPLERİ
// ==========================================
LGFX tft;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// Forward Declarations (Scope Hatası Çözümü)
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void my_touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
void set_brightness(int value); 
void slider_event_cb(lv_event_t * e);

// ==========================================
// HW KONTROL VE CALLBACK TANIMLARI
// ==========================================

void set_brightness(int value) {
    if(value < 0) value = 0;
    if(value > 255) value = 255;
    analogWrite(BACKLIGHT_PIN, value);
    Serial.printf("[HW] Parlaklik Ayarlandi: %d\n", value);
}

void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int brightness_val = (int)lv_slider_get_value(slider);
    set_brightness(brightness_val);
}

/* --- LVGL flush callback --- */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    if (tft.getStartCount() == 0) { tft.startWrite(); }
    tft.pushImageDMA(
        area->x1, area->y1,
        area->x2 - area->x1 + 1,
        area->y2 - area->y1 + 1,
        (lgfx::rgb565_t *)color_p
    );
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

/* --- Dokunmatik okuma callback --- */
void my_touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    uint16_t x, y;
    bool touched = tft.getTouch(&x, &y); 

    if (touched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ==========================================
// USB SERİ VERİ İŞLEME (RAW RGB565 PARSER)
// ==========================================
void process_uart_stream() {
    const size_t target_size = RAW_BUFFER_SIZE; 
    
    while (Serial.available() > 0) {
        uint8_t b = Serial.read();

        switch (currentState) {
            case WAIT_HEADER_1:
                if (b == 0xFF) currentState = WAIT_HEADER_2;
                break;
            
            case WAIT_HEADER_2:
                if (b == 0xAA) {
                    currentState = READ_DATA;
                    buffer_pos = 0;
                } else {
                    currentState = WAIT_HEADER_1;
                }
                break;

            case READ_DATA:
                frame_buffer[buffer_pos++] = b;
                
                // --- ÇİZİM ZAMANI ---
                if (buffer_pos == target_size) {
                    // LVGL koordinatlarını al
                    lv_area_t coords;
                    lv_obj_get_coords(ui_img_panel, &coords);
                    int panel_w = lv_obj_get_width(ui_img_panel);
                    int panel_h = lv_obj_get_height(ui_img_panel);
                    
                    int x = coords.x1 + (panel_w - IMG_W) / 2;
                    int y = coords.y1 + (panel_h - IMG_H) / 2;

                    // RAW veriyi ekrana bas (Python doğru sıralamada gönderiyor)
                    tft.pushImage(x, y, IMG_W, IMG_H, (uint16_t*)frame_buffer);
                    
                    currentState = WAIT_HEADER_1;
                    return; // WDT Reset'i önlemek için hemen çıkış yap
                }
                
                // WDT KONTROLÜ
                if (buffer_pos % 512 == 0) { 
                    lv_timer_handler(); 
                    yield();            
                }
                
                break;
        }
    }
}

// ==========================================
// SETUP
// ==========================================
void setup()
{
    Serial.begin(115200); // USB Seri Monitör (Debug)
    delay(500);

    // KRİTİK: Yüksek hızlı USB Seri Port Başlatma (Pi ile iletişim için)
    Serial.begin(BAUD_RATE); 
    
    Serial.println("\n\n========== SISTEM BASLATILIYOR ==========");

    // 1. EKRAN (LGFX) BAŞLATMA
    if (tft.begin()) {
        Serial.println("==> tft.begin() BASARILI.");
    } else {
        Serial.println("!!! tft.begin() BASARISIZ !!!");
        while(1) delay(10);
    }
    
    if (tft.touch()) {
        Serial.println("==> Dokunmatik aktif.");
    }

    pinMode(BACKLIGHT_PIN, OUTPUT);
    set_brightness(200);
    tft.fillScreen(TFT_DARKGREY);

    // 2. LVGL BAŞLATMA
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

    ui_init(); // UI yükle
    
    Serial.println("========== KURULUM TAMAMLANDI ==========");
    Serial.printf("Görüntü Hedefi: %dx%d @ %d Mbps\n", IMG_W, IMG_H, BAUD_RATE / 1000000);
}

// ==========================================
// LOOP
// ==========================================
void loop()
{
    // 1. LVGL UI Güncellemesi
    lv_timer_handler();
    
    // 2. Kameradan Gelen Veriyi İşle
    process_uart_stream(); 

    // 3. Capture İsteği Kontrolü
    if (capture_requested) {
        Serial.println("[LOOP] Capture komutu gonderiliyor."); 
        Serial.println("CAPTURE"); // USB Seri Port üzerinden Pi'ye gönder
        capture_requested = false;
    }
}