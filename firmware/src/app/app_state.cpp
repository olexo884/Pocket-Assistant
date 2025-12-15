#include "app_state.h"

// ====== audio / bt ======
btAudio audio("PocketAssistantBT");
bool isBTOn = false;
float volumeMic = 0;

File audioFile;
bool recording = false;
bool isSending = false;
bool isEncoderPressed = false;

// ====== buttons ======
bool aiBtnPressed = false;
unsigned long lastAiBtnPressTime = 0;
unsigned long recordingStartTime = 0;

// ====== server upload ======
const char *serverHost = "192.168.0.21";
const int serverPort = 5000;
const char *serverPathVoiceRequest = "/voiceRequest";
const char *fileName = "/voice.wav";

WiFiClient client;

// ====== API ======
const char *geoLocationApiUrl = "http://ip-api.com/json";
const char *timeApiUrl = "https://timeapi.io/api/Time/current/zone?timeZone=";

// ====== pages ======
Page currentPage = MAIN_PAGE;

// ====== ui flags ======
bool wifiConnectDrawn = true;
bool mainPageDrawn = false;

// ====== time ======
int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;
int currentDay = 1;
int currentMonth = 1;
int currentYear = 0;

unsigned long lastUpdateDeviceTime = 0;
unsigned long lastSaveTime = 0;

// ====== wifi list ======
WiFiNetworkInfo *wifiList = nullptr;
int foundNetworks = 0;
int curPosElementWifiList = 0;
String currentNetworkWifiList = "";
bool encryptionEncryptionWifiList = true;

// ====== device AP ======
const char *ssidDevice = "Pocket assistant AI";

// ====== menu ======
int curPosMainList = 0;
const int totalMainPages = 1;

const char *settingsMenuItems[] = {
  "Change Network",
  "AI Settings",
  "Device Info",
  "Set Time",
  "Bluetooth"
};

int curPosMenuList = 0;
const int totalMenuPages = sizeof(settingsMenuItems) / sizeof(settingsMenuItems[0]);

// ====== ai settings ======
int curPosTextScroll = 0;
String aiTestResponse = "";

String ssid = "";
String pass = "";

const char *modelsAI[] = { "gpt-3.5-turbo", "gpt-4", "gpt-4-turbo" };
int modelAIIndex = 0;
char tokensBuffer[20] = {0};
int tokens = 100;
int curPosAISettings = 0;
bool isAISettingsEdit = false;

// !!! важливо: ТУТ ініціалізуй, інакше буде “uninitialized”
const char *settingsAIItems[2] = { "Tokens", "Model" };

// ====== set time ======
int curPosSetTime = 0;
bool isSetTimeEdit = false;

// ====== web server ======
WebServer server(80);

// ====== encoder ======
int encoderPrevS1 = 0;
int encoderCurS1 = 0;
int encoderCurS2 = 0;
bool encoderFlag = false;
int curKey = 0;
unsigned long whenKeyPress = 0;
