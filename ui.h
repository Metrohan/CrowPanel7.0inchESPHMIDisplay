#ifndef UI_H
#define UI_H

#include <lvgl.h>
#include <Arduino.h>

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
extern void slider_event_cb(lv_event_t * e);
extern void set_brightness(int value); 
// TJpg_Decoder ile ilgili externler kaldırıldı.

// -----------------------------------------------------------------
// 3. GLOBAL NESNELER
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

// -----------------------------------------------------------------
// STİLLER (KARANLIK TEMA ve KART YAPISI)
// -----------------------------------------------------------------
static lv_style_t style_sidebar;
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

    lv_style_init(&style_btn_sidebar);
    lv_style_set_bg_color(&style_btn_sidebar, lv_color_hex(0x282828));
    lv_style_set_text_color(&style_btn_sidebar, lv_color_white());
    lv_style_set_border_width(&style_btn_sidebar, 0);
    lv_style_set_radius(&style_btn_sidebar, 0); 

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
            capture_requested = true; 
        }
        else if (strcmp(btn_name, "Metrics") == 0) {
            lv_label_set_text(ui_response_label, "Metrikler Yukleniyor...");
        }
        else if (strcmp(btn_name, "Focus") == 0) {
            lv_label_set_text(ui_response_label, "Odaklaniliyor...");
        }
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
// EKRAN INIT FONKSİYONLARI
// -----------------------------------------------------------------

