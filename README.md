# CrowPanel 7.0" ESP32-S3 HMI Display Performance Template

[Türkçe](#türkçe) | [English](#english)

---

<a name="türkçe"></a>
## 🇹🇷 Türkçe

Bu proje, **CrowPanel 7.0 inç (ESP32-S3)** tabanlı endüstriyel ekranlar için optimize edilmiş, yüksek performanslı bir HMI (İnsan-Makine Arayüzü) şablonudur. Proje, özellikle düşük gecikmeli görüntü aktarımı ve modern bir kullanıcı arayüzü (LVGL 8.x) sunmak amacıyla tasarlanmıştır.

---

### 🖼️ Donanım Görseli
![CrowPanel 7.0 ESP32 HMI Display](./images/screen.jpg)

---

### 🚀 Öne Çıkan Özellikler

- **Yüksek Performanslı Görüntü Aktarımı:** USB-Serial üzerinden **2 Mbps** hızında, JPEG kodlanmış karelerin (480x320) gerçek zamanlı akışı.
- **Modern Arayüz:** LVGL 8.x kütüphanesi kullanılarak tasarlanmış, karanlık tema destekli kart (Card) yapısı.
- **Düşük Gecikme:** PSRAM (OPI) üzerinden doğrudan dekoder ve DMA destekli ekran yenileme.
- **Gelişmiş Donanım Kontrolü:** Odaklama (Autofocus), çekilen fotoğrafların galeriden izlenmesi ve sistem metrikleri takibi.
- **Modüler Yapı:** Kolayca özelleştirilebilir `ui.h` ve `gfx_conf.h` dosyaları.

---

### 📁 Proje Yapısı

| Klasör / Dosya | Açıklama |
|----------------|----------|
| `main.ino` | Ana kontrol akışı, seri haberleşme protokolü ve JPEG dekoder yönetimi. |
| `ui.h` | LVGL arayüz bileşenleri, stil tanımlamaları ve ekranlar arası geçişler. |
| `gfx_conf.h` | CrowPanel 7.0" (ST7701) için pin tanımlaları ve LovyanGFX konfigürasyonu. |
| `libraries/` | Bağımlı kütüphaneler (LVGL, LovyanGFX, GT911 Touch). |
| `ilablogo.c` | UI üzerinde yer alan kurumsal logo verisi (isteğe bağlı değiştirilebilir). |

---

### ⚙️ Gereksinimler & Kurulum

#### 1. Arduino IDE Yapılandırması
- **Board:** `ESP32S3 Dev Module`
- **USB CDC On Boot:** `Enabled`
- **Flash Mode:** `QIO 80MHz`
- **Partition Scheme:** `Huge APP (3MB No OTA/1MB SPIFFS)`
- **PSRAM:** `OPI PSRAM` (ÇOK ÖNEMLİ).

#### 2. Kütüphane Kurulumu
`libraries` klasörü içerisindeki kütüphaneleri Arduino kütüphane klasörünüze kopyalayın.

---

### 🤝 İş Birliği
Bu proje, **i-Lab** ile hazırlanmıştır. Daha fazla bilgi ve iletişim için:

🌐 **Web sitesi:** [pieilab.com](http://www.pieilab.com)  
🔗 **LinkedIn:** [PIE i-Lab](https://www.linkedin.com/company/pielabb)

---

### 👨‍💻 İletişim
**Metehan Günen** - [metehangnn@outlook.com](mailto:metehangnn@outlook.com)

---

<br>
<hr>
<br>

<a name="english"></a>
## 🇺🇸 English

This project is a high-performance HMI (Human-Machine Interface) template optimized for **CrowPanel 7.0 inch (ESP32-S3)** based industrial displays. It is designed to provide low-latency image streaming and a modern user interface (LVGL 8.x).

---

### 🖼️ Hardware Visual
![CrowPanel 7.0 ESP32 HMI Display](./images/screen.jpg)

---

### 🚀 Key Features

- **High-Performance Image Streaming:** Real-time 480x320 JPEG streaming at **2 Mbps** via USB-Serial.
- **Modern UI:** Card-based design with dark theme support using the LVGL 8.x library.
- **Low Latency:** Direct decoding via PSRAM (OPI) and DMA-supported display refresh.
- **Advanced Hardware Control:** Autofocus adjustment, gallery view for captured images, and real-time system metrics tracking.
- **Modular Design:** Easily customizable `ui.h` and `gfx_conf.h` files.

---

### 📁 Project Structure

| Folder / File | Description |
|----------------|----------|
| `main.ino` | Main control flow, serial communication protocol, and JPEG decoder management. |
| `ui.h` | LVGL interface components, style definitions, and screen transitions. |
| `gfx_conf.h` | Pin definitions and LovyanGFX configuration for CrowPanel 7.0" (ST7701). |
| `libraries/` | Dependent libraries (LVGL, LovyanGFX, GT911 Touch). |
| `ilablogo.c` | Corporate logo data used in the UI (optional and replaceable). |

---

### ⚙️ Requirements & Setup

#### 1. Arduino IDE Configuration
- **Board:** `ESP32S3 Dev Module`
- **USB CDC On Boot:** `Enabled`
- **Flash Mode:** `QIO 80MHz`
- **Partition Scheme:** `Huge APP (3MB No OTA/1MB SPIFFS)`
- **PSRAM:** `OPI PSRAM` (CRITICAL: Required for image processing).

#### 2. Library Installation
Copy the libraries inside the `libraries` folder to your Arduino libraries directory.

---

### 🤝 Collaboration
This project was prepared with **i-Lab**. For more information and contact:

🌐 **Website:** [pieilab.com](http://www.pieilab.com)  
🔗 **LinkedIn:** [PIE i-Lab](https://www.linkedin.com/company/pielabb)

---

### 👨‍💻 Contact
**Metehan Günen** - [metehangnn@outlook.com](mailto:metehangnn@outlook.com)

---
*This project was created to push the limits of the CrowPanel 7.0" display and provide developers with a ready-to-use HMI skeleton.*
