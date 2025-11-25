#ifndef UI_H
#define UI_H

#include <lvgl.h>
#include <Arduino.h>
#include <WiFi.h>
#ifdef USE_SPIFFS
#include <SPIFFS.h>
#endif

// -----------------------------------------------------------------
// 1. BOYUT AYARLARI 
// -----------------------------------------------------------------
#define IMG_W 160
#define IMG_H 120

// -----------------------------------------------------------------
// 2. HARİCİ DEĞİŞKEN VE FONKSİYON REFERANSLARI 
// -----------------------------------------------------------------
extern uint8_t frame_buffer[]; // RAW Buffer adı geri döndü
extern bool capture_requested;

// External variable and function references
extern uint8_t frame_buffer[]; // RAW Buffer adı geri döndü
extern bool capture_requested;

extern void slider_event_cb(lv_event_t * e);
extern void set_brightness(int value); 

// Tekil UI frame sayacı bildirimi (gerçek tanım main.ino'da olacak)
extern uint32_t ui_total_frames_counter;

// UI Ayarları
static bool is_dark_theme = true; // Koyu tema aktif
static int text_size_mode = 1; // 0=Küçük, 1=Orta, 2=Büyük

// Font size pointers
static const lv_font_t* current_font_small = &lv_font_montserrat_12;
static const lv_font_t* current_font_medium = &lv_font_montserrat_14;
static const lv_font_t* current_font_large = &lv_font_montserrat_16;
static const lv_font_t* current_font_xlarge = &lv_font_montserrat_18;

// Camera status tracking
static uint32_t last_frame_count = 0;
static unsigned long last_camera_update = 0;
static bool camera_active = false;

// -----------------------------------------------------------------
// 3. GLOBAL NESNELER (UI elemanları)
// -----------------------------------------------------------------
lv_obj_t * ui_Screen_Main;
lv_obj_t * ui_Screen_Settings;
lv_obj_t * ui_Screen_Menu;

lv_obj_t * ui_left_bar;
lv_obj_t * ui_main_panel;
lv_obj_t * ui_btn_panel;
lv_obj_t * ui_img_panel;
lv_obj_t * ui_response_label; 
lv_obj_t * ui_camera_view; 

lv_img_dsc_t my_img_dsc; 

// Diagnostics labels (Menu ekranında gösterilecek)
static lv_obj_t *lbl_conn_status = nullptr;
static lv_obj_t *lbl_camera_status = nullptr;
static lv_obj_t *lbl_fps = nullptr;
static lv_obj_t *lbl_heap = nullptr;
static lv_obj_t *lbl_frame_stats = nullptr;
static lv_obj_t *lbl_wifi_info = nullptr;
static lv_obj_t *btn_refresh = nullptr;

// Settings screen controls
static lv_obj_t *theme_switch = nullptr;
static lv_obj_t *text_size_dropdown = nullptr;

// Main screen labels
static lv_obj_t *lbl_res = nullptr;
static lv_obj_t *lbl_focus = nullptr;
static lv_obj_t *lbl_capture = nullptr;
static lv_obj_t *lbl_menu = nullptr;
static lv_obj_t *lbl_settings = nullptr;

// Menu screen title labels
static lv_obj_t *lbl_menu_title = nullptr;
static lv_obj_t *lbl_conn_title = nullptr;
static lv_obj_t *lbl_cam_title = nullptr;
static lv_obj_t *lbl_perf_title = nullptr;

// Settings screen labels
static lv_obj_t *lbl_settings_title = nullptr;
static lv_obj_t *lbl_brightness = nullptr;
static lv_obj_t *lbl_theme = nullptr;
static lv_obj_t *lbl_text_size = nullptr;

// -----------------------------------------------------------------
// STİLLER (KARANLIK TEMA ve KART YAPISI)
// -----------------------------------------------------------------
static lv_style_t style_sidebar;
static lv_style_t style_sidebar_accent; // Dekoratif açık gri sütun
static lv_style_t style_btn_sidebar;
static lv_style_t style_btn_orange;
static lv_style_t style_btn_green;
static lv_style_t style_btn_blue;
static lv_style_t style_card_bg;

