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
extern bool capture_requested;
extern bool capture_immediate;  // New: immediate capture flag

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
static lv_obj_t *lbl_camera_status = nullptr;
static lv_obj_t *lbl_fps = nullptr;
static lv_obj_t *lbl_heap = nullptr;
static lv_obj_t *lbl_frame_stats = nullptr;
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
static lv_obj_t *lbl_cam_title = nullptr;
static lv_obj_t *lbl_perf_title = nullptr;

// Gallery UI elements
static lv_obj_t *gallery_card = nullptr;
static lv_obj_t *lbl_gallery_title = nullptr;
static lv_obj_t *lbl_gallery_count = nullptr;
static lv_obj_t *gallery_thumb_container = nullptr;
static lv_obj_t *gallery_thumb_imgs[8] = {nullptr};
static lv_obj_t *btn_gallery_prev = nullptr;
static lv_obj_t *btn_gallery_next = nullptr;
static lv_obj_t *btn_gallery_refresh = nullptr;
static int16_t gallery_selected_index = -1;
static lv_obj_t *delete_msgbox = nullptr;

// Capture durum mesajı için kısa gecikmeli timer
static lv_timer_t *capture_status_timer = nullptr;

// Provided by main.ino (framed protocol)
extern void send_capture_request();
extern void send_focus_request();
extern void send_gallery_request();
extern void send_thumb_request(uint16_t index);
extern void send_delete_request(uint16_t index);

