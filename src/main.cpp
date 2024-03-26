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
// #include "SPIFFS.h" // already included
#include "LittleFS.h"

#include <time.h>
#include <NTPClient.h>

void printLocalTime();
int Year;
int Month;
int Day;
int Hour;
int Min;
const char *ntpServer = "pool.ntp.org";
uint8_t timeZone = 9;
uint8_t summerTime = 0;
String strtim;

int currentTransmitCurrentPosition = 0;
int currentTransmitTotalPackages = 0;

// 메소드 선언부
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void InitESPNow();
void configDeviceAP();

// WifiManager
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

#include <esp_wifi.h> // espnow & wifi 동시 구동 핵심
int32_t channel;

int32_t getWiFiChannel(const char *ssid);

void initLittleFS(); // Initialize LittleFS
// void initSPIFFS();                                                 // Initialize SPIFFS
String readFile(fs::FS &fs, const char *path);                     // Read File from SPIFFS
void writeFile(fs::FS &fs, const char *path, const char *message); // Write file to SPIFFS
bool initWiFi();                                                   // Initialize WiFi

bool isFirebaseConfigDefined();

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";
const char *PARAM_INPUT_4 = "gateway";

const char *PARAM_INPUT_5 = "api_key";
const char *PARAM_INPUT_6 = "bucket_id";
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

// Master cam id check
uint8_t camId;
const int NUMBERofMASTER = 2;

// for photo name
int pictureNumber[NUMBERofMASTER] = {
    0,
};
const uint32_t n_zero = 7; // zero padding

// Firebase Tutorial
// #include "userCredential.h" // wifimanager 입력을 사용하지 않을 때, 로컬 하드코딩때 사용

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

// Photo File Name to save in LittleFS/SPIFFS : CamId
// #define FILE_PHOTO_NAME "photo"

// Photo File Path to save in Firebase storage bucket
// final path: /data/{time}.jpg
#define BUCKET_PHOTO_PATH "/data/"

// Define Firebase Data objects; 데이터 객체 정의
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

void fcsUploadCallback(FCS_UploadStatusInfo info);

// Firebase에 성공적으로 연결되었는지 확인하는 bool 변수
bool taskCompleted = false;

