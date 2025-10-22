# CrowPanel 7.0" ESP32 HMI Display

### Bu paket, **SADECE CrowPanel 7.0 inç ESP32 tabanlı HMI dokunmatik ekran** için hazırlanmış bir örnek uygulama paketidir ve **i-Lab** için hazırlanmıştır.  
### Amaç, ESP32 tabanlı endüstriyel ekranlar ile **arayüz tasarımı**, **seri haberleşme**, ve **donanım kontrolü** süreçlerini kolaylaştırmaktır.

---

## 🖼️ Donanım Görseli
![CrowPanel 7.0 ESP32 HMI Display](./images/screen.jpg)

---

## 📁 Proje Yapısı

| Klasör / Dosya | Açıklama |
|----------------|----------|
| `main.ino` | ekranın kontrol akışını içerir. |
| `ui.h` | Ekran arayüzüyle ilgili görseller, fontlar ve UI kodları bulunur. |
| `ui_events.h` | Ekran arayüzüyle ilgili olan eylemler bu dosyada bulunur.|
| `gfx_conf.h` | Ekranın pinleri ve kalibrasyonun olduğu dosyadır. |
| `libraries/` | Harici kütüphaneler (örneğin, dokunmatik sürücüler, sensörler veya haberleşme modülleri) burada tutulur. (içindekileri Arduino'nun libraries klasörüne at.)|
| `README.md` | Proje hakkında genel bilgiler. |
| `images/` | Tanıtım ve dokümantasyon için kullanılan görseller. |

---

## ⚙️ Özellikler

- 7.0" dokunmatik HMI ekran desteği (ESP32 tabanlı)
- Seri haberleşme (UART) ile dış cihaz kontrolü
- Düğme, metin, durum göstergesi ve slider gibi temel HMI bileşenleri
- Hızlı başlangıç için sade ve modüler kod yapısı
- OTA (Over-the-Air) güncelleme desteği (isteğe bağlı)

---

## 🚀 Kurulum

### 1. Gerekli Yazılımlar
- **Arduino IDE** veya **PlatformIO**

- **ESP32 Board eklentisi** (File > Preferences > Additional boards manager URLs: https://espressif.github.io/arduino-esp32/package_esp32_index.json ve soldaki Board Manager bölümünden `esp32 by Espressif Systems` indir)

- Aşağıdaki kütüphaneler (Arduino IDE kullanıyorsan):
  - `LovyanGFX`
  - `lvgl`
  - `PCA9557`

### 2. Kodun Yüklenmesi
1. Proje dosyalarını indir:
   ```bash
   git clone https://github.com/Metrohan/CrowPanel7.0inchESPHMIDisplay.git
   ```

2. **Arduino IDE** veya **PlatformIO** ile aç.

3. Board kısmından **ESP32S3 Dev Module** seç.

4. Arduino IDE üzerinden Tools > Partition Scheme > Huge APP (3MB No OTA/1MB SPIFFS) ayarını seç.

5. Yine Arduino IDE üzerinden Tools > PSRAM > OPI PSRAM ayarını seç.

6. Kodu karta yükle, olmazsa IDE'yi kapatıp aç.

Not: Baud ayarını koddaki ile aynı yapmayı unutma ve dokunmatik için serial monitor'ü kontrol et. Eğer Dokunmatik algılandı dönütü almazsan **I2C Scanner** ile I2C adresini öğren ve gfx.conf.h dosyasındaki `cfg.i2c_addr   = 0x14;` satırını güncelle.