// Gallery data from main.ino
extern uint16_t gallery_image_count;
extern uint16_t gallery_total_size_mb;
extern uint8_t gallery_current_page;
extern bool gallery_thumb_valid[];
extern lv_img_dsc_t gallery_thumb_dsc[];

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
// CAPTURE DURUM TIMER CALLBACK (legacy; no longer used for completion)
// -----------------------------------------------------------------
static void capture_status_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    // 4K foto çekimi Pi tarafında büyük ihtimalle tamamlandı
    if(ui_response_label) {
        lv_label_set_text(ui_response_label, "4K FOTO CEKILDI (Raspberry Pi)");
        lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x4CAF50), 0); // Yesil
    }
    if(capture_status_timer) {
        lv_timer_del(capture_status_timer);
        capture_status_timer = nullptr;
    }
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
            // Auto-request gallery info when entering menu
            send_gallery_request();
        }
        else if (strcmp(btn_name, "Capture") == 0) {
            // Sadece Raspberry Pi'ya 4K capture komutu gönder, ESP32 tarafında
            // JPEG kopyalama / dosya yazma YAPMA (ekran glitch'ini engellemek için)
            lv_label_set_text(ui_response_label, "4K FOTO CEKILIYOR...");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFFB300), 0); // Turuncu-sarı

            // Timer ile tahmin etmiyoruz; Pi ACK gönderince main.ino UI'yı güncelleyecek.
            if(capture_status_timer) {
                lv_timer_del(capture_status_timer);
                capture_status_timer = nullptr;
            }

            send_capture_request();
        }
        else if (strcmp(btn_name, "Metrics") == 0) {
            lv_label_set_text(ui_response_label, "Metrikler Yukleniyor...");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x00E676), 0); // Yeşil
        }
        else if (strcmp(btn_name, "Focus") == 0) {
            lv_label_set_text(ui_response_label, "Odaklaniliyor...");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x00B0FF), 0); // Mavi
            send_focus_request();
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
    if(lbl_cam_title) lv_obj_set_style_text_font(lbl_cam_title, current_font_medium, 0);
    if(lbl_perf_title) lv_obj_set_style_text_font(lbl_perf_title, current_font_medium, 0);
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
// GALLERY UI CALLBACK FUNCTIONS (called from main.ino)
// -----------------------------------------------------------------
void update_gallery_info_ui(uint16_t count, uint16_t size_mb) {
    if(lbl_gallery_count) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d goruntu / %d MB", count, size_mb);
        lv_label_set_text(lbl_gallery_count, buf);
    }
    
    // Clear and hide current thumbnails first (to avoid "NO DATA")
    for(int i = 0; i < 4; i++) {
        if(gallery_thumb_imgs[i]) {
            lv_img_set_src(gallery_thumb_imgs[i], NULL);
            lv_obj_add_flag(gallery_thumb_imgs[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(gallery_thumb_imgs[i], lv_color_hex(0x333333), 0);
        }
    }
    
    // Request thumbnails for current page (indices are global, not slot-based)
    uint16_t start_idx = gallery_current_page * 4;
    for(int i = 0; i < 4 && (start_idx + i) < count; i++) {
        send_thumb_request(start_idx + i);
    }
    
    // Show placeholders for available slots (will be unhidden when thumb loads)
    for(int i = 0; i < 4; i++) {
        if(gallery_thumb_imgs[i] && (start_idx + i) < count) {
            lv_obj_clear_flag(gallery_thumb_imgs[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void update_gallery_thumb_ui(uint8_t visible_pos) {
    // visible_pos is 0-3 (position on screen)
    // Corresponding slot in buffer is (gallery_current_page * 4 + visible_pos) % 8
    uint8_t slot = (gallery_current_page * 4 + visible_pos) % 8;
    
    if(visible_pos < 4 && gallery_thumb_imgs[visible_pos] && gallery_thumb_valid[slot]) {
        lv_img_set_src(gallery_thumb_imgs[visible_pos], &gallery_thumb_dsc[slot]);
        lv_obj_clear_flag(gallery_thumb_imgs[visible_pos], LV_OBJ_FLAG_HIDDEN);
        Serial.printf("[UI] Updated visible pos %d from slot %d\n", visible_pos, slot);
    }
}

void on_delete_result(uint8_t status) {
    if(delete_msgbox) {
        lv_msgbox_close(delete_msgbox);
        delete_msgbox = nullptr;
    }
    
    if(ui_response_label) {
        if(status == 0) {
            lv_label_set_text(ui_response_label, "Goruntu silindi");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0x4CAF50), 0);
            
            // Clear all thumbnails - gallery_request will reload them
            for(int i = 0; i < 4; i++) {
                if(gallery_thumb_imgs[i]) {
                    lv_img_set_src(gallery_thumb_imgs[i], NULL);
                    lv_obj_add_flag(gallery_thumb_imgs[i], LV_OBJ_FLAG_HIDDEN);  // Hide to avoid "NO DATA"
                    lv_obj_set_style_bg_color(gallery_thumb_imgs[i], lv_color_hex(0x333333), 0);
                    lv_obj_set_style_border_width(gallery_thumb_imgs[i], 1, 0);
                    lv_obj_set_style_border_color(gallery_thumb_imgs[i], lv_color_hex(0x444444), 0);
                }
            }
            // Invalidate thumb cache
            for(int i = 0; i < 8; i++) {
                gallery_thumb_valid[i] = false;
            }
        } else {
            lv_label_set_text(ui_response_label, "Silme basarisiz!");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFF5722), 0);
        }
    }
    
    gallery_selected_index = -1;
}

static void gallery_thumb_click_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        int slot = (int)(intptr_t)lv_event_get_user_data(e);
        uint16_t actual_index = gallery_current_page * 4 + slot;
        
        if(actual_index < gallery_image_count) {
            gallery_selected_index = actual_index;
            Serial.printf("[UI] Selected image #%d\n", actual_index);
            
            // Highlight selected thumbnail
            for(int i = 0; i < 4; i++) {
                if(gallery_thumb_imgs[i]) {
                    if(i == slot) {
                        lv_obj_set_style_border_width(gallery_thumb_imgs[i], 3, 0);
                        lv_obj_set_style_border_color(gallery_thumb_imgs[i], lv_color_hex(0x2196F3), 0);
                    } else {
                        lv_obj_set_style_border_width(gallery_thumb_imgs[i], 1, 0);
                        lv_obj_set_style_border_color(gallery_thumb_imgs[i], lv_color_hex(0x444444), 0);
                    }
                }
            }
        }
    }
}

