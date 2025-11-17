#ifndef UI_H
#define UI_H

#include <lvgl.h>
#include <Arduino.h>

// -----------------------------------------------------------------
// GLOBAL NESNELER
// -----------------------------------------------------------------

// --- EKRANLAR ---
lv_obj_t * ui_Screen_Main;
lv_obj_t * ui_Screen_Settings;

// --- Ana Ekran Bileşenleri ---
lv_obj_t * ui_left_bar;
lv_obj_t * ui_main_panel;
lv_obj_t * ui_btn_panel;
lv_obj_t * ui_img_panel;
lv_obj_t * ui_response_label; 

// -----------------------------------------------------------------
// STİLLER
// -----------------------------------------------------------------
static lv_style_t style_sidebar;
static lv_style_t style_btn_sidebar;
static lv_style_t style_btn_orange;
static lv_style_t style_btn_green;
static lv_style_t style_btn_blue;
static lv_style_t style_btn_cyan;
static lv_style_t style_btn_red;

void create_styles() {
    lv_style_init(&style_sidebar);
    lv_style_set_bg_color(&style_sidebar, lv_color_hex(0x1A2A3A));
    lv_style_set_radius(&style_sidebar, 0);
    lv_style_set_border_width(&style_sidebar, 0);
    lv_style_set_pad_all(&style_sidebar, 0);

    lv_style_init(&style_btn_sidebar);
    lv_style_set_bg_color(&style_btn_sidebar, lv_color_hex(0xFFA500));
    lv_style_set_text_color(&style_btn_sidebar, lv_color_white());
    lv_style_set_border_width(&style_btn_sidebar, 0);

    lv_style_init(&style_btn_orange);
    lv_style_set_bg_color(&style_btn_orange, lv_color_hex(0xFFA500));
    lv_style_set_border_width(&style_btn_orange, 0);

    lv_style_init(&style_btn_green);
    lv_style_set_bg_color(&style_btn_green, lv_color_hex(0x4CAF50));
    lv_style_set_border_width(&style_btn_green, 0);

    lv_style_init(&style_btn_blue);
    lv_style_set_bg_color(&style_btn_blue, lv_color_hex(0x2196F3));
    lv_style_set_border_width(&style_btn_blue, 0);

    lv_style_init(&style_btn_cyan);
    lv_style_set_bg_color(&style_btn_cyan, lv_color_hex(0x00BCD4));
    lv_style_set_border_width(&style_btn_cyan, 0);

    lv_style_init(&style_btn_red);
    lv_style_set_bg_color(&style_btn_red, lv_color_hex(0xF44336));
    lv_style_set_border_width(&style_btn_red, 0);
}

// -----------------------------------------------------------------
// EVENT HANDLER'LAR (ÖNCE TANIMLA!)
// -----------------------------------------------------------------

// GERİ BUTONU OLAY YÖNETİCİSİ
static void settings_back_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[UI] 'Geri' Butonuna Basildi, Ana Ekrana Donuluyor.");
        lv_scr_load_anim(ui_Screen_Main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
}

// GENEL OLAY YÖNETİCİSİ
static void general_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const char * btn_name = (const char *)lv_event_get_user_data(e);
    
    if (code == LV_EVENT_CLICKED) {
        Serial.printf("[UI] Buton Tiklandi: %s\n", btn_name);
        
        if (strcmp(btn_name, "Ayarlar") == 0) {
            Serial.println("[UI] Ayarlar Ekranina Geciliyor.");
            lv_scr_load_anim(ui_Screen_Main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        }
        else if (strcmp(btn_name, "Capture") == 0) {
            Serial.println("[UI] Capture butonuna basildi!");
            lv_label_set_text(ui_response_label, "Fotograf Cekildi!");
            lv_obj_center(ui_response_label);
        }
        else if (strcmp(btn_name, "Metrics") == 0) {
            Serial.println("[UI] Metrics butonuna basildi!");
            lv_label_set_text(ui_response_label, "Metrikler Gosteriliyor...");
            lv_obj_center(ui_response_label);
        }
        else if (strcmp(btn_name, "Focus") == 0) {
            Serial.println("[UI] Focus butonuna basildi!");
            lv_label_set_text(ui_response_label, "Otomatik Odaklama Yapildi.");
            lv_obj_center(ui_response_label);
        }
        else if (strcmp(btn_name, "Menu") == 0) {
            Serial.println("[UI] Menu butonuna basildi!");
            lv_label_set_text(ui_response_label, "Menu Acildi!");
            lv_obj_center(ui_response_label);
        }
    }
}

// -----------------------------------------------------------------
// YARDIMCI BUTON OLUŞTURUCU
// -----------------------------------------------------------------

static void add_button_press_effect(lv_obj_t * btn)
{
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, [](lv_event_t * e){
        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);  // ✅ cast eklendi

        if (code == LV_EVENT_PRESSED) {
            lv_obj_set_style_bg_opa(obj, LV_OPA_70, LV_PART_MAIN);
            lv_obj_set_style_bg_color(
                obj,
                lv_color_darken(lv_obj_get_style_bg_color(obj, LV_PART_MAIN), 40),
                LV_PART_MAIN
            );
        }
        else if (code == LV_EVENT_RELEASED || code == LV_EVENT_CLICKED) {
            lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_bg_color(
                obj,
                lv_color_lighten(lv_obj_get_style_bg_color(obj, LV_PART_MAIN), 40),
                LV_PART_MAIN
            );
        }
    }, LV_EVENT_ALL, NULL);
}