void create_styles() {
    lv_style_init(&style_card_bg);
    lv_style_set_bg_color(&style_card_bg, lv_color_hex(0x1F1F1F)); 
    lv_style_set_radius(&style_card_bg, 10); 
    lv_style_set_border_width(&style_card_bg, 0);
    lv_style_set_pad_all(&style_card_bg, 10);

    lv_style_init(&style_sidebar);
    lv_style_set_bg_color(&style_sidebar, lv_color_hex(0x131313));
    lv_style_set_radius(&style_sidebar, 0);
    lv_style_set_border_width(&style_sidebar, 0);
    lv_style_set_pad_all(&style_sidebar, 0);

    lv_style_init(&style_sidebar_accent);
    lv_style_set_bg_color(&style_sidebar_accent, lv_color_hex(0x3A3A3A)); // Açık gri
    lv_style_set_radius(&style_sidebar_accent, 0);
    lv_style_set_border_width(&style_sidebar_accent, 0);
    lv_style_set_pad_all(&style_sidebar_accent, 0);
    lv_style_set_bg_opa(&style_sidebar_accent, LV_OPA_COVER); // Tam opak

    lv_style_init(&style_btn_sidebar);
    lv_style_set_bg_color(&style_btn_sidebar, lv_color_hex(0x2D2D2D));
    lv_style_set_bg_opa(&style_btn_sidebar, LV_OPA_COVER);
    lv_style_set_text_color(&style_btn_sidebar, lv_color_hex(0xE0E0E0));
    lv_style_set_border_width(&style_btn_sidebar, 0);
    lv_style_set_radius(&style_btn_sidebar, 8); // Yuvarlatılmış köşeler
    lv_style_set_shadow_width(&style_btn_sidebar, 5);
    lv_style_set_shadow_color(&style_btn_sidebar, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&style_btn_sidebar, LV_OPA_30); 

    lv_style_init(&style_btn_orange);
    lv_style_set_bg_color(&style_btn_orange, lv_color_hex(0xFD6D4E));
    lv_style_set_radius(&style_btn_orange, 8); 

    lv_style_init(&style_btn_green);
    lv_style_set_bg_color(&style_btn_green, lv_color_hex(0x4CAF50));
    lv_style_set_radius(&style_btn_green, 8); 

    lv_style_init(&style_btn_blue);
    lv_style_set_bg_color(&style_btn_blue, lv_color_hex(0x2196F3));
    lv_style_set_radius(&style_btn_blue, 8); 
}

// -----------------------------------------------------------------
// EVENT HANDLER'LAR
// -----------------------------------------------------------------
static void back_to_main_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        Serial.println("[UI] Geri donuluyor -> Main Screen");
        lv_scr_load_anim(ui_Screen_Main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
}

static void general_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const char * btn_name = (const char *)lv_event_get_user_data(e);
    
    if (code == LV_EVENT_CLICKED) {
        if (strcmp(btn_name, "Ayarlar") == 0) {
            Serial.println("[UI] Gecis -> Ayarlar");
            lv_scr_load_anim(ui_Screen_Settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        }
        else if (strcmp(btn_name, "Menu") == 0) {
            Serial.println("[UI] Gecis -> Menu");
            lv_scr_load_anim(ui_Screen_Menu, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        }
        else if (strcmp(btn_name, "Capture") == 0) {
            lv_label_set_text(ui_response_label, "FOTO CEKILIYOR...");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFFB300), 0); // Turuncu-sarı
            capture_requested = true; 
        }
        else if (strcmp(btn_name, "Metrics") == 0) {
            lv_label_set_text(ui_response_label, "Metrikler Yukleniyor...");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x00E676), 0); // Yeşil
        }
        else if (strcmp(btn_name, "Focus") == 0) {
            lv_label_set_text(ui_response_label, "Odaklaniliyor...");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x00B0FF), 0); // Mavi
        }
    }
}

// -----------------------------------------------------------------
// TEMA DEĞİŞTİRME FONKSİYONU
// -----------------------------------------------------------------
void update_text_sizes() {
    // Update font pointers based on text_size_mode
    if(text_size_mode == 0) { // Small
        current_font_small = &lv_font_montserrat_10;
        current_font_medium = &lv_font_montserrat_12;
        current_font_large = &lv_font_montserrat_14;
        current_font_xlarge = &lv_font_montserrat_16;
    } else if(text_size_mode == 1) { // Medium (default)
        current_font_small = &lv_font_montserrat_12;
        current_font_medium = &lv_font_montserrat_14;
        current_font_large = &lv_font_montserrat_16;
        current_font_xlarge = &lv_font_montserrat_18;
    } else { // Large
        current_font_small = &lv_font_montserrat_14;
        current_font_medium = &lv_font_montserrat_16;
        current_font_large = &lv_font_montserrat_18;
        current_font_xlarge = &lv_font_montserrat_20;
    }
    
    // Apply to main screen
    if(ui_response_label) lv_obj_set_style_text_font(ui_response_label, current_font_large, 0);
    if(lbl_res) lv_obj_set_style_text_font(lbl_res, current_font_small, 0);
    if(lbl_focus) lv_obj_set_style_text_font(lbl_focus, current_font_xlarge, 0);
    if(lbl_capture) lv_obj_set_style_text_font(lbl_capture, current_font_xlarge, 0);
    if(lbl_menu) lv_obj_set_style_text_font(lbl_menu, current_font_small, 0);
    if(lbl_settings) lv_obj_set_style_text_font(lbl_settings, current_font_small, 0);
    
    // Apply to menu screen
    if(lbl_menu_title) lv_obj_set_style_text_font(lbl_menu_title, current_font_large, 0);
    if(lbl_conn_title) lv_obj_set_style_text_font(lbl_conn_title, current_font_medium, 0);
    if(lbl_cam_title) lv_obj_set_style_text_font(lbl_cam_title, current_font_medium, 0);
    if(lbl_perf_title) lv_obj_set_style_text_font(lbl_perf_title, current_font_medium, 0);
    if(lbl_conn_status) lv_obj_set_style_text_font(lbl_conn_status, current_font_small, 0);
    if(lbl_camera_status) lv_obj_set_style_text_font(lbl_camera_status, current_font_small, 0);
    if(lbl_fps) lv_obj_set_style_text_font(lbl_fps, current_font_small, 0);
    if(lbl_heap) lv_obj_set_style_text_font(lbl_heap, current_font_small, 0);
    if(lbl_frame_stats) lv_obj_set_style_text_font(lbl_frame_stats, current_font_small, 0);
    
    // Apply to settings screen
    if(lbl_settings_title) lv_obj_set_style_text_font(lbl_settings_title, current_font_large, 0);
    if(lbl_brightness) lv_obj_set_style_text_font(lbl_brightness, current_font_medium, 0);
    if(lbl_theme) lv_obj_set_style_text_font(lbl_theme, current_font_medium, 0);
    if(lbl_text_size) lv_obj_set_style_text_font(lbl_text_size, current_font_medium, 0);
    
    Serial.printf("[UI] Text size updated to mode %d\n", text_size_mode);
}

