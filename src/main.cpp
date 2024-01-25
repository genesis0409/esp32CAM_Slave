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

// 메소드 선언부
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void InitESPNow();
void configDeviceAP();

// WifiManager
void initSPIFFS();                                                 // Initialize SPIFFS
String readFile(fs::FS &fs, const char *path);                     // Read File from SPIFFS
void writeFile(fs::FS &fs, const char *path, const char *message); // Write file to SPIFFS
bool initWiFi();                                                   // Initialize WiFi

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";
const char *PARAM_INPUT_4 = "gateway";

const char *PARAM_INPUT_5 = "Firebase API Key";
const char *PARAM_INPUT_6 = "Storage Bucket ID";
const char *PARAM_INPUT_7 = "user_email";
const char *PARAM_INPUT_8 = "user_password";

// Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

String api_key;
String bucket_id;
String user_email;
String user_password;

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *ipPath = "/ip.txt";
const char *gatewayPath = "/gateway.txt";

const char *api_keyPath = "/api_key.txt";
const char *bucket_idPath = "/bucket_id.txt";
const char *user_emailPath = "/user_email.txt";
const char *user_passwordPath = "/user_password.txt";

IPAddress localIP;
// IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
// IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 255, 0);

// Timer variables
unsigned long currentMillis = 0;
unsigned long previousMillis = 0; // Stores last time using Reference time
const long interval = 10000;      // interval to wait for Wi-Fi connection (milliseconds)

// for photo name
int pictureNumber = 1;
const uint32_t n_zero = 7; // zero padding

// Firebase Tutorial
#include "userCredential.h" // wifimanager 입력을 사용하지 않을 때, 로컬 하드코딩때 사용

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
#include "ESPAsyncWebServer.h"

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

// Photo File Path to save in Firebase storage bucket
// final path: /data/{camid}/photo.jpg
#define BUCKET_PHOTO_PATH "/data"

String camId;

void setup()
{
    Serial.begin(115200);
    Serial.println("ESPNow/Slave");

    initSPIFFS();

    // esp가 스테이션 모드에서 성공적으로 연결된 이후
    // 웹 서버 요청을 처리하는 명령
    if (initWiFi())
    {
    }
    // 스테이션 모드 연결 실패 시 AP모드 진입(wifi 재설정) softAP() 메소드
    else
    {
        // Connect to Wi-Fi network with SSID and password
        Serial.println("Setting AP (Access Point)");
        // NULL sets an open Access Point
        WiFi.softAP("ESP-WIFI-MANAGER", NULL);

        IPAddress IP = WiFi.softAPIP(); // Software enabled Access Point : 가상 라우터, 가상의 액세스 포인트
        Serial.print("AP IP address: ");
        Serial.println(IP);

        // Web Server Root URL
        // GET방식
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

        server.serveStatic("/", SPIFFS, "/");
        // POST방식
        server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
                  {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      // ESP가 양식 세부 정보를 수신했음을 알 수 있도록 일부 텍스트가 포함된 응답을 send
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart(); });
        server.begin();
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

// Initialize SPIFFS
void initSPIFFS()
{
    // start spiffs
    if (!SPIFFS.begin(true))
    {
        Serial.println(F("ERROR: File System Mount Failed!"));
    }
    else
    {
        Serial.println(F("Success Init SPIFFS"));
    }
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return String();
    }

    String fileContent;
    while (file.available())
    {
        fileContent = file.readStringUntil('\n');
        break;
    }
    return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
}

// Initialize WiFi
bool initWiFi()
{
    if (ssid == "" || ip == "")
    {
        Serial.println("Undefined SSID or IP address.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    localIP.fromString(ip.c_str());
    localGateway.fromString(gateway.c_str());

    if (!WiFi.config(localIP, localGateway, subnet))
    {
        Serial.println("STA Failed to configure");
        return false;
    }
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.println("Connecting to WiFi...");

    unsigned long currentMillis = millis();
    previousMillis = currentMillis;

    while (WiFi.status() != WL_CONNECTED)
    {
        currentMillis = millis();
        if (currentMillis - previousMillis >= interval)
        {
            Serial.println("Failed to connect.");
            return false;
        }
    }
    Serial.print("Connected IP : ");
    Serial.print(WiFi.localIP());
    Serial.println("(Station Mode)");
    return true;
}