void setup()
{
    Serial.begin(115200);
    Serial.println("ESPNow/Slave");

    // initSPIFFS();
    initLittleFS();

    // Load values saved in LittleFS
    ssid = readFile(LittleFS, ssidPath);
    pass = readFile(LittleFS, passPath);
    // ip = readFile(LittleFS, ipPath);
    // gateway = readFile(LittleFS, gatewayPath);

    api_key = readFile(LittleFS, api_keyPath);
    bucket_id = readFile(LittleFS, bucket_idPath);
    user_email = readFile(LittleFS, user_emailPath);
    user_password = readFile(LittleFS, user_passwordPath);

    Serial.println(api_key);
    Serial.println(bucket_id);
    Serial.println(user_email);
    Serial.println(user_password);

    // esp가 스테이션 모드에서 성공적으로 연결된 이후(온라인)
    if (isFirebaseConfigDefined())
    {
        // initWiFi() &&
        //  Set device in AP mode to begin with
        //  WiFi.mode(WIFI_AP); // ESPNOW+WIFI 동시통신을 위해 WiFi.mode(WIFI_AP) 대신 WiFi.mode(WIFI_AP_STA) 사용
        // WiFi.mode(WIFI_AP_STA);

        initWiFi();

        // configure device AP mode
        configDeviceAP();

        // This is the mac address of the Slave in AP Mode
        Serial.print("AP MAC: ");
        Serial.println(WiFi.softAPmacAddress());

        // Init ESPNow with a fallback logic
        InitESPNow();

        channel = getWiFiChannel(ssid.c_str());
        Serial.print("WIFI Channel: ");
        Serial.println(channel);

        // Once ESPNow is successfully Init, we will register for recv CB to
        // get recv packer info.
        esp_now_register_recv_cb(OnDataRecv); // 수신 시 콜백함수 등록 (콜백: 파라미터로 함수 자체를 전달)

        // Firebase Setup; Firebase 구성 객체에 다음 설정 할당
        // Assign the api key
        configF.api_key = api_key;
        // Assign the user sign in credentials
        auth.user.email = user_email;
        auth.user.password = user_password;
        // Assign the callback function for the long running token generation task; 콜백함수
        configF.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

        Firebase.begin(&configF, &auth);
        Firebase.reconnectNetwork(true); // Firebase.reconnectWiFi(true); is deprecated.
    }
    // 스테이션 모드(온라인) 연결 실패 시 AP모드 진입(wifi 재설정) softAP() 메소드
    else
    {
        Serial.println("Undefiend Settings...");
        // Connect to Wi-Fi network with SSID and password
        Serial.println("Setting AP (Access Point)");
        // NULL sets an open Access Point
        WiFi.softAP("ESP-WIFI-MANAGER_Slave", NULL);

        IPAddress APIP = WiFi.softAPIP(); // Software enabled Access Point : 가상 라우터, 가상의 액세스 포인트
        Serial.print("AP IP address: ");
        Serial.println(APIP);

        // Web Server Root URL
        // GET방식
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(LittleFS, "/wifimanager.html", "text/html"); });

        server.serveStatic("/", LittleFS, "/");
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
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
        //   // HTTP POST ip value
        //   if (p->name() == PARAM_INPUT_3) {
        //     ip = p->value().c_str();
        //     Serial.print("IP Address set to: ");
        //     Serial.println(ip);
        //     // Write file to save value
        //     writeFile(LittleFS, ipPath, ip.c_str());
        //   }
        //   // HTTP POST gateway value
        //   if (p->name() == PARAM_INPUT_4) {
        //     gateway = p->value().c_str();
        //     Serial.print("Gateway set to: ");
        //     Serial.println(gateway);
        //     // Write file to save value
        //     writeFile(LittleFS, gatewayPath, gateway.c_str());
        //   }
          // HTTP POST API Key value
          if (p->name() == PARAM_INPUT_5)
          {
              api_key = p->value().c_str();
              Serial.print("API Key set to: ");
              Serial.println(api_key);
              // Write file to save value
              writeFile(LittleFS, api_keyPath, api_key.c_str());
          }
          // HTTP POST Bucket ID value
          if (p->name() == PARAM_INPUT_6)
          {
              bucket_id = p->value().c_str();
              Serial.print("Bucket ID set to: ");
              Serial.println(bucket_id);
              // Write file to save value
              writeFile(LittleFS, bucket_idPath, bucket_id.c_str());
          }
          // HTTP POST User Email value
          if (p->name() == PARAM_INPUT_7)
          {
              user_email = p->value().c_str();
              Serial.print("User Email set to: ");
              Serial.println(user_email);
              // Write file to save value
              writeFile(LittleFS, user_emailPath, user_email.c_str());
          }
          // HTTP User Password value
          if (p->name() == PARAM_INPUT_8)
          {
              user_password = p->value().c_str();
              Serial.print("API Key set to: ");
              Serial.println(user_password);
              // Write file to save value
              writeFile(LittleFS, user_passwordPath, user_password.c_str());
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
}

void loop()
{
    // firebase에 보내는 기능 추가하면 되나?
    // espnow 콜백함수에서 받자마자 처리하는게 나아보이는데
}