void apply_theme() {
    if(is_dark_theme) {
        // Koyu tema renkleri
        lv_obj_set_style_bg_color(ui_Screen_Main, lv_color_hex(0x111111), 0);
        lv_obj_set_style_bg_color(ui_Screen_Settings, lv_color_hex(0x111111), 0);
        lv_obj_set_style_bg_color(ui_Screen_Menu, lv_color_hex(0x111111), 0);
        lv_obj_set_style_bg_color(ui_left_bar, lv_color_hex(0x3A3A3A), 0);
        lv_obj_set_style_bg_color(ui_main_panel, lv_color_hex(0x111111), 0);
        
        // Kartları koyu yap
        lv_style_set_bg_color(&style_card_bg, lv_color_hex(0x1F1F1F));
        Serial.println("[UI] Dark theme applied");
    } else {
        // Açık tema renkleri
        lv_obj_set_style_bg_color(ui_Screen_Main, lv_color_hex(0xF5F5F5), 0);
        lv_obj_set_style_bg_color(ui_Screen_Settings, lv_color_hex(0xF5F5F5), 0);
        lv_obj_set_style_bg_color(ui_Screen_Menu, lv_color_hex(0xF5F5F5), 0);
        lv_obj_set_style_bg_color(ui_left_bar, lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_bg_color(ui_main_panel, lv_color_hex(0xF5F5F5), 0);
        
        // Kartları açık yap
        lv_style_set_bg_color(&style_card_bg, lv_color_hex(0xFFFFFF));
        Serial.println("[UI] Light theme applied");
    }
}

// -----------------------------------------------------------------
// YARDIMCI BUTON FONKSİYONU
// -----------------------------------------------------------------
lv_obj_t* create_ui_button(lv_obj_t * parent, const char * text, const char * event_data, lv_style_t* style)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_add_style(btn, style, 0); 
    lv_obj_set_size(btn, 180, 70);
    
    lv_obj_add_event_cb(btn, general_event_handler, LV_EVENT_CLICKED, (void*)event_data);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE); 
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text); 
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    return btn;
}

// -----------------------------------------------------------------
// UI EKRANLARI (init fonksiyonları)
// -----------------------------------------------------------------

