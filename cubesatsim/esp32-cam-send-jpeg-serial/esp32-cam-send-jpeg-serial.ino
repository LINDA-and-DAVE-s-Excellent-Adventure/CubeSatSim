//  ESP32-CAM JPEG over Serial
//
//  by Jim McLaughlin and Alan Johnston
//
//  Tested on ESP32-CAM-MB Camera Module hardware
//  with Arduino 1.8.19 using Board: ESP32 Arduino Selected: AI Thinker ESP32-CAM
//
//  example loop() program stores a JPEG and sends it over serial then waits 5 seconds.
//
//  Based on code example https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/Camera/CameraWebServer
//
//  initialize_camera initializes the ESP32-CAM - only call once per boot!
//
//  save_camera_image takes a JPEG image and stores it with selected filename on
//       built-in SPIFFS storage on ESP32-CAM (no SD card is needed)
//
//  send_image_serial sends a selected filename from built-in storage over the serial
//       port at 115200 in binary format with a 30 byte start flag and 40 byte end flag
//       CRC-8 checksum is calculated over file and end flag and sent as the last byte.
//
//  listDir gives a directory file listing of the built-in file storage
//
#include <Arduino.h>
#include "esp_camera.h"
#include <FastCRC.h>
//#include "FS.h"
#include "SPIFFS.h"

// some info: https://visualgdb.com/tutorials/esp32/camera/

//#define DEBUG
#define FORMAT_SPIFFS_IF_FAILED true

