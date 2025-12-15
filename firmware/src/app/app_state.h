#pragma once
#include <Arduino.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <btAudio.h>

// ====== API urls ======
extern const char *geoLocationApiUrl;
extern const char *timeApiUrl;

// ====== server upload ======
extern const char *serverHost;
extern const int serverPort;
extern const char *serverPathVoiceRequest;
extern const char *fileName;

extern WiFiClient client;

// ====== audio / bt ======
extern btAudio audio;
extern bool isBTOn;
extern float volumeMic;

extern File audioFile;
extern bool recording;
extern bool isSending;

// ====== buttons ======
extern bool aiBtnPressed;
extern unsigned long lastAiBtnPressTime;
extern unsigned long recordingStartTime;

extern bool isEncoderPressed;

// ====== WIFI list types ======
struct WiFiNetworkInfo {
  String ssid;
  int32_t rssi;
  bool encryption;
};

extern WiFiNetworkInfo *wifiList;
extern int foundNetworks;
extern int curPosElementWifiList;
extern String currentNetworkWifiList;
extern bool encryptionEncryptionWifiList;

// ====== pages ======
enum Page {
  MAIN_PAGE,
  AI_REQUEST,
  MENU,
  WIFI_LIST,
  WIFI_CONNECT,
  AI_SETTINGS,
  BLUETOOTH,
  SET_TIME,
  DEVICE_INFO
};

extern Page currentPage;

// ====== ui flags ======
extern bool wifiConnectDrawn;
extern bool mainPageDrawn;

// ====== time ======
extern int currentHour, currentMinute, currentSecond;
extern int currentDay, currentMonth, currentYear;

extern unsigned long lastUpdateDeviceTime;
extern unsigned long lastSaveTime;

// ====== device AP ======
extern const char *ssidDevice;

// ====== menu ======
extern int curPosMainList;
extern const int totalMainPages;

extern const char *settingsMenuItems[];
extern int curPosMenuList;
extern const int totalMenuPages;

// ====== ai settings ======
extern int curPosTextScroll;
extern String aiTestResponse;

extern String ssid;
extern String pass;

extern const char *modelsAI[];
extern int modelAIIndex;
extern char tokensBuffer[20];
extern int tokens;
extern int curPosAISettings;
extern bool isAISettingsEdit;
extern const char *settingsAIItems[2];

extern int curPosSetTime;
extern bool isSetTimeEdit;

// ====== web server ======
extern WebServer server;

// ====== encoder ======
extern int encoderPrevS1;
extern int encoderCurS1, encoderCurS2;
extern bool encoderFlag;
extern int curKey;
extern unsigned long whenKeyPress;
