#include <Wire.h>
#include <SPI.h>
#include <lvgl.h>
#include "gfx_conf.h"
#include "ui.h"
#include "ui_events.h"


LGFX tft; 

static lv_color_t buf[screenWidth * 10]; 
static lv_display_t * display; 
static lv_indev_t * indev;

void my_disp_flush(lv_display_t * disp, const lv_area_t *area, uint8_t *px_map)
{
    tft.pushImageDMA(
        area->x1, area->y1, 
        area->x2 - area->x1 + 1, 
        area->y2 - area->y1 + 1, 
        (lgfx::rgb565_t*)px_map
    );
    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);

    if (!touched) {
        data->state = LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("Ekran başlatılıyor...");
    tft.begin();

    tft.setBrightness(160);

    tft.fillScreen(TFT_DARKGREY);

    lv_init();

    display = lv_display_create(screenWidth, screenHeight);
    lv_display_set_buffers(display, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, my_disp_flush);

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    lv_indev_set_display(indev, display);

    ui_init(); 

    Serial.println("Kurulum tamamlandı. Dokunmatik test başlıyor...");
}

void loop()
{
    lv_timer_handler();
    delay(5);
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        Serial.printf("Dokunma algılandı! X=%d, Y=%d\n", x, y);
    }
}
