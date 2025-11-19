#include <Wire.h>
#include <SPI.h>
#include <lvgl.h>
#include "gfx_conf.h"
#include "ui.h"

// ==========================================
// UART & GÖRÜNTÜ AYARLARI
// ==========================================
// Kendi kartına göre pinleri kontrol et!
#define RX_PIN 15      // Raspberry Pi TX -> Buraya
#define TX_PIN 16      // Raspberry Pi RX -> Buraya
#define BAUD_RATE 2000000 // Yüksek hız şart
#define BACKLIGHT_PIN 38

// --- GÖRÜNTÜ ÇÖZÜNÜRLÜĞÜ ---
// UYARI: Raspberry Pi Python kodundaki boyutla BİREBİR AYNI olmalıdır.
// UART bant genişliği sınırlı olduğu için küçük boyut seçtik.
#define IMG_W 160 
#define IMG_H 120

// RGB565 formatında her piksel 2 byte'tır.
#define RAW_BUFFER_SIZE (IMG_W * IMG_H * 2)

// Gelen görüntüyü tutacak bellek
uint8_t frame_buffer[RAW_BUFFER_SIZE];
int buffer_pos = 0;

// Protokol Durum Makinesi
enum ReadState { WAIT_HEADER_1, WAIT_HEADER_2, READ_DATA };
ReadState currentState = WAIT_HEADER_1;

// UI'dan gelen istek (ui.h ile bağlantılı)
extern bool capture_requested; 
bool capture_requested = false; 

// ==========================================
// NESNELER
// ==========================================
LGFX tft;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// UART Nesnesi (Serial1 kullanıyoruz)
HardwareSerial CommSerial(1); 

// ==========================================
// CALLBACK'LER
// ==========================================

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

// ==========================================
// SETUP
// ==========================================
void setup()
{
    Serial.begin(115200); // USB Serial (Debug)
    delay(500);
    Serial.println("\n\n========== SISTEM BASLATILIYOR ==========");

    // 1. UART BAŞLATMA (Raspberry Pi Bağlantısı)
    // Raw görüntü büyük olduğu için buffer'ı artırıyoruz
    CommSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    CommSerial.setRxBufferSize(RAW_BUFFER_SIZE + 2048); 
    Serial.println("UART (Serial1) RAW Modda baslatildi.");

    // 2. EKRAN (LGFX) BAŞLATMA
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

    // 3. LVGL BAŞLATMA
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
    
    Serial.println("========== KURULUM TAMAMLANDI ==========");
    Serial.printf("Görüntü Hedefi: %dx%d\n", IMG_W, IMG_H);
    
    // Backlight (Varsa)
    pinMode(38, OUTPUT);
    digitalWrite(38, HIGH);
}

// ==========================================
// UART VERİ İŞLEME (RAW RGB565 PARSER)
// ==========================================
void process_uart_stream() {
    // Seri portta veri varsa oku
    while (CommSerial.available()) {
        uint8_t b = CommSerial.read();

        switch (currentState) {
            // Başlık Kontrolü (0xFF 0xAA)
            case WAIT_HEADER_1:
                if (b == 0xFF) currentState = WAIT_HEADER_2;
                break;
            
            case WAIT_HEADER_2:
                if (b == 0xAA) {
                    currentState = READ_DATA;
                    buffer_pos = 0; // Buffer'ı sıfırla, veriyi beklemeye başla
                } else {
                    currentState = WAIT_HEADER_1; // Hatalı başlık
                }
                break;

            // Veri Okuma
            case READ_DATA:
                frame_buffer[buffer_pos++] = b;
                
                // Tam bir kare dolduğunda
                if (buffer_pos == RAW_BUFFER_SIZE) {
                    // --- ÇİZİM İŞLEMİ ---
                    
                    // 1. Hedef panelin koordinatlarını al
                    lv_area_t coords;
                    lv_obj_get_coords(ui_img_panel, &coords);
                    
                    // 2. Görüntüyü panelin ortasına hizala
                    int panel_w = lv_obj_get_width(ui_img_panel);
                    int panel_h = lv_obj_get_height(ui_img_panel);
                    
                    int x = coords.x1 + (panel_w - IMG_W) / 2;
                    int y = coords.y1 + (panel_h - IMG_H) / 2;

                    // 3. LGFX ile belleği doğrudan ekrana bas (DMA kullanır, çok hızlıdır)
                    // pushImage(x, y, w, h, data)
                    // Not: ui_response_label yazısı üstte kalırsa şanslıyız, 
                    // yoksa LVGL redraw yaptığında üzerine yazar.
                    tft.pushImage(x, y, IMG_W, IMG_H, (uint16_t*)frame_buffer);
                    
                    // 4. Durumu sıfırla
                    currentState = WAIT_HEADER_1;
                    return; // Loop'a dön, UI donmasın
                }
                break;
        }
    }
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
        Serial.println("[LOOP] Capture komutu gonderiliyor...");
        CommSerial.println("CAPTURE"); // Pi'ye komut gönder
        capture_requested = false;
    }
    
    // İşlemciyi boğmamak için minik bir gecikme
    // Raw veri akarken bunu kaldırmak performansı artırabilir
    // delay(1); 
}