void ui_Screen_Settings_init(void)
{
    ui_Screen_Settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_Screen_Settings, lv_color_hex(0x111111), 0);
    lv_obj_clear_flag(ui_Screen_Settings, LV_OBJ_FLAG_SCROLLABLE); 

    lv_obj_t* settings_card = lv_obj_create(ui_Screen_Settings);
    lv_obj_add_style(settings_card, &style_card_bg, 0);
    lv_obj_set_size(settings_card, 600, 300);
    lv_obj_center(settings_card);
    lv_obj_clear_flag(settings_card, LV_OBJ_FLAG_SCROLLABLE); 

    lv_obj_t* label_title = lv_label_create(settings_card);
    lv_label_set_text(label_title, "AYARLAR");
    lv_obj_set_style_text_color(label_title, lv_color_white(), 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_obj_t* slider_label = lv_label_create(settings_card);
    lv_label_set_text(slider_label, "Ekran Parlakligi");
    lv_obj_set_style_text_color(slider_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(slider_label, LV_ALIGN_TOP_LEFT, 20, 60);

    lv_obj_t* slider = lv_slider_create(settings_card);
    lv_obj_set_size(slider, 550, 20);
    lv_obj_align_to(slider, slider_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    lv_slider_set_range(slider, 0, 255); 
    lv_slider_set_value(slider, 200, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t* btn_back = lv_btn_create(ui_Screen_Settings);
    lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn_back, 100, 50);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(btn_back, back_to_main_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "GERI");
    lv_obj_center(label_back);
}

void ui_Screen_Menu_init(void)
{
    ui_Screen_Menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_Screen_Menu, lv_color_hex(0x111111), 0);
    lv_obj_clear_flag(ui_Screen_Menu, LV_OBJ_FLAG_SCROLLABLE); 

    // --- MENÜ İÇERİĞİ KARTI (DOLDURULDU) ---
    lv_obj_t* menu_card = lv_obj_create(ui_Screen_Menu);
    lv_obj_add_style(menu_card, &style_card_bg, 0);
    lv_obj_set_size(menu_card, 600, 300);
    lv_obj_center(menu_card);
    lv_obj_set_flex_flow(menu_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu_card, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(menu_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(menu_card);
    lv_label_set_text(title, "SISTEM DIAGNOSTIK"); 
    lv_obj_set_style_text_color(title, lv_color_hex(0x00BCD4), 0); 
    
    lv_obj_t* diag1 = lv_label_create(menu_card);
    lv_label_set_text(diag1, "USB HIZI: 2.0 Mbps");
    lv_obj_set_style_text_color(diag1, lv_color_white(), 0);

    lv_obj_t* diag2 = lv_label_create(menu_card);
    lv_label_set_text(diag2, "KAMERA DURUMU: OK");
    lv_obj_set_style_text_color(diag2, lv_color_hex(0x4CAF50), 0);

    lv_obj_t* btn_back = lv_btn_create(ui_Screen_Menu);
    lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn_back, 100, 50);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(btn_back, back_to_main_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "GERI");
    lv_obj_center(label_back);
}

void ui_Screen_Main_init(void)
{
    ui_Screen_Main = lv_scr_act();
    lv_obj_set_style_bg_color(ui_Screen_Main, lv_color_hex(0x111111), 0); 
    lv_obj_clear_flag(ui_Screen_Main, LV_OBJ_FLAG_SCROLLABLE); 

    ui_left_bar = lv_obj_create(ui_Screen_Main);
    lv_obj_add_style(ui_left_bar, &style_sidebar, 0);
    lv_obj_set_size(ui_left_bar, 80, 480);
    lv_obj_align(ui_left_bar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_clear_flag(ui_left_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* menu_btn = lv_btn_create(ui_left_bar);
    lv_obj_add_style(menu_btn, &style_btn_sidebar, 0);
    lv_obj_set_size(menu_btn, 70, 50);
    lv_obj_align(menu_btn, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_add_event_cb(menu_btn, general_event_handler, LV_EVENT_CLICKED, (void*)"Menu");
    
    lv_obj_t* menu_label = lv_label_create(menu_btn);
    lv_label_set_text(menu_label, "Menu");
    lv_obj_center(menu_label);

    lv_obj_t* settings_btn = lv_btn_create(ui_left_bar);
    lv_obj_add_style(settings_btn, &style_btn_sidebar, 0);
    lv_obj_set_size(settings_btn, 70, 50);
    lv_obj_align(settings_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(settings_btn, general_event_handler, LV_EVENT_CLICKED, (void*)"Ayarlar");

    lv_obj_t* settings_label = lv_label_create(settings_btn);
    lv_label_set_text(settings_label, "Ayar");
    lv_obj_center(settings_label);

    ui_main_panel = lv_obj_create(ui_Screen_Main);
    lv_obj_set_size(ui_main_panel, 720, 480);
    lv_obj_align(ui_main_panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(ui_main_panel, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_width(ui_main_panel, 0, 0);
    lv_obj_clear_flag(ui_main_panel, LV_OBJ_FLAG_SCROLLABLE); 
    
    // --- BUTON KARTLARI (Kapsayıcı) ---
    ui_btn_panel = lv_obj_create(ui_main_panel);
    lv_obj_set_size(ui_btn_panel, 700, 100);
    lv_obj_align(ui_btn_panel, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_flex_flow(ui_btn_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_btn_panel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(ui_btn_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_btn_panel, 0, 0);
    lv_obj_clear_flag(ui_btn_panel, LV_OBJ_FLAG_SCROLLABLE);

    create_ui_button(ui_btn_panel, "CAPTURE", "Capture", &style_btn_orange);
    create_ui_button(ui_btn_panel, "METRICS", "Metrics", &style_btn_green);
    create_ui_button(ui_btn_panel, "FOCUS", "Focus", &style_btn_blue);

    // --- KAMERA ÖNİZLEME KARTI ---
    ui_img_panel = lv_obj_create(ui_main_panel);
    lv_obj_add_style(ui_img_panel, &style_card_bg, 0); 
    lv_obj_set_size(ui_img_panel, 700, 340);
    lv_obj_align(ui_img_panel, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_clear_flag(ui_img_panel, LV_OBJ_FLAG_SCROLLABLE);
    
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
    lv_obj_center(ui_camera_view); 

    // --- BİLDİRİM ETİKETİ (SAĞ ÜST KÖŞEYE TAŞINDI) ---
    ui_response_label = lv_label_create(ui_img_panel);
    lv_label_set_text(ui_response_label, "Camera Ready");
    lv_obj_set_style_text_color(ui_response_label, lv_color_white(), 0);
    lv_obj_align(ui_response_label, LV_ALIGN_TOP_RIGHT, -10, 10); 

    lv_obj_t *res_label = lv_label_create(ui_img_panel);
    lv_label_set_text(res_label, "RES: 160x120 (RAW)");
    lv_obj_set_style_text_color(res_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(res_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
}

void ui_init(void)
{
    create_styles();
    ui_Screen_Main_init();     
    ui_Screen_Settings_init(); 
    ui_Screen_Menu_init();     
    lv_scr_load(ui_Screen_Main); 
}

#endif