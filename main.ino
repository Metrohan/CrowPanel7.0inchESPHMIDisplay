#include <Wire.h>
#include <SPI.h>
#include <lvgl.h>
#include "gfx_conf.h"
#include "ui.h"

LGFX tft;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

/* --- LVGL flush callback --- */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    tft.pushImageDMA(
        area->x1, area->y1,
        area->x2 - area->x1 + 1,
        area->y2 - area->y1 + 1,
        (lgfx::rgb565_t *)color_p
    );
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
        Serial.printf("[TOUCH] Dokunma algılandı! X=%d Y=%d\n", x, y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n========== SISTEM BASLATILIYOR ==========");

    Serial.println("LGFX (tft.begin()) baslatiliyor...");
    bool init_ok = tft.begin();
    
    if (init_ok) {
        Serial.println("==> tft.begin() BASARILI OLDU.");
    } else {
        Serial.println("!!! tft.begin() BASARISIZ OLDU !!!");
        while(1) { delay(10); }
    }
    
    if (tft.touch()) {
        Serial.println("==> LovyanGFX: Dokunmatik surucu bulundu ve aktif.");
    } else {
        Serial.println("!!! LovyanGFX: Dokunmatik surucu BULUNAMADI !!!");
        while(1) { delay(10); }
    }

    tft.setBrightness(160);
    tft.fillScreen(TFT_DARKGREY);

    lv_init();

    // === DISPLAY ===
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);

    // === TOUCH INPUT ===
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();
    
    Serial.println("========== KURULUM TAMAMLANDI ==========");
    Serial.printf("Ekran boyutu: %d x %d\n", screenWidth, screenHeight);
    Serial.println("Ekrana dokunun...\n");

    pinMode(38, OUTPUT);
    digitalWrite(38, LOW);
    delay(20);
    digitalWrite(38, HIGH);
    delay(100);
}

void loop()
{
    lv_timer_handler();
    delay(5);
}