//FONTS
const uint8_t b_fonts[43][11] = {
        {0x00, 0x18, 0x24, 0x62, 0x62, 0x62, 0x7E, 0x62, 0x62, 0x62, 0x00}, //00: A
        {0x00, 0x7C, 0x32, 0x32, 0x32, 0x3C, 0x32, 0x32, 0x32, 0x7C, 0x00}, //01: B
        {0x00, 0x3C, 0x62, 0x62, 0x60, 0x60, 0x60, 0x62, 0x62, 0x3C, 0x00}, //02: C
        {0x00, 0x7C, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x7C, 0x00}, //03: D
        {0x00, 0x7E, 0x60, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x7E, 0x00}, //04: E
        {0x00, 0x7E, 0x60, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x60, 0x00}, //05: F
        {0x00, 0x3C, 0x62, 0x62, 0x60, 0x60, 0x66, 0x62, 0x62, 0x3C, 0x00}, //06: G
        {0x00, 0x62, 0x62, 0x62, 0x62, 0x7E, 0x62, 0x62, 0x62, 0x62, 0x00}, //07: H
        {0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00}, //08: I
        {0x00, 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x4C, 0x4C, 0x4C, 0x38, 0x00}, //09: J
        {0x00, 0x62, 0x64, 0x68, 0x70, 0x68, 0x64, 0x62, 0x62, 0x62, 0x00}, //10: K
        {0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}, //11: L
        {0x00, 0x42, 0x62, 0x76, 0x6A, 0x62, 0x62, 0x62, 0x62, 0x62, 0x00}, //12: M
        {0x00, 0x42, 0x62, 0x72, 0x6A, 0x66, 0x62, 0x62, 0x62, 0x62, 0x00}, //13: N
        {0x00, 0x3C, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x3C, 0x00}, //14: O
        {0x00, 0x7C, 0x62, 0x62, 0x62, 0x7C, 0x60, 0x60, 0x60, 0x60, 0x00}, //15: P
        {0x00, 0x3C, 0x62, 0x62, 0x62, 0x62, 0x62, 0x6A, 0x6A, 0x3C, 0x08}, //16: Q
        {0x00, 0x7C, 0x62, 0x62, 0x62, 0x7C, 0x68, 0x64, 0x62, 0x62, 0x00}, //17: R
        {0x00, 0x3C, 0x62, 0x60, 0x60, 0x3C, 0x06, 0x06, 0x46, 0x3C, 0x00}, //18: S
        {0x00, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, //19: T
        {0x00, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x3C, 0x00}, //20: U
        {0x00, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x22, 0x14, 0x08, 0x00}, //21: V
        {0x00, 0x62, 0x62, 0x62, 0x62, 0x62, 0x6A, 0x76, 0x62, 0x42, 0x00}, //22: W
        {0x00, 0x42, 0x62, 0x74, 0x38, 0x1C, 0x2E, 0x46, 0x42, 0x42, 0x00}, //23: X
        {0x00, 0x42, 0x62, 0x74, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, //24: Y
        {0x00, 0x7E, 0x06, 0x0E, 0x0C, 0x18, 0x30, 0x70, 0x60, 0x7E, 0x00}, //25: Z
        {0x00, 0x3C, 0x62, 0x62, 0x66, 0x6A, 0x72, 0x62, 0x62, 0x3C, 0x00}, //26: 0
        {0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, //27: 1
        {0x00, 0x3C, 0x46, 0x06, 0x06, 0x1C, 0x20, 0x60, 0x60, 0x7E, 0x00}, //28: 2
        {0x00, 0x3C, 0x46, 0x06, 0x06, 0x1C, 0x06, 0x06, 0x46, 0x3C, 0x00}, //29: 3
        {0x00, 0x0C, 0x1C, 0x2C, 0x4C, 0x4C, 0x7E, 0x0C, 0x0C, 0x0C, 0x00}, //30: 4
        {0x00, 0x7E, 0x60, 0x60, 0x60, 0x7C, 0x06, 0x06, 0x46, 0x3C, 0x00}, //31: 5
        {0x00, 0x3C, 0x62, 0x60, 0x60, 0x7C, 0x62, 0x62, 0x62, 0x3C, 0x00}, //32: 6
        {0x00, 0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00}, //33: 7
        {0x00, 0x3C, 0x62, 0x62, 0x62, 0x3C, 0x62, 0x62, 0x62, 0x3C, 0x00}, //34: 8
        {0x00, 0x3C, 0x46, 0x46, 0x46, 0x3E, 0x06, 0x06, 0x46, 0x3C, 0x00}, //35: 9
        {0x00, 0x00, 0x02, 0x06, 0x0E, 0x1C, 0x38, 0x70, 0x60, 0x40, 0x00}, //36: /
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x7E, 0x00, 0x00, 0x00, 0x00}, //37: -
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x00}, //38: .
        {0x00, 0x3C, 0x46, 0x06, 0x06, 0x0C, 0x10, 0x00, 0x30, 0x30, 0x00}, //39: ?
        {0x00, 0x18, 0x18, 0x18, 0x18, 0x10, 0x10, 0x00, 0x18, 0x18, 0x00}, //40: !
        {0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00}, //41: :
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  //42: space
};

// Nibble font table
const uint8_t l_fonts[23][5] = {
  { 0xE2, 0xA6, 0xA2, 0xA2, 0xE2 }, // 0: 01
  { 0xEE, 0x22, 0xE6, 0x82, 0xEE }, // 1: 23
  { 0xAE, 0xA8, 0xEE, 0x22, 0x2E }, // 2: 45
  { 0x8E, 0x82, 0xE2, 0xA2, 0xE2 }, // 3: 67
  { 0xEE, 0xAA, 0xEE, 0xA2, 0xE2 }, // 4: 89
  { 0x00, 0x22, 0x00, 0x22, 0x04 }, // 5: :;
  { 0x20, 0x4E, 0x80, 0x4E, 0x20 }, // 6: <=
  { 0x8E, 0x42, 0x26, 0x40, 0x84 }, // 7: >?
  { 0x64, 0x9A, 0xBE, 0x8A, 0x7A }, // 8: @A
  { 0xC6, 0xA8, 0xC8, 0xA8, 0xC6 }, // 9: BC
  { 0xCE, 0xA8, 0xAC, 0xA8, 0xCE }, // 10: DE
  { 0xE6, 0x88, 0xCE, 0x8A, 0x86 }, // 11: FG
  { 0xA4, 0xA4, 0xE4, 0xA4, 0xA4 }, // 12: HI
  { 0x69, 0x2A, 0x2C, 0x2A, 0x49 }, // 13: JK
  { 0x8A, 0x8E, 0x8E, 0x8A, 0xEA }, // 14: LM
  { 0x04, 0x9A, 0xDA, 0xBA, 0x94 }, // 15: NO
  { 0xC4, 0xAA, 0xCA, 0x8E, 0x86 }, // 16: PQ
  { 0xC6, 0xA8, 0xC4, 0xA2, 0xAC }, // 17: RS
  { 0xE0, 0x4A, 0x4A, 0x4A, 0x44 }, // 18: TU
  { 0x09, 0xA9, 0xA9, 0x6F, 0x26 }, // 19: vW (sort of..)
  { 0x0A, 0xAA, 0x46, 0xA2, 0x04 }, // 20: XY
  { 0xE6, 0x24, 0x44, 0x84, 0xE6 }, // 21: Z[
  { 0x00, 0x00, 0x00, 0x00, 0x00 }  // 22: SPACE
};

char input_buffer[240][240][3];

void app_main();
void send_image_serial(char *filename);
void save_camera_image(char* filename);
void initialize_camera();

static esp_err_t init_camera();
#define RX2_pin         IO16 // AMBE3000 RX to ESP32 TXD
#define TX2_pin         IO17 // AMBE3000 TX to ESP32 RXD
#define LED_PIN 33  // LED pin on ESP32-CAM-MB board is 33

const char start_flag[] = "3d99de816e5ad7742b61a37c39141783";  // MD5 hash of "ESP32-CAM" as start flag
const char end_flag[] = "f681a5c52351befe0e3524eb1a40f14b7803317a"; // SHA-1 hash of "ESP32-CAM" as end flag
FastCRC8 CRC8;
char hexValue[6];
char file_buf[50000];
int file_len;
bool file_flag = false;
camera_fb_t *pic;
File inFile;
File outFile;
int blocks = 0;
//int counter = 0;
bool write_complete = false;

// example code from https://github.com/espressif/arduino-esp32/blob/master/libraries/LittleFS/examples/LITTLEFS_test/LITTLEFS_test.ino
//
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

void setup() {

//  delay(5000);  
        
//  #define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
//  #define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */
//  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);  // testing sleep
       
  Serial.begin(115200);
        

}

void loop() {

  bool timeout = false; 
  bool take_photo = false;
  
  pinMode(LED_PIN, OUTPUT); // Set the pin as output

    Serial.println("\nBlink three times");
    digitalWrite(LED_PIN, LOW); // Turn on
    delay (100); // Wait 0.1 sec
    digitalWrite(LED_PIN, HIGH); // Turn off
    delay(100);  // Wait 0.1 sec
    digitalWrite(LED_PIN, LOW); // Turn on
    delay (100); // Wait 0.1 sec
    digitalWrite(LED_PIN, HIGH); // Turn off       
     delay(100);  // Wait 0.1 sec
    digitalWrite(LED_PIN, LOW); // Turn on
    delay (100); // Wait 0.1 sec
    digitalWrite(LED_PIN, HIGH); // Turn off  
        
  Serial.println("Checking for serial input before sleeping");      
  unsigned long timer_ms = millis();      
  while ((Serial.available() <= 0) && !timeout) { 
//    if ((millis() - timer_ms) > 10000) timeout = true;  // poll serial for 10 seconds
  }
  if (Serial.available() > 0)  {
    char result = Serial.read();
    if ((result == 'f') || (result == 'F')) SPIFFS.format(); 
    take_photo = true;
    Serial.println("Serial input received!");      
  }
  if (!take_photo) {
    Serial.println("No serial input received!");     
/*          
    esp_sleep_enable_timer_wakeup(5 * 1000000);  // sleep for 10 seconds
    Serial.println("Going to sleep now for 5 seconds");
    Serial.flush(); 
    esp_deep_sleep_start();           
*/    
  }
               

  initialize_camera();
        
  config_camera();      

  Serial.println("ESP32-CAM Camera initialized v0.3");

   // On the ESP32 you can also define another hardware serial interface using pins of your choice like this
  // Serial2.begin(115200,SERIAL_8N1,14,15);

  //  Serial.println("Initializing CRC");

  //  crc.setPolynome(0x1021);
        
  // SPIFFS.format();       // force a format of flash storage, if file names show up twice in directory, uncomment this line once
        
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  listDir(SPIFFS, "/", 0);
        
  char filename[] = "/cam.jpg";

  save_camera_image(filename);

  send_image_serial(filename);

//  delay(500);

/*        
  esp_sleep_enable_timer_wakeup(2 * 1000000);  // sleep for 10 seconds
 
  Serial.println("Going to sleep now for 2 seconds");
  Serial.flush(); 
  esp_deep_sleep_start();   
        
*/        
          
}

/**
   This example takes a picture every 5s and print its size on serial monitor.
*/

// =============================== SETUP ======================================

// 1. Board setup (Uncomment):
// #define BOARD_WROVER_KIT
// #define BOARD_ESP32CAM_AITHINKER

/**
   2. Kconfig setup

   If you have a Kconfig file, copy the content from
    https://github.com/espressif/esp32-camera/blob/master/Kconfig into it.
   In case you haven't, copy and paste this Kconfig file inside the src directory.
   This Kconfig file has definitions that allows more control over the camera and
   how it will be initialized.
*/

/**
   3. Enable PSRAM on sdkconfig:

   CONFIG_ESP32_SPIRAM_SUPPORT=y

   More info on
   https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html#config-esp32-spiram-support
*/

// ================================ CODE ======================================

//#include <esp_log.h>
//#include <esp_system.h>
//#include <nvs_flash.h>
//#include <sys/param.h>
//#include <string.h>

//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"

// ESP32Cam (AiThinker) PIN Map

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

//static const char *TAG = "example:take_picture";

static camera_config_t camera_config = {
  .pin_pwdn = CAM_PIN_PWDN,
  .pin_reset = CAM_PIN_RESET,
  .pin_xclk = CAM_PIN_XCLK,
  .pin_sscb_sda = CAM_PIN_SIOD,
  .pin_sscb_scl = CAM_PIN_SIOC,

  .pin_d7 = CAM_PIN_D7,
  .pin_d6 = CAM_PIN_D6,
  .pin_d5 = CAM_PIN_D5,
  .pin_d4 = CAM_PIN_D4,
  .pin_d3 = CAM_PIN_D3,
  .pin_d2 = CAM_PIN_D2,
  .pin_d1 = CAM_PIN_D1,
  .pin_d0 = CAM_PIN_D0,
  .pin_vsync = CAM_PIN_VSYNC,
  .pin_href = CAM_PIN_HREF,
  .pin_pclk = CAM_PIN_PCLK,

  //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,

  .pixel_format = PIXFORMAT_JPEG, // Options: YUV422, GRAYSCALE, RGB565, JPEG
  .frame_size = FRAMESIZE_QVGA, // Options: QQVGA-UXGA, QVGA  Do not use sizes above QVGA when not JPEG

  .jpeg_quality = 6, // 12, //0-63 lower number means higher quality
  .fb_count = 1,       //if more than one, i2s runs in continuous mode. Use only with JPEG
  //    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

static esp_err_t init_camera()
{
  //initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK)
  {
    //        ESP_LOGE(TAG, "Camera Init Failed");
    return err;
  }
   
        
  return ESP_OK;
}

void config_camera() {
 
  sensor_t * s = esp_camera_sensor_get();
        
//  s->set_brightness(s, 2);     // -2 to 2
//  s->set_contrast(s, 0);       // -2 to 2
//  s->set_saturation(s, 1);     // -2 to 2  
  s->set_hmirror(s, 1);        // 0 = disable , 1 = enable
//  s->set_vflip(s, 1);         
}

void initialize_camera() {
  if (ESP_OK != init_camera()) {
    Serial.println("Failed to initialize camera!");
    return;
  }
 }
 
void save_camera_image(char* filename)
{
  Serial.print("Saving camera image to file: ");
  Serial.println(filename);

  Serial.println("\nBlink...");
  digitalWrite(LED_PIN, LOW); // Turn on
  delay (100); // Wait 0.1 sec
  digitalWrite(LED_PIN, HIGH); // Turn off

  Serial.println("\nTaking picture...\n");

  pic = esp_camera_fb_get();

  const char path[] = "/cam.jpg";

  Serial.println("Writing image file"); //: %s\r\n", path);

  //   File file = SPIFFS.open(path, FILE_WRITE);
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }

  for (int k = 0; k < pic->len; k++) {
    file.write(pic->buf[k]);
  }
  file.close();

  Serial.println("Write complete");

  listDir(SPIFFS, "/", 0);
}

void send_image_serial(char *filename)
{

  Serial.println("Sending image over serial");
 
    Serial.println("\nBlink twice");
    digitalWrite(LED_PIN, LOW); // Turn on
    delay (100); // Wait 0.1 sec
    digitalWrite(LED_PIN, HIGH); // Turn off
    delay(100);  // Wait 0.1 sec
    digitalWrite(LED_PIN, LOW); // Turn on
    delay (100); // Wait 0.1 sec
    digitalWrite(LED_PIN, HIGH); // Turn off
  
// read from file

      const char path[] = "/cam.jpg";

      Serial.println("Reading image file"); //: %s\r\n", path);

//      File file = SPIFFS.open(path, FILE_READ);
      File file = SPIFFS.open(filename, FILE_READ);
      if (!file) {
        Serial.println("Failed to open file for reading");
        return;
      }
      int k = 0;
      while (file.available()) {
        //       file.write(pic->buf[size]);
        file_buf[k] = file.read();
        k++;
      }
      file.close();
      file_len = k;
      Serial.print(file_len);
      Serial.println(" bytes read from file.");

      file_flag = true;

//    }

 // Sending image file
    Serial.println("Start of binary data:");

    //      Serial.println("\nResetting CRC");
    //      crc.restart();

    int j = 0;
    // flag at start
    for (int i = 0; i < strlen(start_flag); i++) {
      sprintf(hexValue, "%02X", start_flag[i]);
      //        Serial.print(hexValue);
      Serial.write(start_flag[i]);
      //        Serial.print(start_flag[i], HEX);
    }

#ifdef DEBUG
#endif
    char str[] = "ESP32-CAM";
    if (file_flag == true) {
      //       Serial.println("******************* Sending stored file! ***************************\n");
      for (j = 0; j < file_len; j++) {
        ///      for (int i = 0; i < 9; i++) {
        uint8_t *data = (uint8_t *) &file_buf[j];
        //       crc.add(data[0]);
        //        crc.add(data[0]);
        ///        crc.add(str[i]);
#ifndef DEBUG
        if (file_buf[j] != 0x66)
          sprintf(hexValue, "%02X", file_buf[j]);
        else
          strcpy(hexValue, "66");
        Serial.write(file_buf[j]);
        //        Serial.print(hexValue);
        ///        Serial.write(str[j]);
#endif
        //        Serial.print(pic->buf[j], HEX);
#ifdef DEBUG
        //     char hexValue[5];
        sprintf(hexValue, "%02X", file_buf[j]);
        Serial.print(hexValue);
#endif
        //        Serial.print(" ");
      }
    } else {  // !file_flag - read from camera
      for (j = 0; j < pic->len; j++) {
        ///      for (int i = 0; i < 9; i++) {
        uint8_t *data = (uint8_t *) &pic->buf[j];
        //       crc.add(data[0]);
        //        crc.add(data[0]);
        ///        crc.add(str[i]);
        file_buf[j] = pic->buf[j];
#ifndef DEBUG
        sprintf(hexValue, "%02X", pic->buf[j]);
        Serial.write(pic->buf[j]);
        //        Serial.print(hexValue);
        ///        Serial.write(str[j]);
#endif
        //        Serial.print(pic->buf[j], HEX);
#ifdef DEBUG
        //     char hexValue[5];
        sprintf(hexValue, "%02X", pic->buf[j]);
        Serial.print(hexValue);
#endif
        //        Serial.print(" ");
      }
    }
    //    Serial.print("j = ");
    //    Serial.println(j);
    // flag at end
    for (int i = 0; i < strlen(end_flag); i++) {
      sprintf(hexValue, "%02X", end_flag[i]);
      //          Serial.print(hexValue);
      file_buf[j++] = end_flag[i];
      Serial.write(end_flag[i]);
      //         Serial.print(end_flag[i], HEX);
      //          if (file_flag) {
      //            file_buf[j++] = end_flag[i];
      //          } else {
      //            pic->buf[i++] = end_flag[j];
      //          }
    }

    uint8_t * data;
    int index1;

    data = (uint8_t *) &file_buf[0];
    index1 = j;

    //    int calculated_crc = crc8(data, index1, 0x07, 0x0000, 0x0000, false, false);

    int calculated_crc = CRC8.smbus(data, index1);
    //#ifndef DEBUG
    sprintf(hexValue, "%02X", calculated_crc);
    Serial.write(calculated_crc);
    //    Serial.print(hexValue);
    //   Serial.write(0xff & calculated_crc);
    //#endif
    //#ifdef DEBUG

    //     sprintf(hexValue, "%04X", calculated_crc);
    //     Serial.print(hexValue);
    //     Serial.println("\n ");
    //#endif

    Serial.println("\nEnd of binary data.");

    Serial.println("\nCRC cacluation data:");
    Serial.println(file_buf[0], HEX);
    Serial.println(file_buf[j - 1], HEX);
    Serial.println(j);

    Serial.print("\nCRC sent with image: ");
    //     char hexValue[6];
    sprintf(hexValue, "%02X", calculated_crc);
    Serial.print(hexValue);

    Serial.println("\n");

    /*  remove hex print
          for (int i = 0; i < pic->len; i++) {
      //        Serial.write(pic->buf[i]);
            Serial.print(pic->buf[i], HEX);
      //        Serial.print(" ");
          }
          Serial.println(" ");
    */
    // use pic->buf to access the image
    ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
    // format of the captured picture will be stored in the format field of the
    // structure returned by esp_camera_fb_get()

    Serial.print("image size ");
    if (file_flag)
      Serial.println(file_len);
    else {
      Serial.println(pic->len);
      // must free the memory allocated by esp_camera_fb_get()
      esp_camera_fb_return(pic);
    }
  Serial.println("File sent!");
}

void print_hex(byte octet) {
      char hexValue[5];
      sprintf(hexValue, "%02X", octet);
      Serial.print(hexValue); 
}