lv_obj_t* create_ui_button(lv_obj_t * parent, const char * text, const char * event_data, lv_style_t* style)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_add_style(btn, style, 0); 
    lv_obj_set_size(btn, 115, 60);
    
    // Event callback ekle
    lv_obj_add_event_cb(btn, general_event_handler, LV_EVENT_CLICKED, (void*)event_data);
    
    // Butonu tıklanabilir yap
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text); 
    lv_obj_center(label);

    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);

    Serial.printf("[UI] Buton olusturuldu: %s\n", text);
    
    return btn;
}



// -----------------------------------------------------------------
// AYARLAR EKRANI OLUŞTURUCU
// -----------------------------------------------------------------
void ui_Screen_Settings_init(void)
{
    ui_Screen_Settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_Screen_Settings, lv_color_hex(0x1E1E1E), 0);

    lv_obj_t* label_title = lv_label_create(ui_Screen_Settings);
    lv_label_set_text(label_title, "AYARLAR");
    lv_obj_set_style_text_color(label_title, lv_color_white(), 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t* btn_back = lv_btn_create(ui_Screen_Settings);
    lv_obj_set_size(btn_back, 100, 50);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(btn_back, settings_back_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn_back, LV_OBJ_FLAG_CLICKABLE);
    
    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Geri");
    lv_obj_center(label_back);

    lv_obj_t* slider_label = lv_label_create(ui_Screen_Settings);
    lv_label_set_text(slider_label, "Ekran Parlakligi");
    lv_obj_set_style_text_color(slider_label, lv_color_white(), 0);
    lv_obj_align(slider_label, LV_ALIGN_TOP_LEFT, 50, 80);

    lv_obj_t* slider = lv_slider_create(ui_Screen_Settings);
    lv_obj_set_size(slider, 300, 20);
    lv_obj_align_to(slider, slider_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    
    Serial.println("[UI] Ayarlar ekrani olusturuldu");
}

// -----------------------------------------------------------------
// ANA ARAYÜZ OLUŞTURUCU
// -----------------------------------------------------------------
void ui_Screen_Main_init(void)
{
    ui_Screen_Main = lv_scr_act();
    lv_obj_set_style_bg_color(ui_Screen_Main, lv_color_hex(0x1E1E1E), 0);

    ui_left_bar = lv_obj_create(ui_Screen_Main);
    lv_obj_add_style(ui_left_bar, &style_sidebar, 0);
    lv_obj_set_size(ui_left_bar, 80, 480);
    lv_obj_align(ui_left_bar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(ui_left_bar, lv_color_hex(0x252525), 0);

    lv_obj_t* menu_btn = lv_btn_create(ui_left_bar);
    lv_obj_add_style(menu_btn, &style_btn_sidebar, 0);
    lv_obj_set_size(menu_btn, 70, 50);
    lv_obj_align(menu_btn, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_add_event_cb(menu_btn, general_event_handler, LV_EVENT_CLICKED, (void*)"Menu");
    lv_obj_clear_flag(menu_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(menu_btn, LV_OBJ_FLAG_CLICKABLE);
    
    lv_obj_t* menu_label = lv_label_create(menu_btn);
    lv_label_set_text(menu_label, "Menu");
    lv_obj_center(menu_label);

    lv_obj_t* settings_btn = lv_btn_create(ui_left_bar);
    lv_obj_add_style(settings_btn, &style_btn_sidebar, 0);
    lv_obj_set_size(settings_btn, 70, 50);
    lv_obj_align(settings_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(settings_btn, general_event_handler, LV_EVENT_CLICKED, (void*)"Ayarlar");
    lv_obj_clear_flag(settings_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(settings_btn, LV_OBJ_FLAG_CLICKABLE);
    
    lv_obj_t* settings_label = lv_label_create(settings_btn);
    lv_label_set_text(settings_label, "Ayar");
    lv_obj_center(settings_label);

    ui_main_panel = lv_obj_create(ui_Screen_Main);
    lv_obj_set_size(ui_main_panel, 720, 480);
    lv_obj_align(ui_main_panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(ui_main_panel, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_border_width(ui_main_panel, 0, 0);
    lv_obj_set_style_pad_all(ui_main_panel, 10, 0);
    
    ui_btn_panel = lv_obj_create(ui_main_panel);
    lv_obj_set_size(ui_btn_panel, 700, 160);
    lv_obj_align(ui_btn_panel, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_flex_flow(ui_btn_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_btn_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(ui_btn_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_btn_panel, 0, 0);
    lv_obj_set_style_pad_gap(ui_btn_panel, 15, 0);

    create_ui_button(ui_btn_panel, "Capture", "Capture", &style_btn_orange);
    create_ui_button(ui_btn_panel, "Metrics", "Metrics", &style_btn_green);
    create_ui_button(ui_btn_panel, "Focus", "Focus", &style_btn_blue);

    ui_img_panel = lv_obj_create(ui_main_panel);
    lv_obj_set_size(ui_img_panel, 700, 280);
    lv_obj_align(ui_img_panel, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(ui_img_panel, lv_color_hex(0x303030), 0);
    lv_obj_set_style_border_color(ui_img_panel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_border_width(ui_img_panel, 2, 0);

    ui_response_label = lv_label_create(ui_img_panel);
    lv_label_set_text(ui_response_label, "Camera Preview");
    lv_obj_set_style_text_color(ui_response_label, lv_color_white(), 0);
    lv_obj_center(ui_response_label);
    
    Serial.println("[UI] Ana ekran olusturuldu");
}

// -----------------------------------------------------------------
// ANA UI GİRİŞ FONKSİYONU
// -----------------------------------------------------------------
void ui_init(void)
{
    Serial.println("[UI] UI baslatiiliyor...");
    create_styles();          
    ui_Screen_Main_init();    
    ui_Screen_Settings_init();
    Serial.println("[UI] UI baslatma tamamlandi");
}

#endif