// callback when data is recv from Master
// 콜백함수: 마스터로부터 데이터 수신 시: esp_now_send(peer_addr, dataArray, dataArrayLength);
// espnow 메시지 데이터를 받으면 case 통과하면서 전부 처리됨
// camId와 같은 이름의 파일 쓰기/삭제: camId별 파일관리
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
    camId = *data++; // 0번째 원소: camId 쓰고 1번째 원소 가리켜 switch에 넣음
    std::string FILE_PHOTO_NAME = std::to_string(camId);

    /* // 보낼 때 지정하자
    std::string old_str = std::to_string(pictureNumber[camId]); // 0부터
    std::string new_str = std::string(n_zero - std::min(n_zero, old_str.length()), '0') + old_str;

    // Path where new picture will be saved in
    std::string path = "/CAM" + std::to_string(camId) + "_" + new_str + ".jpg"; // /CAM0_0000001.jpg
    */
    switch (*data++) // *data++ == *(data++); 어떻게 해석해야될지 모르겠단말이지: *data(1번째 원소)로 switch넣고 나서 ++한다 -> 2번째 원소 가리키도록 함
    {
    case 0x01: // startTransmit() 전송 시작을 알리는 메시지: 전송 데이터량 고지
        Serial.print("Start of new file transmit from CamId: ");
        Serial.println(camId);

        currentTransmitCurrentPosition = 0;                    // 전송위치 초기화
        currentTransmitTotalPackages = (*data++) << 8 | *data; // 데이터 재구축: 2번째 원소(2nd 8bits)를 Left Shift 하여 비트 OR로 3번째 원소(1st bits)와 합침 -> 패키지 수 표현하는 하위 16비트 재구축)
        Serial.println("currentTransmitTotalPackages = " + String(currentTransmitTotalPackages));
        // SPIFFS.remove(("/" + FILE_PHOTO_NAME).c_str()); // 이전 파일 삭제; String 객체는 문자열 인자가 아님 -> char* 변환필요: .c_str()
        LittleFS.remove(("/" + FILE_PHOTO_NAME).c_str()); // 이전 파일 삭제; String 객체는 문자열 인자가 아님 -> char* 변환필요: .c_str()
        break;
    case 0x02: // sendNextPackage() 다음 패키지 전송을 위한 기능
        // Serial.println("chunk of file transmit");
        currentTransmitCurrentPosition = (*data++) << 8 | *data++; // 데이터 재구축: 같은 원리로 [현재 전송 위치] 복원
        // Serial.println("chunk NUMBER = " + String(currentTransmitCurrentPosition));
        // File file = SPIFFS.open(("/" + FILE_PHOTO_NAME).c_str(), FILE_APPEND); // 파일을 덮어쓰지 않고 이어붙이도록 open
        File file = LittleFS.open(("/" + FILE_PHOTO_NAME).c_str(), FILE_APPEND); // 파일을 덮어쓰지 않고 이어붙이도록 open
        if (!file)
            Serial.println("Error opening file ...");

        for (int i = 0; i < (data_len - 4); i++)
        {
            // byte dat = *data++;
            // Serial.println(dat);
            file.write(*data++); // 순차적으로(for) 다음 배열 원소 내용을 계속 작성
        }
        file.close();

        if (currentTransmitCurrentPosition == currentTransmitTotalPackages) // 현재 위치가 마지막 위치라면(index 상 전송 완료 이후 상태)
        {
            // showImage = 1; // 삭제 예정 - 이미지 표시기능
            Serial.print("Done File Transfer from Cam ID: ");
            Serial.println(camId);

            // File file = SPIFFS.open(("/" + FILE_PHOTO_NAME).c_str());
            File file = LittleFS.open(("/" + FILE_PHOTO_NAME).c_str());
            Serial.println(file.size());

            file.close();

            // pictureNumber[camId]++; // 파일 이름으로 쓰기엔 아쉬운 저장공간

            //  시간 객체 사용 - 파일이름으로
            configTime(3600 * timeZone, 3600 * summerTime, ntpServer);
            printLocalTime();

            // while (WiFi.status() != WL_CONNECTED)
            // {
            //     WiFi.disconnect();
            //     WiFi.mode(WIFI_STA);
            //     WiFi.begin(ssid.c_str(), pass.c_str());
            //     Serial.println("Connecting to WiFi...");

            //     // unsigned long currentMillis = millis();
            //     // previousMillis = currentMillis;
            //     delay(1000);
            //     Serial.print(".");
            //     // currentMillis = millis();
            //     // if (currentMillis - previousMillis >= interval)
            //     // {
            //     //     Serial.println("Failed to connect.");
            //     //     return false;
            //     // }
            // }
            // Serial.print("Connected IP : ");
            // Serial.print(WiFi.localIP());
            // Serial.println("(Station Mode)");

            // Upload files to Firebase
            delay(1);

            if (Firebase.ready())
            {
                Serial.print("Uploading picture... ");

                // MIME type should be valid to avoid the download problem.
                // The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
                if (Firebase.Storage.upload(&fbdo, "esp32-send-img2firestorage.appspot.com", ("/" + FILE_PHOTO_NAME).c_str(), mem_storage_type_flash /* memory storage type, mem_storage_type_flash and mem_storage_type_sd */, BUCKET_PHOTO_PATH + strtim + ".jpg" /* path of remote file stored in the bucket */, "image/jpeg" /* mime type */, fcsUploadCallback))
                {
                    Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
                }
                else
                {
                    Serial.println(fbdo.errorReason());
                }
            }
        }

        break;
    } // end case
} // end OnDataRecv