static void gallery_delete_msgbox_cb(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_current_target(e);
    const char * btn_text = lv_msgbox_get_active_btn_text(obj);
    
    if(btn_text && strcmp(btn_text, "SIL") == 0) {
        if(gallery_selected_index >= 0) {
            Serial.printf("[UI] Confirming delete of image #%d\n", gallery_selected_index);
            send_delete_request((uint16_t)gallery_selected_index);
        }
    } else {
        // Cancel
        gallery_selected_index = -1;
    }
    
    lv_msgbox_close(obj);
    delete_msgbox = nullptr;
}

static void show_delete_confirm_dialog() {
    if(gallery_selected_index < 0) {
        if(ui_response_label) {
            lv_label_set_text(ui_response_label, "Once goruntu secin");
            lv_obj_set_style_text_color(ui_response_label, lv_color_hex(0xFFB300), 0);
        }
        return;
    }
    
    static const char * btns[] = {"SIL", "IPTAL", ""};
    
    char msg[64];
    snprintf(msg, sizeof(msg), "Goruntu #%d silinsin mi?", gallery_selected_index);
    
    delete_msgbox = lv_msgbox_create(NULL, "ONAY", msg, btns, false);
    lv_obj_add_event_cb(delete_msgbox, gallery_delete_msgbox_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_center(delete_msgbox);
    
    // Style the delete button red
    lv_obj_t * btn_matrix = lv_msgbox_get_btns(delete_msgbox);
    lv_btnmatrix_set_btn_ctrl(btn_matrix, 0, LV_BTNMATRIX_CTRL_CHECKED);
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

    // --- MENÜ İÇERİĞİ KARTI (scrollable for more content) ---
    lv_obj_t* menu_card = lv_obj_create(ui_Screen_Menu);
    lv_obj_add_style(menu_card, &style_card_bg, 0);
    lv_obj_set_size(menu_card, 750, 460);
    lv_obj_align(menu_card, LV_ALIGN_CENTER, 0, 5);
    lv_obj_add_flag(menu_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(menu_card, 15, 0);
    lv_obj_set_scrollbar_mode(menu_card, LV_SCROLLBAR_MODE_AUTO);

    // Başlık
    lbl_menu_title = lv_label_create(menu_card);
    lv_label_set_text(lbl_menu_title, "SISTEM DIAGNOSTIK"); 
    lv_obj_set_style_text_color(lbl_menu_title, lv_color_hex(0x00BCD4), 0);
    lv_obj_set_style_text_font(lbl_menu_title, current_font_large, 0);
    lv_obj_align(lbl_menu_title, LV_ALIGN_TOP_MID, 0, 0);

    // === GALERI KARTI ===
    gallery_card = lv_obj_create(menu_card);
    lv_obj_set_size(gallery_card, 710, 150);
    lv_obj_align(gallery_card, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_color(gallery_card, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(gallery_card, 0, 0);
    lv_obj_set_style_radius(gallery_card, 8, 0);
    lv_obj_clear_flag(gallery_card, LV_OBJ_FLAG_SCROLLABLE);

    lbl_gallery_title = lv_label_create(gallery_card);
    lv_label_set_text(lbl_gallery_title, "GORUNTU GALERISI");
    lv_obj_set_style_text_color(lbl_gallery_title, lv_color_hex(0x9C27B0), 0);
    lv_obj_set_style_text_font(lbl_gallery_title, current_font_medium, 0);
    lv_obj_align(lbl_gallery_title, LV_ALIGN_TOP_LEFT, 10, -5);

    lbl_gallery_count = lv_label_create(gallery_card);
    lv_label_set_text(lbl_gallery_count, "Yukleniyor...");
    lv_obj_set_style_text_color(lbl_gallery_count, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lbl_gallery_count, current_font_small, 0);
    lv_obj_align(lbl_gallery_count, LV_ALIGN_TOP_RIGHT, -10, -2);

    // Thumbnail container (4 thumbnails per page: 80x60 each)
    gallery_thumb_container = lv_obj_create(gallery_card);
    lv_obj_set_size(gallery_thumb_container, 400, 75);
    lv_obj_align(gallery_thumb_container, LV_ALIGN_LEFT_MID, 5, 10);
    lv_obj_set_style_bg_opa(gallery_thumb_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gallery_thumb_container, 0, 0);
    lv_obj_clear_flag(gallery_thumb_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(gallery_thumb_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(gallery_thumb_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(gallery_thumb_container, 10, 0);

    // Create 4 thumbnail image slots
    for(int i = 0; i < 4; i++) {
        gallery_thumb_imgs[i] = lv_img_create(gallery_thumb_container);
        lv_obj_set_size(gallery_thumb_imgs[i], 80, 60);
        lv_obj_set_style_bg_color(gallery_thumb_imgs[i], lv_color_hex(0x333333), 0);
        lv_obj_set_style_bg_opa(gallery_thumb_imgs[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(gallery_thumb_imgs[i], 1, 0);
        lv_obj_set_style_border_color(gallery_thumb_imgs[i], lv_color_hex(0x444444), 0);
        lv_obj_set_style_radius(gallery_thumb_imgs[i], 4, 0);
        lv_obj_add_flag(gallery_thumb_imgs[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(gallery_thumb_imgs[i], gallery_thumb_click_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    }

    // Gallery control buttons (right side)
    lv_obj_t* gallery_btn_container = lv_obj_create(gallery_card);
    lv_obj_set_size(gallery_btn_container, 280, 75);
    lv_obj_align(gallery_btn_container, LV_ALIGN_RIGHT_MID, -5, 10);
    lv_obj_set_style_bg_opa(gallery_btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gallery_btn_container, 0, 0);
    lv_obj_clear_flag(gallery_btn_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(gallery_btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(gallery_btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Previous page button
    btn_gallery_prev = lv_btn_create(gallery_btn_container);
    lv_obj_set_size(btn_gallery_prev, 50, 40);
    lv_obj_set_style_bg_color(btn_gallery_prev, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_radius(btn_gallery_prev, 6, 0);
    lv_obj_add_event_cb(btn_gallery_prev, [](lv_event_t * e){
        if(lv_event_get_code(e) == LV_EVENT_CLICKED && gallery_current_page > 0) {
            gallery_current_page--;
            // Clear and hide current thumbs (avoid NO DATA)
            for(int i = 0; i < 4; i++) {
                gallery_thumb_valid[i] = false;
                if(gallery_thumb_imgs[i]) {
                    lv_img_set_src(gallery_thumb_imgs[i], NULL);
                    lv_obj_add_flag(gallery_thumb_imgs[i], LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_bg_color(gallery_thumb_imgs[i], lv_color_hex(0x333333), 0);
                    lv_obj_set_style_border_width(gallery_thumb_imgs[i], 1, 0);
                    lv_obj_set_style_border_color(gallery_thumb_imgs[i], lv_color_hex(0x444444), 0);
                }
            }
            gallery_selected_index = -1;
            // Request new thumbnails
            uint8_t start_idx = gallery_current_page * 4;
            for(int i = 0; i < 4 && (start_idx + i) < gallery_image_count; i++) {
                if(gallery_thumb_imgs[i]) lv_obj_clear_flag(gallery_thumb_imgs[i], LV_OBJ_FLAG_HIDDEN);
                send_thumb_request(start_idx + i);
            }
            Serial.printf("[UI] Gallery page: %d\n", gallery_current_page);
        }
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_prev = lv_label_create(btn_gallery_prev);
    lv_label_set_text(lbl_prev, "<");
    lv_obj_center(lbl_prev);

    // Refresh gallery button
    btn_gallery_refresh = lv_btn_create(gallery_btn_container);
    lv_obj_set_size(btn_gallery_refresh, 70, 40);
    lv_obj_set_style_bg_color(btn_gallery_refresh, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(btn_gallery_refresh, 6, 0);
    lv_obj_add_event_cb(btn_gallery_refresh, [](lv_event_t * e){
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            gallery_current_page = 0;
            gallery_selected_index = -1;
            // Clear and hide thumbnails (avoid NO DATA)
            for(int i = 0; i < 4; i++) {
                gallery_thumb_valid[i] = false;
                if(gallery_thumb_imgs[i]) {
                    lv_img_set_src(gallery_thumb_imgs[i], NULL);
                    lv_obj_add_flag(gallery_thumb_imgs[i], LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_bg_color(gallery_thumb_imgs[i], lv_color_hex(0x333333), 0);
                }
            }
            // Invalidate all cache slots
            for(int i = 0; i < 8; i++) {
                gallery_thumb_valid[i] = false;
            }
            send_gallery_request();
            Serial.println("[UI] Gallery refresh requested");
        }
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_ref = lv_label_create(btn_gallery_refresh);
    lv_label_set_text(lbl_ref, LV_SYMBOL_REFRESH);
    lv_obj_center(lbl_ref);

    // Next page button
    btn_gallery_next = lv_btn_create(gallery_btn_container);
    lv_obj_set_size(btn_gallery_next, 50, 40);
    lv_obj_set_style_bg_color(btn_gallery_next, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_radius(btn_gallery_next, 6, 0);
    lv_obj_add_event_cb(btn_gallery_next, [](lv_event_t * e){
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            uint8_t max_pages = (gallery_image_count + 3) / 4;
            if(gallery_current_page < max_pages - 1) {
                gallery_current_page++;
                // Clear and hide current thumbs (avoid NO DATA)
                for(int i = 0; i < 4; i++) {
                    gallery_thumb_valid[i] = false;
                    if(gallery_thumb_imgs[i]) {
                        lv_img_set_src(gallery_thumb_imgs[i], NULL);
                        lv_obj_add_flag(gallery_thumb_imgs[i], LV_OBJ_FLAG_HIDDEN);
                        lv_obj_set_style_bg_color(gallery_thumb_imgs[i], lv_color_hex(0x333333), 0);
                        lv_obj_set_style_border_width(gallery_thumb_imgs[i], 1, 0);
                        lv_obj_set_style_border_color(gallery_thumb_imgs[i], lv_color_hex(0x444444), 0);
                    }
                }
                gallery_selected_index = -1;
                // Request new thumbnails
                uint8_t start_idx = gallery_current_page * 4;
                for(int i = 0; i < 4 && (start_idx + i) < gallery_image_count; i++) {
                    if(gallery_thumb_imgs[i]) lv_obj_clear_flag(gallery_thumb_imgs[i], LV_OBJ_FLAG_HIDDEN);
                    send_thumb_request(start_idx + i);
                }
                Serial.printf("[UI] Gallery page: %d\n", gallery_current_page);
            }
        }
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_next = lv_label_create(btn_gallery_next);
    lv_label_set_text(lbl_next, ">");
    lv_obj_center(lbl_next);

    // Delete button
    lv_obj_t* btn_gallery_delete = lv_btn_create(gallery_btn_container);
    lv_obj_set_size(btn_gallery_delete, 50, 40);
    lv_obj_set_style_bg_color(btn_gallery_delete, lv_color_hex(0xF44336), 0);
    lv_obj_set_style_radius(btn_gallery_delete, 6, 0);
    lv_obj_add_event_cb(btn_gallery_delete, [](lv_event_t * e){
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            show_delete_confirm_dialog();
        }
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_del = lv_label_create(btn_gallery_delete);
    lv_label_set_text(lbl_del, LV_SYMBOL_TRASH);
    lv_obj_center(lbl_del);

    // === KAMERA DURUMU KARTI ===
    lv_obj_t* cam_card = lv_obj_create(menu_card);
    lv_obj_set_size(cam_card, 710, 55);
    lv_obj_align(cam_card, LV_ALIGN_TOP_MID, 0, 190);
    lv_obj_set_style_bg_color(cam_card, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(cam_card, 0, 0);
    lv_obj_set_style_radius(cam_card, 8, 0);
    lv_obj_clear_flag(cam_card, LV_OBJ_FLAG_SCROLLABLE);

    lbl_cam_title = lv_label_create(cam_card);
    lv_label_set_text(lbl_cam_title, "KAMERA");
    lv_obj_set_style_text_color(lbl_cam_title, lv_color_hex(0xFD6D4E), 0);
    lv_obj_set_style_text_font(lbl_cam_title, current_font_medium, 0);
    lv_obj_align(lbl_cam_title, LV_ALIGN_TOP_LEFT, 10, -5);

    lbl_camera_status = lv_label_create(cam_card);
    lv_label_set_text(lbl_camera_status, "DURUM: KONTROL EDILIYOR...");
    lv_obj_set_width(lbl_camera_status, 580);
    lv_obj_set_style_text_color(lbl_camera_status, lv_color_hex(0xE0E0E0), 0);
    lv_obj_align(lbl_camera_status, LV_ALIGN_TOP_LEFT, 10, 18);

    // === PERFORMANS KARTI ===
    lv_obj_t* perf_card = lv_obj_create(menu_card);
    lv_obj_set_size(perf_card, 710, 100);
    lv_obj_align(perf_card, LV_ALIGN_TOP_MID, 0, 255);
    lv_obj_set_style_bg_color(perf_card, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(perf_card, 0, 0);
    lv_obj_set_style_radius(perf_card, 8, 0);
    lv_obj_clear_flag(perf_card, LV_OBJ_FLAG_SCROLLABLE);

    lbl_perf_title = lv_label_create(perf_card);
    lv_label_set_text(lbl_perf_title, "PERFORMANS");
    lv_obj_set_style_text_color(lbl_perf_title, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_text_font(lbl_perf_title, current_font_medium, 0);
    lv_obj_align(lbl_perf_title, LV_ALIGN_TOP_LEFT, 10, -5);

    lbl_fps = lv_label_create(perf_card);
    lv_label_set_text(lbl_fps, "FPS: 0");
    lv_obj_set_style_text_color(lbl_fps, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(lbl_fps, current_font_small, 0);
    lv_obj_align(lbl_fps, LV_ALIGN_TOP_LEFT, 10, 18);

    lbl_heap = lv_label_create(perf_card);
    lv_label_set_text(lbl_heap, "Free Heap: ...");
    lv_obj_set_style_text_color(lbl_heap, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(lbl_heap, current_font_small, 0);
    lv_obj_align(lbl_heap, LV_ALIGN_TOP_LEFT, 10, 38);

    lbl_frame_stats = lv_label_create(perf_card);
    lv_label_set_text(lbl_frame_stats, "Toplam Frame: 0");
    lv_obj_set_style_text_color(lbl_frame_stats, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(lbl_frame_stats, current_font_small, 0);
    lv_obj_align(lbl_frame_stats, LV_ALIGN_TOP_LEFT, 10, 58);

    // Back butonu (Sol üst köşe, küçük ok)
    lv_obj_t* btn_back = lv_btn_create(ui_Screen_Menu);
    lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn_back, 50, 50);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 15, 15);
    lv_obj_set_style_radius(btn_back, 25, 0);
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
        unsigned long now = millis();

        // Kamera durumu kontrolü
        static uint32_t last_camera_frames = 0;
        static unsigned long last_camera_check = 0;
        
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
    
    // Image Widget Oluştur - main.ino tarafından jpeg_img_dsc ile doldurulacak
    ui_camera_view = lv_img_create(ui_img_panel);
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