void ui_Screen_Settings_init(void)
{
    ui_Screen_Settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_Screen_Settings, lv_color_hex(0x111111), 0);
    lv_obj_clear_flag(ui_Screen_Settings, LV_OBJ_FLAG_SCROLLABLE); 

    lv_obj_t* settings_card = lv_obj_create(ui_Screen_Settings);
    lv_obj_add_style(settings_card, &style_card_bg, 0);
    lv_obj_set_size(settings_card, 750, 470);
    lv_obj_center(settings_card);
    lv_obj_clear_flag(settings_card, LV_OBJ_FLAG_SCROLLABLE); 

    lbl_settings_title = lv_label_create(settings_card);
    lv_label_set_text(lbl_settings_title, "AYARLAR");
    lv_obj_set_style_text_color(lbl_settings_title, lv_color_hex(0x00BCD4), 0);
    lv_obj_set_style_text_font(lbl_settings_title, current_font_large, 0);
    lv_obj_align(lbl_settings_title, LV_ALIGN_TOP_MID, 0, 10);

    // --- EKRAN PARLAKLIGI ---
    lv_obj_t* brightness_section = lv_obj_create(settings_card);
    lv_obj_set_size(brightness_section, 710, 95);
    lv_obj_align(brightness_section, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_style_bg_color(brightness_section, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(brightness_section, 0, 0);
    lv_obj_set_style_radius(brightness_section, 8, 0);
    lv_obj_clear_flag(brightness_section, LV_OBJ_FLAG_SCROLLABLE);

    lbl_brightness = lv_label_create(brightness_section);
    lv_label_set_text(lbl_brightness, "SCREEN BRIGHTNESS");
    lv_obj_set_style_text_color(lbl_brightness, lv_color_hex(0xFFB300), 0);
    lv_obj_set_style_text_font(lbl_brightness, current_font_medium, 0);
    lv_obj_align(lbl_brightness, LV_ALIGN_TOP_LEFT, 10, -4);

    lv_obj_t* slider = lv_slider_create(brightness_section);
    lv_obj_set_size(slider, 660, 22);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_slider_set_range(slider, 20, 255); 
    lv_slider_set_value(slider, 200, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- TEMA SECIMI ---
    lv_obj_t* theme_section = lv_obj_create(settings_card);
    lv_obj_set_size(theme_section, 710, 100);
    lv_obj_align(theme_section, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_set_style_bg_color(theme_section, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(theme_section, 0, 0);
    lv_obj_set_style_radius(theme_section, 8, 0);
    lv_obj_clear_flag(theme_section, LV_OBJ_FLAG_SCROLLABLE);

    lbl_theme = lv_label_create(theme_section);
    lv_label_set_text(lbl_theme, "THEME");
    lv_obj_set_style_text_color(lbl_theme, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_text_font(lbl_theme, current_font_medium, 0);
    lv_obj_align(lbl_theme, LV_ALIGN_TOP_LEFT, 10, -4);

    // Light label (switch'in solunda)
    lv_obj_t* light_label = lv_label_create(theme_section);
    lv_label_set_text(light_label, "Light");
    lv_obj_set_style_text_color(light_label, lv_color_hex(0xE0E0E0), 0);
    lv_obj_align(light_label, LV_ALIGN_BOTTOM_LEFT, 20, -18);

    theme_switch = lv_switch_create(theme_section);
    lv_obj_align(theme_switch, LV_ALIGN_BOTTOM_LEFT, 80, -15);
    lv_obj_add_state(theme_switch, LV_STATE_CHECKED); // Koyu tema aktif
    lv_obj_add_event_cb(theme_switch, [](lv_event_t * e){
        lv_event_code_t code = lv_event_get_code(e);
        if(code == LV_EVENT_VALUE_CHANGED) {
            is_dark_theme = lv_obj_has_state(theme_switch, LV_STATE_CHECKED);
            Serial.printf("[UI] Theme changed: %s\n", is_dark_theme ? "Dark" : "Light");
            apply_theme(); // Temayı uygula
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Dark label (switch'in sağında)
    lv_obj_t* theme_status = lv_label_create(theme_section);
    lv_label_set_text(theme_status, "Dark");
    lv_obj_set_style_text_color(theme_status, lv_color_hex(0xE0E0E0), 0);
    lv_obj_align(theme_status, LV_ALIGN_BOTTOM_LEFT, 140, -18);

    // --- TEXT SIZE ---
    lv_obj_t* text_section = lv_obj_create(settings_card);
    lv_obj_set_size(text_section, 710, 120);
    lv_obj_align(text_section, LV_ALIGN_TOP_MID, 0, 260);
    lv_obj_set_style_bg_color(text_section, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(text_section, 0, 0);
    lv_obj_set_style_radius(text_section, 8, 0);
    lv_obj_clear_flag(text_section, LV_OBJ_FLAG_SCROLLABLE);

    lbl_text_size = lv_label_create(text_section);
    lv_label_set_text(lbl_text_size, "TEXT SIZE");
    lv_obj_set_style_text_color(lbl_text_size, lv_color_hex(0xFD6D4E), 0);
    lv_obj_set_style_text_font(lbl_text_size, current_font_medium, 0);
    lv_obj_align(lbl_text_size, LV_ALIGN_TOP_LEFT, 10, -4);

    text_size_dropdown = lv_dropdown_create(text_section);
    lv_dropdown_set_options(text_size_dropdown, "Small\nMedium\nLarge");
    lv_dropdown_set_selected(text_size_dropdown, 1); // Medium
    lv_obj_set_width(text_size_dropdown, 200);
    lv_obj_align(text_size_dropdown, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(text_size_dropdown, [](lv_event_t * e){
        lv_event_code_t code = lv_event_get_code(e);
        if(code == LV_EVENT_VALUE_CHANGED) {
            text_size_mode = lv_dropdown_get_selected(text_size_dropdown);
            Serial.printf("[UI] Text size changed: %d\n", text_size_mode);
            update_text_sizes();
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Back butonu (Sol üst köşe, küçük ok)
    lv_obj_t* btn_back = lv_btn_create(ui_Screen_Settings);
    lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn_back, 50, 50);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 15, 15);
    lv_obj_set_style_radius(btn_back, 25, 0); // Yuvarlak buton
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x2D2D2D), 0);
    lv_obj_add_event_cb(btn_back, back_to_main_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "<");
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_20, 0);
    lv_obj_center(label_back);
}

void ui_Screen_Menu_init(void)
{
    ui_Screen_Menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_Screen_Menu, lv_color_hex(0x111111), 0);
    lv_obj_clear_flag(ui_Screen_Menu, LV_OBJ_FLAG_SCROLLABLE); 

    // --- MENÜ İÇERİĞİ KARTI ---
    lv_obj_t* menu_card = lv_obj_create(ui_Screen_Menu);
    lv_obj_add_style(menu_card, &style_card_bg, 0);
    lv_obj_set_size(menu_card, 650, 420);
    lv_obj_center(menu_card);
    lv_obj_clear_flag(menu_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(menu_card, 20, 0);

    // Başlık
    lbl_menu_title = lv_label_create(menu_card);
    lv_label_set_text(lbl_menu_title, "SISTEM DIAGNOSTIK"); 
    lv_obj_set_style_text_color(lbl_menu_title, lv_color_hex(0x00BCD4), 0);
    lv_obj_set_style_text_font(lbl_menu_title, current_font_large, 0);
    lv_obj_align(lbl_menu_title, LV_ALIGN_TOP_MID, 0, 0);

    // Bağlantı Durumu - Kart
    lv_obj_t* conn_card = lv_obj_create(menu_card);
    lv_obj_set_size(conn_card, 600, 60);
    lv_obj_align(conn_card, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_bg_color(conn_card, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(conn_card, 0, 0);
    lv_obj_set_style_radius(conn_card, 8, 0);
    lv_obj_clear_flag(conn_card, LV_OBJ_FLAG_SCROLLABLE);

    lbl_conn_title = lv_label_create(conn_card);
    lv_label_set_text(lbl_conn_title, "BAGLANTI DURUMU");
    lv_obj_set_style_text_color(lbl_conn_title, lv_color_hex(0xFFB300), 0);
    lv_obj_set_style_text_font(lbl_conn_title, current_font_medium, 0);
    lv_obj_align(lbl_conn_title, LV_ALIGN_TOP_LEFT, 10, -9);

    lbl_conn_status = lv_label_create(conn_card);
    lv_label_set_text(lbl_conn_status, "DURUM: ...");
    lv_obj_set_width(lbl_conn_status, 580);
    lv_obj_set_style_text_color(lbl_conn_status, lv_color_hex(0xE0E0E0), 0);
    lv_obj_align(lbl_conn_status, LV_ALIGN_TOP_LEFT, 10, 12);

    // Kamera Durumu - Kart
    lv_obj_t* cam_card = lv_obj_create(menu_card);
    lv_obj_set_size(cam_card, 600, 60);
    lv_obj_align(cam_card, LV_ALIGN_TOP_MID, 0, 110);
    lv_obj_set_style_bg_color(cam_card, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(cam_card, 0, 0);
    lv_obj_set_style_radius(cam_card, 8, 0);
    lv_obj_clear_flag(cam_card, LV_OBJ_FLAG_SCROLLABLE);

    lbl_cam_title = lv_label_create(cam_card);
    lv_label_set_text(lbl_cam_title, "KAMERA");
    lv_obj_set_style_text_color(lbl_cam_title, lv_color_hex(0xFD6D4E), 0);
    lv_obj_set_style_text_font(lbl_cam_title, current_font_medium, 0);
    lv_obj_align(lbl_cam_title, LV_ALIGN_TOP_LEFT, 10, -9);

    lbl_camera_status = lv_label_create(cam_card);
    lv_label_set_text(lbl_camera_status, "DURUM: KONTROL EDILIYOR...");
    lv_obj_set_width(lbl_camera_status, 580);
    lv_obj_set_style_text_color(lbl_camera_status, lv_color_hex(0xE0E0E0), 0);
    lv_obj_align(lbl_camera_status, LV_ALIGN_TOP_LEFT, 10, 12);

    // Performans - Kart
    lv_obj_t* perf_card = lv_obj_create(menu_card);
    lv_obj_set_size(perf_card, 600, 100);
    lv_obj_align(perf_card, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_set_style_bg_color(perf_card, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(perf_card, 0, 0);
    lv_obj_set_style_radius(perf_card, 8, 0);
    lv_obj_clear_flag(perf_card, LV_OBJ_FLAG_SCROLLABLE);

    lbl_perf_title = lv_label_create(perf_card);
    lv_label_set_text(lbl_perf_title, "PERFORMANS");
    lv_obj_set_style_text_color(lbl_perf_title, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_text_font(lbl_perf_title, current_font_medium, 0);
    lv_obj_align(lbl_perf_title, LV_ALIGN_TOP_LEFT, 10, -9);

    lbl_fps = lv_label_create(perf_card);
    lv_label_set_text(lbl_fps, "FPS: 0");
    lv_obj_set_style_text_color(lbl_fps, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(lbl_fps, current_font_small, 0);
    lv_obj_align(lbl_fps, LV_ALIGN_TOP_LEFT, 10, 14);

    lbl_heap = lv_label_create(perf_card);
    lv_label_set_text(lbl_heap, "Free Heap: ...");
    lv_obj_set_style_text_color(lbl_heap, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(lbl_heap, current_font_small, 0);
    lv_obj_align(lbl_heap, LV_ALIGN_TOP_LEFT, 10, 34);

    lbl_frame_stats = lv_label_create(perf_card);
    lv_label_set_text(lbl_frame_stats, "Toplam Frame: 0");
    lv_obj_set_style_text_color(lbl_frame_stats, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(lbl_frame_stats, current_font_small, 0);
    lv_obj_align(lbl_frame_stats, LV_ALIGN_TOP_LEFT, 10, 54);

    // Refresh butonu
    btn_refresh = lv_btn_create(menu_card);
    lv_obj_set_size(btn_refresh, 140, 45);
    lv_obj_align(btn_refresh, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_refresh, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(btn_refresh, 8, 0);
    lv_obj_add_event_cb(btn_refresh, [](lv_event_t * e){
        lv_event_code_t code = lv_event_get_code(e);
        if(code == LV_EVENT_CLICKED){
            Serial.println("[UI] Manual refresh requested");
        }
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_r = lv_label_create(btn_refresh);
    lv_label_set_text(lbl_r, "YENILE");
    lv_obj_set_style_text_color(lbl_r, lv_color_white(), 0);
    lv_obj_center(lbl_r);

    // Back butonu (Sol üst köşe, küçük ok)
    lv_obj_t* btn_back = lv_btn_create(ui_Screen_Menu);
    lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn_back, 50, 50);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 15, 15);
    lv_obj_set_style_radius(btn_back, 25, 0); // Yuvarlak buton
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x2D2D2D), 0);
    lv_obj_add_event_cb(btn_back, back_to_main_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "<");
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_20, 0);
    lv_obj_center(label_back);

    // Periodik diagnostics update timer (2 saniye - reduced frequency)
    lv_timer_create([](lv_timer_t * t){
        (void)t;
        
        char buf[256];
        
        // WiFi Connection status (static, update once)
        static bool wifi_updated = false;
        if(!wifi_updated) {
            if(WiFi.isConnected()) {
                snprintf(buf, sizeof(buf), "WiFi: %s | IP: %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
            } else {
                IPAddress apip = WiFi.softAPIP();
                snprintf(buf, sizeof(buf), "AP: %s | IP: %s", WiFi.softAPSSID().c_str(), apip.toString().c_str());
            }
            lv_label_set_text(lbl_conn_status, buf);
            wifi_updated = true;
        }

        // Kamera durumu kontrolü
        static uint32_t last_camera_frames = 0;
        static unsigned long last_camera_check = 0;
        unsigned long now = millis();
        
        if(now - last_camera_check >= 10000) {
            uint32_t current_frames = ui_total_frames_counter;
            if(current_frames > last_camera_frames) {
                snprintf(buf, sizeof(buf), " AKTIF | SON FRAME: #%u", (unsigned)current_frames);
                lv_obj_set_style_text_color(lbl_camera_status, lv_color_hex(0x4CAF50), 0);
            } else {
                snprintf(buf, sizeof(buf), "VERI YOK | BAGLANTIYI KONTROL EDIN");
                lv_obj_set_style_text_color(lbl_camera_status, lv_color_hex(0xFF5722), 0);
            }
            lv_label_set_text(lbl_camera_status, buf);
            last_camera_frames = current_frames;
            last_camera_check = now;
        }

        // FPS hesaplama
        static uint32_t last_frames = 0;
        static unsigned long last_ms = 0;
        static int fps_val = 0;
        
        if(now - last_ms >= 2000) { // Update every 2 seconds
            uint32_t total_frames = ui_total_frames_counter;
            fps_val = (total_frames - last_frames) / 2; // Divide by 2 for 2-second interval
            last_frames = total_frames;
            last_ms = now;
            
            snprintf(buf, sizeof(buf), "FPS: %d", fps_val);
            lv_label_set_text(lbl_fps, buf);

            // Heap (update less frequently)
            size_t free_heap = esp_get_free_heap_size();
            float heap_kb = free_heap / 1024.0;
            snprintf(buf, sizeof(buf), "Free Heap: %.1f KB", heap_kb);
            lv_label_set_text(lbl_heap, buf);

            // Frame stats
            snprintf(buf, sizeof(buf), "Toplam Frame: %u", (unsigned)ui_total_frames_counter);
            lv_label_set_text(lbl_frame_stats, buf);
        }

    }, 2000, NULL);
}

void ui_Screen_Main_init(void)
{
    ui_Screen_Main = lv_scr_act();
    lv_obj_set_style_bg_color(ui_Screen_Main, lv_color_hex(0x111111), 0); 
    lv_obj_clear_flag(ui_Screen_Main, LV_OBJ_FLAG_SCROLLABLE); 

    // Sol sidebar (tek parça, açık gri)
    ui_left_bar = lv_obj_create(ui_Screen_Main);
    lv_obj_set_size(ui_left_bar, 90, 480);
    lv_obj_align(ui_left_bar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_clear_flag(ui_left_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_left_bar, lv_color_hex(0x3A3A3A), 0); // Açık gri
    lv_obj_set_style_bg_opa(ui_left_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ui_left_bar, 0, 0);
    lv_obj_set_style_pad_all(ui_left_bar, 0, 0);
    
    lv_obj_t* menu_btn = lv_btn_create(ui_left_bar);
    lv_obj_add_style(menu_btn, &style_btn_sidebar, 0);
    lv_obj_set_size(menu_btn, 70, 60);
    lv_obj_align(menu_btn, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_add_event_cb(menu_btn, general_event_handler, LV_EVENT_CLICKED, (void*)"Menu");
    
    lbl_menu = lv_label_create(menu_btn);
    lv_label_set_text(lbl_menu, "MENU");
    lv_obj_set_style_text_font(lbl_menu, current_font_small, 0);
    lv_obj_center(lbl_menu);

    lv_obj_t* settings_btn = lv_btn_create(ui_left_bar);
    lv_obj_add_style(settings_btn, &style_btn_sidebar, 0);
    lv_obj_set_size(settings_btn, 70, 60);
    lv_obj_align(settings_btn, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_add_event_cb(settings_btn, general_event_handler, LV_EVENT_CLICKED, (void*)"Ayarlar");

    lbl_settings = lv_label_create(settings_btn);
    lv_label_set_text(lbl_settings, "AYAR");
    lv_obj_set_style_text_font(lbl_settings, current_font_small, 0);
    lv_obj_center(lbl_settings);

    ui_main_panel = lv_obj_create(ui_Screen_Main);
    lv_obj_set_size(ui_main_panel, 710, 480);
    lv_obj_align(ui_main_panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(ui_main_panel, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_width(ui_main_panel, 0, 0);
    lv_obj_clear_flag(ui_main_panel, LV_OBJ_FLAG_SCROLLABLE); 
    
    // --- CAMERA STATUS LABEL (ABOVE PREVIEW) ---
    ui_response_label = lv_label_create(ui_main_panel);
    lv_label_set_text(ui_response_label, "Camera Initializing...");
    lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFFB300), 0); 
    lv_obj_set_style_text_font(ui_response_label, current_font_large, 0);
    lv_obj_set_style_bg_color(ui_response_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ui_response_label, LV_OPA_70, 0);
    lv_obj_set_style_pad_all(ui_response_label, 10, 0);
    lv_obj_set_style_radius(ui_response_label, 6, 0);
    lv_obj_align(ui_response_label, LV_ALIGN_TOP_MID, 0, 5);
    
    // --- ILAB LOGO (SAĞ ÜST KÖŞE) ---
    // External logo declaration (from ilablogo.c)
    extern const lv_img_dsc_t ilablogo;
    
    lv_obj_t *logo_img = lv_img_create(ui_main_panel);
    lv_img_set_src(logo_img, &ilablogo);
    lv_img_set_zoom(logo_img, 128);  // 128 = %50 (256 = %100)
    lv_obj_align(logo_img, LV_ALIGN_TOP_RIGHT, 15, -30);
    
    // Logo altına ILAB yazısı ekle
    lv_obj_t *logo_label = lv_label_create(ui_main_panel);
    lv_label_set_text(logo_label, "ILAB");
    lv_obj_set_style_text_color(logo_label, lv_color_hex(0x00BCD4), 0);
    lv_obj_set_style_text_font(logo_label, &lv_font_montserrat_14, 0);
    lv_obj_align(logo_label, LV_ALIGN_TOP_RIGHT, -35, 65);  // Logo altında, daha aşağıda
    
    Serial.println("[UI] ILAB logo image created");

    
    // --- KAMERA ÖNİZLEME KARTI ---
    ui_img_panel = lv_obj_create(ui_main_panel);
    lv_obj_add_style(ui_img_panel, &style_card_bg, 0); 
    lv_obj_set_size(ui_img_panel, 680, 360);
    lv_obj_align(ui_img_panel, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_clear_flag(ui_img_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_shadow_width(ui_img_panel, 15, 0);
    lv_obj_set_style_shadow_color(ui_img_panel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(ui_img_panel, LV_OPA_40, 0);
    
    // LVGL struct'a değerleri ata (C++ Uyumlu Yöntem)
    my_img_dsc.header.always_zero = 0;
    my_img_dsc.header.w = IMG_W;
    my_img_dsc.header.h = IMG_H;
    my_img_dsc.data_size = IMG_W * IMG_H * 2;
    my_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    my_img_dsc.data = frame_buffer; // RAW Buffer adı

    // Image Widget Oluştur ve göster
    ui_camera_view = lv_img_create(ui_img_panel);
    lv_img_set_src(ui_camera_view, &my_img_dsc);
    lv_obj_align(ui_camera_view, LV_ALIGN_CENTER, 0, 0); 

    lbl_res = lv_label_create(ui_img_panel);
    lv_label_set_text(lbl_res, "160x120");
    lv_obj_set_style_text_color(lbl_res, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lbl_res, current_font_small, 0);
    lv_obj_align(lbl_res, LV_ALIGN_BOTTOM_MID, 0, -8);

    // --- BUTTON CONTAINER (ALT KISIM) ---
    lv_obj_t *btn_container = lv_obj_create(ui_main_panel);
    lv_obj_set_size(btn_container, 680, 80);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // --- FOCUS BUTTON ---
    lv_obj_t *btn_focus = lv_btn_create(btn_container);
    lv_obj_set_size(btn_focus, 280, 65);
    lv_obj_add_style(btn_focus, &style_btn_blue, 0);
    lv_obj_set_style_shadow_width(btn_focus, 10, 0);
    lv_obj_set_style_shadow_color(btn_focus, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_shadow_opa(btn_focus, LV_OPA_30, 0);
    lv_obj_add_event_cb(btn_focus, general_event_handler, LV_EVENT_CLICKED, (void*)"Focus");
    
    lbl_focus = lv_label_create(btn_focus);
    lv_label_set_text(lbl_focus, "FOCUS");
    lv_obj_set_style_text_font(lbl_focus, current_font_xlarge, 0);
    lv_obj_center(lbl_focus);

    // --- CAPTURE BUTTON ---
    lv_obj_t *btn_capture = lv_btn_create(btn_container);
    lv_obj_set_size(btn_capture, 280, 65);
    lv_obj_add_style(btn_capture, &style_btn_orange, 0);
    lv_obj_set_style_shadow_width(btn_capture, 10, 0);
    lv_obj_set_style_shadow_color(btn_capture, lv_color_hex(0xFD6D4E), 0);
    lv_obj_set_style_shadow_opa(btn_capture, LV_OPA_30, 0);
    lv_obj_add_event_cb(btn_capture, general_event_handler, LV_EVENT_CLICKED, (void*)"Capture");
    
    lbl_capture = lv_label_create(btn_capture);
    lv_label_set_text(lbl_capture, "CAPTURE");
    lv_obj_set_style_text_font(lbl_capture, current_font_xlarge, 0);
    lv_obj_center(lbl_capture);

    // Camera status update timer (check every 3 seconds, reduced frequency)
    lv_timer_create([](lv_timer_t * t){
        (void)t;
        unsigned long now = millis();
        
        // Check if frames are being received
        if(ui_total_frames_counter > last_frame_count) {
            if(!camera_active) {
                camera_active = true;
                lv_label_set_text(ui_response_label, "Camera Active");
                lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x4CAF50), 0);
            }
            last_frame_count = ui_total_frames_counter;
            last_camera_update = now;
        } else {
            // No new frames for 8 seconds
            if(camera_active && (now - last_camera_update > 8000)) {
                camera_active = false;
                lv_label_set_text(ui_response_label, "Camera Disconnected");
                lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFF5722), 0);
            }
        }
    }, 3000, NULL);
}

// -----------------------------------------------------------------
// UI Başlatma fonksiyonu
// -----------------------------------------------------------------
void ui_init(void)
{
    create_styles();
    update_text_sizes(); // Initialize font sizes before creating screens
    ui_Screen_Main_init();     
    ui_Screen_Settings_init(); 
    ui_Screen_Menu_init();     
    lv_scr_load(ui_Screen_Main); 
}

// -----------------------------------------------------------------
// ARKA PLAN GÖREVLERİ ve STATISTICS
// -----------------------------------------------------------------

// notify: frame alındığında çağır
inline void notify_frame_received_for_ui() {
    ui_total_frames_counter++;
}

// settings background tasks (loop içinde çağır)
inline void settings_background_tasks() {
    // Burada UI üzerinden tetiklenen flag'leri kontrol edip işleyeceksiniz.
}

// -----------------------------------------------------------------
#endif