// Init ESP Now with fallback
void InitESPNow()
{
    // WiFi.disconnect(); // 끊지 않고 AP_STA 모드 활용하는 방안

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
    // bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
    bool result = WiFi.softAP(SSID, "Slave_1_Password");
    if (!result)
    {
        Serial.println("AP Config failed.");
    }
    else
    {
        Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    }
}

// Initialize LittleFS
void initLittleFS()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting LittleFS");
        ESP.restart();
    }
    else
    {
        delay(500);
        Serial.println("LittleFS mounted successfully");
    }
}

// Initialize SPIFFS
// void initSPIFFS()
// {
//     // start spiffs
//     if (!SPIFFS.begin(true))
//     {
//         Serial.println(F("ERROR: File System Mount Failed!"));
//     }
//     else
//     {
//         Serial.println(F("Success Init SPIFFS"));
//     }
// }

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
    if (ssid == "")
    {
        Serial.println("Undefined SSID.");
        return false;
    }

    WiFi.mode(WIFI_AP_STA); // AP, STA 동시 사용?
    // localIP.fromString(ip.c_str());
    // localGateway.fromString(gateway.c_str());

    // if (!WiFi.config(localIP, localGateway, subnet))
    // {
    //     Serial.println("STA Failed to configure");
    //     return false;
    // }
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.println("Connecting to WiFi...");

    // unsigned long currentMillis = millis();
    // previousMillis = currentMillis;

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
        // currentMillis = millis();
        // if (currentMillis - previousMillis >= interval)
        // {
        //     Serial.println("Failed to connect.");
        //     return false;
        // }
    }
    Serial.print("Connected IP : ");
    Serial.print(WiFi.localIP());
    Serial.println("(Station Mode)");
    return true;
}

bool isFirebaseConfigDefined()
{
    if (api_key == "" || bucket_id == "" || user_email == "" || user_password == "")
    {
        Serial.println("Undefined API_KEY/Bucket_ID or UserInfo.");
        return false;
    }
    return true;
}

// get Wifi Channel
int32_t getWiFiChannel(const char *ssid)
{
    if (int32_t n = WiFi.scanNetworks())
    {
        for (uint8_t i = 0; i < n; i++)
        {
            if (!strcmp(ssid, WiFi.SSID(i).c_str()))
            {
                return WiFi.channel(i);
            }
        }
    }
    return 0;
}

// The Firebase Storage upload callback function
void fcsUploadCallback(FCS_UploadStatusInfo info)
{
    if (info.status == firebase_fcs_upload_status_init)
    {
        Serial.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
    }
    else if (info.status == firebase_fcs_upload_status_upload)
    {
        Serial.printf("Uploaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    }
    else if (info.status == firebase_fcs_upload_status_complete)
    {
        Serial.println("Upload completed\n");
        FileMetaInfo meta = fbdo.metaData();
        Serial.printf("Name: %s\n", meta.name.c_str());
        Serial.printf("Bucket: %s\n", meta.bucket.c_str());
        Serial.printf("contentType: %s\n", meta.contentType.c_str());
        Serial.printf("Size: %d\n", meta.size);
        Serial.printf("Generation: %lu\n", meta.generation);
        Serial.printf("Metageneration: %lu\n", meta.metageneration);
        Serial.printf("ETag: %s\n", meta.etag.c_str());
        Serial.printf("CRC32: %s\n", meta.crc32.c_str());
        Serial.printf("Tokens: %s\n", meta.downloadTokens.c_str());
        Serial.printf("Download URL: %s\n\n", fbdo.downloadURL().c_str());
    }
    else if (info.status == firebase_fcs_upload_status_error)
    {
        Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
    }
}

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
        // delay(500);
    }
    Serial.println("Successed to obtain time");
    Year = timeinfo.tm_year + 1900;
    Month = timeinfo.tm_mon + 1;
    Day = timeinfo.tm_mday;
    Hour = timeinfo.tm_hour;
    Min = timeinfo.tm_min;
    strtim = String(Year) + "-" + String(Month) + "-" + String(Day) + " " + String(Hour) + ":" + String(Min); // 2024-01-01 01:01
    Serial.println(strtim);

    // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}