#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#include <lvgl.h>
#include <Arduino.h>



extern lv_obj_t * ui_Screen_Main;
extern lv_obj_t * ui_Screen_Settings;
extern lv_obj_t * ui_response_label;


// -----------------------------------------------------------------
// GERİ BUTONU OLAY YÖNETİCİSİ
// -----------------------------------------------------------------
static void settings_back_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        Serial.println("[UI] 'Geri' Butonuna Basıldı, Ana Ekrana Dönülüyor.");
        
        // 'extern' ile bildirdiğimiz global nesneyi kullan
        lv_screen_load_anim(ui_Screen_Main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
}


// -----------------------------------------------------------------
// GENEL OLAY YÖNETİCİSİ
// -----------------------------------------------------------------
static void general_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const char * btn_name = (const char *)lv_event_get_user_data(e); 

    if (code == LV_EVENT_CLICKED) {
        
        Serial.printf("[UI] Buton Tıklandı: %s\n", btn_name);
        
        if (strcmp(btn_name, "Ayarlar") == 0) {
            Serial.println("[UI] Ayarlar Ekranına Geçiliyor.");
            // 'extern' ile bildirdiğimiz global nesneyi kullan
            lv_screen_load_anim(ui_Screen_Settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        }
        else if (strcmp(btn_name, "Capture") == 0) {
            lv_label_set_text(ui_response_label, "Fotoğraf Çekildi!");
            lv_obj_center(ui_response_label);
        }
        else if (strcmp(btn_name, "Metrics") == 0) {
            lv_label_set_text(ui_response_label, "Metrikler Gosteriliyor...");
            lv_obj_center(ui_response_label);
        }
        else if (strcmp(btn_name, "Focus") == 0) {
            lv_label_set_text(ui_response_label, "Otomatik Odaklama Yapıldı.");
            lv_obj_center(ui_response_label);
        }

        else if (strcmp(btn_name, "Menu") == 0) {
            lv_label_set_text(ui_response_label, "Menu Acildi!");
            lv_obj_center(ui_response_label);
        }
    }
}

#endif