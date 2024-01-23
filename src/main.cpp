/*
// CAPTURE AND SEND IMAGE OVER ESP NOW
// Code by: Tal Ofer
// https://github.com/talofer99/ESP32CAM-Capture-and-send-image-over-esp-now
//
// This is the screen portion of the code.
//
// for more information
// https://www.youtube.com/watch?v=0s4Bm9Ar42U
*/

#include <esp_now.h>
#include <WiFi.h>
#define CHANNEL 1
#include "SPIFFS.h" // already included

int currentTransmitCurrentPosition = 0;
int currentTransmitTotalPackages = 0;
// byte showImage = 0;

// 메소드 선언부
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void InitESPNow();
void configDeviceAP();

// Firebase Tutorial
#include "userCredential.h"

#include "Arduino.h"
#include "soc/soc.h"          // Disable brownout problems
#include "soc/rtc_cntl_reg.h" // Disable brownout problems
#include "driver/rtc_io.h"
// #include <LittleFS.h>
// #include <SPIFFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Replace with your network credentials
// const char *ssid = "REPLACE_WITH_YOUR_SSID";
// const char *password = "REPLACE_WITH_YOUR_PASSWORD";

// Insert Firebase project API Key
// #define API_KEY "REPLACE_WITH_YOUR_FIREBASE_PROJECT_API_KEY."

// Insert Authorized Email and Corresponding Password - OR add "userCredential.h"
// #define USER_EMAIL "REPLACE_WITH_THE_AUTHORIZED_USER_EMAIL"
// #define USER_PASSWORD "REPLACE_WITH_THE_AUTHORIZED_USER_PASSWORD"

// Insert Firebase storage bucket ID e.g bucket-name.appspot.com
// For example:
// #define STORAGE_BUCKET_ID "esp-iot-app.appspot.com"

// Photo File Name to save in LittleFS/SPIFFS
#define FILE_PHOTO_PATH "/photo.jpg"
#define BUCKET_PHOTO_PATH "/data"

void setup()
{
    Serial.begin(115200);
    Serial.println("ESPNow/Slave");

    // start spiffs
    if (!SPIFFS.begin(true))
    {
        Serial.println(F("ERROR: File System Mount Failed!"));
    }
    else
    {
        Serial.println(F("Success Init SPIFFS"));
    }

    // Set device in AP mode to begin with
    WiFi.mode(WIFI_AP);
    // configure device AP mode
    configDeviceAP();
    // This is the mac address of the Slave in AP Mode
    Serial.print("AP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    // Init ESPNow with a fallback logic
    InitESPNow();
    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info.
    esp_now_register_recv_cb(OnDataRecv); // 수신 시 콜백함수 등록 (콜백: 파라미터로 함수 자체를 전달)
}

void loop()
{
    // if show image flag
    // 삭제 예정
    // if (showImage)
    // {
    //     showImage = 0;
    //     gfx->fillScreen(BLACK);
    //     unsigned long start = millis();
    //     jpegClass.draw(&SPIFFS,
    //                    (char *)"FILE_PHOTO_PATH", jpegDrawCallback, true /* useBigEndian */,
    //                    0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);

    //     Serial.printf("Time used: %lu\n", millis() - start);
    // }

    // firebase에 보내는 기능 추가하면 되나?
}

// callback when data is recv from Master
// 콜백함수: 마스터로부터 데이터 수신 시: esp_now_send(peer_addr, dataArray, dataArrayLength);
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
    // 1:
    // 2:
    switch (*data++) // *data++ == *(data++); 어떻게 해석해야될지 모르겠단말이지: *data(첫 번째 원소)로 switch넣고 나서 ++한다 -> 두 번째 원소 가리키도록 함
    {
    case 0x00: // camID 등 송신자 정보 출력 용도, 케이스 메시지를 나눠야겠는데 CAMID 수만큼 ㄷㄷ
        Serial.println();
    case 0x01: // startTransmit() 전송 시작을 알리는 메시지: 전송 데이터량 고지
        Serial.println("Start of new file transmit");
        currentTransmitCurrentPosition = 0;                    // 전송위치 초기화
        currentTransmitTotalPackages = (*data++) << 8 | *data; // 데이터 재구축: 2번째 원소(2nd 8bits)를 Left Shift 하여 비트 OR로 3번재 원소(1st bits)와 합침 -> 패키지 수 표현하는 하위 16비트 재구축)
        Serial.println("currentTransmitTotalPackages = " + String(currentTransmitTotalPackages));
        SPIFFS.remove(FILE_PHOTO_PATH); // 이전 파일 삭제; 이름은 자유
        break;
    case 0x02: // sendNextPackage() 다음 패키지 전송을 위한 기능
        // Serial.println("chunk of file transmit");
        currentTransmitCurrentPosition = (*data++) << 8 | *data++; // 데이터 재구축: 같은 원리로 [현재 전송 위치] 복원
        // Serial.println("chunk NUMBER = " + String(currentTransmitCurrentPosition));
        File file = SPIFFS.open(FILE_PHOTO_PATH, FILE_APPEND); // 파일을 덮어쓰지 않고 이어붙이도록 open
        if (!file)
            Serial.println("Error opening file ...");

        for (int i = 0; i < (data_len - 3); i++)
        {
            // byte dat = *data++;
            // Serial.println(dat);
            file.write(*data++); // 순차적으로(for) 다음 배열 원소 내용을 계속 작성
        }
        file.close();

        if (currentTransmitCurrentPosition == currentTransmitTotalPackages) // 현재 위치가 마지막 위치라면(index 상 전송 완료 이후 상태)
        {
            // showImage = 1; // 삭제 예정 - 이미지 표시기능
            Serial.println("Done File Transfer");
            File file = SPIFFS.open(FILE_PHOTO_PATH);
            Serial.println(file.size());
            file.close();

            // 파일 닫기 전후 쯤 firebase 저장기능 추가해야지 싶은데?
        }

        break;
    } // end case
} // end OnDataRecv

// Init ESP Now with fallback
void InitESPNow()
{
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK)
    {
        Serial.println("ESPNow Init Success");
    }
    else
    {
        Serial.println("ESPNow Init Failed");
        // Retry InitESPNow, add a count and then restart?
        // InitESPNow();
        // or Simply Restart
        ESP.restart();
    }
}

// config AP SSID
void configDeviceAP()
{
    const char *SSID = "Slave_1";
    bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
    if (!result)
    {
        Serial.println("AP Config failed.");
    }
    else
    {
        Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    }
}