#include "config.h"

#include "icon/icons.h"
#include "web/page.h"
#include "app/app_state.h"
#include "eeprom/eeprom.h"

#define FILTER_COEF 0.95f  
#define GAIN 800.0f       

#define EEPROM_SIZE 128

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_MOSI 23
#define OLED_CLK 18
#define OLED_DC 17
#define OLED_CS 5
#define OLED_RESET 16

#define Encoder_S1 34
#define Encoder_S2 35
#define Encoder_key 32

#define LEFT_BUTTON_PIN 13
#define RIGHT_BUTTON_PIN 15

#define ENCODER_BUTTON_PIN 32

#define VOLUME_FACTOR 0.1f
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

#define I2S_WS 26
#define I2S_SD 21
#define I2S_SCK 27
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE (16000)
#define I2S_SAMPLE_BITS (16)
#define I2S_READ_LEN (1024)
#define I2S_CHANNEL_NUM (1)

#define SD_CS 4
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

void drawConnectToWiFi(const String &password, bool encryption);
void handleSetWifi();
void startAPServer();

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         &SPI, OLED_DC, OLED_RESET, OLED_CS);

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = I2S_READ_LEN,
    .use_apll = true
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
  Serial.println("I2S ініціалізовано.");
}
void i2sDeInit() {
  i2s_driver_uninstall(I2S_NUM_0);
}


void displayConnectionStatus(const String &topText, const String &bottomText) {
  display.fillRect(0, 18, 128, 33, SSD1306_BLACK);

  display.drawBitmap(0, 20, epd_bitmap_wifi, 34, 25, SSD1306_WHITE);

  display.setFont(&FreeSans12pt7b);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(39, 35);
  display.println(topText);

  display.setFont();
  display.setTextSize(1);
  display.setCursor(39, 38);
  display.println(bottomText);

  display.display();
}
void displayConnectionStatusError(const String &text) {
  display.fillRect(0, 18, 128, 33, SSD1306_BLACK);

  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println(text);

  display.display();
}

bool tryConnectWiFi(const char *ssidWifi, const char *password = nullptr, bool overwriting = false, int maxAttempts = 10, int attemptDelay = 500) {
  if (strlen(ssidWifi) > 0) {
    WiFi.begin(ssidWifi, password);
  } else {
    WiFi.begin(ssidWifi);
  }

  unsigned long startTime = millis();
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    if (millis() - startTime > attemptDelay) {
      Serial.print(".");
      startTime = millis();
      attempts++;
    }
  }
  if (WiFi.status() == WL_CONNECTED && overwriting) {
    ssid = ssidWifi;
    pass = password;
    saveToEEPROM();
  }

  return WiFi.status() == WL_CONNECTED;
}

void drawConnectToWiFi(const String &password, bool encryption) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  displayConnectionStatus("Wait...", "Connecting...");

  bool connected;
  connected = tryConnectWiFi(currentNetworkWifiList.c_str(), password.c_str(), true);

  if (connected) {
    displayConnectionStatus("Connect", "successful");
    mainPageDrawn = false;
  } else {
    displayConnectionStatusError("Could not connect to Wi-Fi");
    startAPServer();
  }
}

void updateEncoderValue(int &value, int step = 1, int minVal = 0, int maxVal = 100) {
  encoderCurS1 = digitalRead(Encoder_S1);

  if (encoderCurS1 != encoderPrevS1) {
    encoderCurS2 = digitalRead(Encoder_S2);

    if (encoderFlag) {
      if (encoderCurS2 == encoderCurS1) {
        value -= step;
      } else {
        value += step;
      }

      value = constrain(value, minVal, maxVal);
      Serial.print("Значення: ");
      Serial.println(value);

      encoderFlag = false;
    } else {
      encoderFlag = true;
    }
  }

  encoderPrevS1 = encoderCurS1;
}

void handleSetWifi() {
  displayConnectionStatus("Wait...", "Settings set");

  if (server.method() != HTTP_POST) {
    displayConnectionStatusError("Method not supported. Expected POST");
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String body = server.arg("plain");

  if (body.isEmpty()) {
    displayConnectionStatusError("The request body is empty.");
    server.send(400, "application/json", "{\"error\":\"Порожній запит\"}");
    return;
  }

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    displayConnectionStatusError(error.f_str());
    server.send(400, "application/json", "{\"error\":\"Невірний формат JSON\"}");
    return;
  }

  String password = doc["password"] | "";
  password.trim();

  if (password.isEmpty()) {
    displayConnectionStatusError("The password field is missing or empty.");
    server.send(400, "application/json", "{\"error\":\"Поле password відсутнє або порожнє\"}");
    return;
  }

  drawConnectToWiFi(password, encryptionEncryptionWifiList);
  server.send(200, "application/json", "{\"status\":\"OK\"}");
}
void startAPServer() {
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  if (WiFi.softAP(ssidDevice)) {
    delay(50);

    String page = String(connectPage);
    page.replace("%NETWORK_NAME%", currentNetworkWifiList);

    server.on("/", [page]() {
      server.send(200, "text/html", page);
    });

    server.on("/setwifi", handleSetWifi);

    server.begin();

    Serial.print("Сервер запущено! IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Не вдалося запустити точку доступу!");
  }
}

void drawScroll(int x, int y, int pos, int quantity) {
  display.fillRect(x, y, 3, 50, SSD1306_BLACK);
  for (int i = 0; i < 26; i++) {
    display.fillCircle(x + 1, y + i * 2, 0, SSD1306_WHITE);
  }
  display.fillRect(x, y + (pos * 45) / ((quantity > 1) ? quantity - 1 : 1), 3, 5, SSD1306_WHITE);
}
void drawTextMessage(const String &text, int x = 0, int y = 0) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.print(text);
  display.fillRect(0, 52, 128, 13, SSD1306_BLACK);
  display.drawBitmap(106, 53, epd_bitmap_home, 12, 12, SSD1306_WHITE);
  display.display();
}
void drawTextMessageWithScroll(String &text) {
  uint8_t lineWidth = text.length() > 126 ? 20 : 21;
  int16_t x = 0;
  int16_t y = 0;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y - curPosTextScroll * 8);
  uint16_t count = 0;
  for (uint16_t i = 0; i < text.length(); ++i) {
    char c = text.charAt(i);

    if (count == lineWidth) {
      if (c == ' ') {
        continue;
      }
      // Переходимо на новий рядок
      display.print('\n');
      // Рухаємо курсор вниз
      y += 8;
      display.setCursor(x, y - curPosTextScroll * 8);
      count = 0;
    }

    // Друкуємо символ
    display.print(c);
    count++;

    // Якщо зустріли ручний перенос, обнуляємо лічильник
    if (c == '\n') {
      count = 0;
      y += 8;
      display.setCursor(x, y - curPosTextScroll * 8);
    }
  }
  if (text.length() > 126) drawScroll(124, 1, curPosTextScroll, ceil((float)text.length() / lineWidth) - 6);
  display.fillRect(0, 52, 128, 13, SSD1306_BLACK);
  display.drawBitmap(106, 53, epd_bitmap_home, 12, 12, SSD1306_WHITE);
  display.display();
  updateEncoderValue(curPosTextScroll, 1, 0, ceil((float)text.length() / lineWidth) - 6);
}

void playAudioFile() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi не підключено");
    return;
  }

  HTTPClient http;
  String url = "http://" + String(serverHost) + ":" + String(serverPort) + "/download";
  http.begin(url);


  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("❌ HTTP помилка: %d\n", httpCode);
    http.end();
    return;
  }

  // Відкриваємо файл на SD для запису
  File file = SD.open("/sound.wav", FILE_WRITE);
  if (!file) {
    Serial.println("❌ Не вдалося відкрити файл на SD");
    http.end();
    return;
  }

  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[512];
  int total = 0;

  while (http.connected() && stream->available()) {
    size_t len = stream->readBytes(buffer, sizeof(buffer));
    file.write(buffer, len);
    total += len;
  }

  file.close();
  http.end();

  i2sDeInit();
  audio.I2S(I2S_BCLK, I2S_DOUT, I2S_LRC);
  i2s_set_clk(I2S_NUM_0, 32000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

  File audioFile = SD.open("/sound.wav");
  if (!audioFile) {
    Serial.println("Файл не відкрито");
    return;
  }

  audioFile.seek(44);  // Пропускаємо заголовок

  while (audioFile.available()) {
    size_t bytesRead = audioFile.read(buffer, 512);

    for (size_t i = 0; i < bytesRead; i += 2) {
      int16_t *sample = (int16_t *)&buffer[i];
      *sample = (int16_t)(*sample * VOLUME_FACTOR);
    }

    size_t bytesWritten = 0;
    i2s_write(I2S_NUM_0, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
  }

  audioFile.close();
  i2sDeInit();
  i2sInit();
}
void sendVoiceFile() {
  const char *boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
  const int bufferSize = 1024;
  byte buffer[bufferSize];

  if (client.connected()) {
    client.stop();  // Дуже важливо: закриваємо старе з'єднання
  }

  if (!SD.begin(SD_CS)) {
    Serial.println(F("Не вдалося ініціалізувати SD-карту!"));
    drawTextMessage("SD card initialization error!");
    return;
  }

  File file = SD.open("/voice.wav", FILE_READ);
  if (!file) {
    Serial.println(F("Не вдалося відкрити файл для читання"));
    drawTextMessage("Could not open file for reading");
    return;
  }

  if (!client.connect(serverHost, serverPort)) {
    Serial.println(F("[ERROR] Підключення не вдалося!"));
    drawTextMessage("Connection failed!");
    file.close();
    return;
  }
  Serial.println(F("Підключено до сервера"));
  drawTextMessage("Connected to server");

  // Формуємо поля multipart-даних
  String formModel =
    "--" + String(boundary) + "\r\n" + "Content-Disposition: form-data; name=\"model\"\r\n\r\n" + modelsAI[modelAIIndex] + "\r\n";

  String formTokens =
    "--" + String(boundary) + "\r\n" + "Content-Disposition: form-data; name=\"max_tokens\"\r\n\r\n" + String(tokens) + "\r\n";

  String formStart =
    "--" + String(boundary) + "\r\n" + "Content-Disposition: form-data; name=\"file\"; filename=\"voice.wav\"\r\n" + "Content-Type: audio/wav\r\n\r\n";

  String formEnd = "\r\n--" + String(boundary) + "--\r\n";

  int contentLength = formModel.length() + formTokens.length() + formStart.length() + file.size() + formEnd.length();

  // Формуємо HTTP-заголовки
  String header =
    "POST " + String(serverPathVoiceRequest) + " HTTP/1.1\r\n" + "Host: " + String(serverHost) + ":" + String(serverPort) + "\r\n" + "Cache-Control: no-cache\r\n" + "Content-Type: multipart/form-data; boundary=" + String(boundary) + "\r\n" + "Content-Length: " + String(contentLength) + "\r\n\r\n";

  // Надсилаємо заголовки та частини запиту
  client.print(header);
  client.print(formModel);
  client.print(formTokens);
  client.print(formStart);

  Serial.println(F("Заголовки надіслано"));

  // Відправляємо сам файл порціями
  while (file.available()) {
    int bytesRead = file.read(buffer, bufferSize);
    if (bytesRead < 0) {
      Serial.println("Помилка читання з SD-карти!");
      client.println("Помилка під час передачі даних.");
      file.close();
      return;
    }
    if (bytesRead > 0) {
      client.write(buffer, bytesRead);
    }
  }
  file.close();

  // Закриваючі символи після файлу
  client.print("\r\n");

  // Кінець форми
  client.print(formEnd);
  client.flush();

  Serial.println(F("[INFO] Відправка запиту завершена."));
  drawTextMessage("Sending request is complete.");

  // Зчитуємо відповідь
  String response;
  unsigned long timeout = millis();

  while (client.connected() && (millis() - timeout < 120000)) {
    while (client.available()) {
      char c = client.read();
      response += c;
      timeout = millis();  // Оновлюємо таймер
    }
  }

  client.stop();
  Serial.println(response);

  // Парсимо JSON з відповіді
  int jsonStart = response.indexOf('{');
  if (jsonStart == -1) {
    Serial.println(F("[ERROR] JSON не знайдено у відповіді!"));
    drawTextMessage("JSON not found in response!");
    return;
  }

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, response.substring(jsonStart));

  if (error) {
    Serial.print(F("deserializeJson() помилка: "));
    Serial.println(error.c_str());
    drawTextMessage(error.c_str());
    return;
  }

  if (doc.containsKey("error")) {
    String errorMsg = doc["error"];
    Serial.println(F("[ERROR] Відповідь з помилкою:"));
    Serial.println(errorMsg);
    aiTestResponse = errorMsg;
  } else {
    String gptResponse = doc["gpt_response"] | "null";
    String transcribedText = doc["transcribed_text"] | "null";

    Serial.println(F("[INFO] GPT-відповідь:"));
    Serial.println(gptResponse);
    Serial.println(transcribedText);

    aiTestResponse = gptResponse;
  }
}
void deleteVoiceFile() {
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Не вдалося ініціалізувати SD-карту!"));
    return;
  }

  if (SD.exists("/voice.wav")) {
    if (SD.remove("/voice.wav")) {
      Serial.println(F("[INFO] Файл voice.wav успішно видалено."));
    } else {
      Serial.println(F("[ERROR] Не вдалося видалити файл voice.wav!"));
    }
  } else {
    Serial.println(F("[INFO] Файл voice.wav не існує."));
  }
}
void listFiles() {
  // Відкриття кореневого каталогу
  File root = SD.open("/");

  // Перевірка, чи корінь був відкритий успішно
  if (!root) {
    Serial.println("Не вдалося відкрити корінь!");
    return;
  }

  Serial.println("Список файлів на SD-картці:");

  // Ітерація через всі елементи в кореневому каталозі
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      // Вихід з циклу, коли більше немає файлів
      break;
    }

    // Виведення імені файлу або директорії
    if (entry.isDirectory()) {
      Serial.print("[DIR] ");
    } else {
      Serial.print("[FILE] ");
    }
    Serial.println(entry.name());

    entry.close();  // Закриваємо файл або директорію
  }

  root.close();  // Закриваємо корінь
}
void wavHeader(byte *header, int wavSize) {
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';                           // "RIFF"
  unsigned int fileSize = wavSize + 44 - 8;  // Обчислюємо розмір файлу
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';  // "WAVE"
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;  // Subchunk1Size = 16
  header[20] = 0x01;
  header[21] = 0x00;  // AudioFormat = PCM
  header[22] = I2S_CHANNEL_NUM;
  header[23] = 0x00;  // NumChannels
  header[24] = (byte)(I2S_SAMPLE_RATE & 0xFF);
  header[25] = (byte)((I2S_SAMPLE_RATE >> 8) & 0xFF);
  header[26] = (byte)((I2S_SAMPLE_RATE >> 16) & 0xFF);
  header[27] = (byte)((I2S_SAMPLE_RATE >> 24) & 0xFF);  // SampleRate
  unsigned int byteRate = I2S_SAMPLE_RATE * I2S_CHANNEL_NUM * I2S_SAMPLE_BITS / 8;
  header[28] = (byte)(byteRate & 0xFF);
  header[29] = (byte)((byteRate >> 8) & 0xFF);
  header[30] = (byte)((byteRate >> 16) & 0xFF);
  header[31] = (byte)((byteRate >> 24) & 0xFF);  // ByteRate
  header[32] = (byte)(I2S_CHANNEL_NUM * I2S_SAMPLE_BITS / 8);
  header[33] = 0x00;  // BlockAlign
  header[34] = I2S_SAMPLE_BITS;
  header[35] = 0x00;  // BitsPerSample
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';  // "data"
  header[40] = (byte)(wavSize & 0xFF);
  header[41] = (byte)((wavSize >> 8) & 0xFF);
  header[42] = (byte)((wavSize >> 16) & 0xFF);
  header[43] = (byte)((wavSize >> 24) & 0xFF);  // DataSize
}
void startRecording() {
  if (!SD.begin(SD_CS)) {
    Serial.println("Помилка ініціалізації SD-карти!");
    drawTextMessage("SD card initialization error!");
    return;
  }

  char filename[32];
  snprintf(filename, sizeof(filename), "/voice.wav");
  Serial.printf("Запис у файл: %s\n", filename);

  audioFile = SD.open(filename, FILE_WRITE);
  if (!audioFile) {
    drawTextMessage("Could not open file!");
    Serial.println("Не вдалося відкрити файл!");
    deleteVoiceFile();
    return;
  }

  byte header[44];
  wavHeader(header, 0x7FFFFFFF);  // Попередньо записуємо заголовок з макс. розміром
  audioFile.write(header, 44);
  audioFile.flush();

  recording = true;
}
void stopRecording() {
  recording = false;
  if (audioFile) {
    audioFile.flush();
    audioFile.close();
    audioFile = File();
    Serial.println("Запис завершено.");
  }

  // Перевірка чи файл успішно створено
  if (SD.exists("/voice.wav")) {
    Serial.println("[OK] Файл voice.wav існує.");
  } else {
    Serial.println("[ERROR] Файл voice.wav не знайдено!");
  }
}
void recordAudio() {
  static char *i2s_read_buff = (char *)calloc(I2S_READ_LEN, sizeof(char));
  static uint8_t *flash_write_buff = (uint8_t *)calloc(I2S_READ_LEN, sizeof(char));
  static float filtered_sample = 0;
  size_t bytes_read;

  if (!i2s_read_buff || !flash_write_buff) return;

  if (i2s_read(I2S_PORT, (void *)i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY) == ESP_OK && bytes_read > 0) {
    int32_t sum_amplitude = 0;  // Сума абсолютних значень для обчислення середньої гучності
    int sample_count = 0;

    for (int i = 0; i < bytes_read; i += 2) {
      int16_t *sample = (int16_t *)(i2s_read_buff + i);

      // Фільтрація
      filtered_sample = FILTER_COEF * filtered_sample + (1.0f - FILTER_COEF) * (*sample);

      // Підсилення
      int32_t amplified_sample = (int32_t)(filtered_sample * GAIN);

      // Обмеження
      if (amplified_sample > 32767) amplified_sample = 32767;
      if (amplified_sample < -32768) amplified_sample = -32768;

      int16_t out_sample = (int16_t)amplified_sample;
      flash_write_buff[i] = out_sample & 0xFF;
      flash_write_buff[i + 1] = (out_sample >> 8) & 0xFF;

      // Для гучності
      sum_amplitude += abs(out_sample);
      sample_count++;
    }

    if (sample_count > 0) {
      float avg_amplitude = (float)sum_amplitude / sample_count;
      volumeMic = 10.0f + (avg_amplitude / 32767.0f) * (22.0f - 10.0f);
    } else {
      volumeMic = 10.0f;
    }

    audioFile.write((const byte *)flash_write_buff, bytes_read);
  }
}
void i2s_adc_data_scale(uint8_t *d_buff, uint8_t *s_buff, uint32_t len) {
  int16_t *input = (int16_t *)s_buff;
  int16_t *output = (int16_t *)d_buff;
  uint32_t samples = len / 2;
  int16_t max_val = 1;
  for (uint32_t i = 0; i < samples; i++) {
    int16_t val = input[i];
    if (abs(val) > max_val) max_val = abs(val);
  }
  float gain = 32767.0f / max_val;
  for (uint32_t i = 1; i < samples - 1; i++) {
    int32_t smoothed = (input[i - 1] + input[i] + input[i + 1]) / 3;
    float scaled = smoothed * gain;
    if (scaled > 32767) scaled = 32767;
    if (scaled < -32768) scaled = -32768;
    output[i] = (int16_t)scaled;
  }
  output[0] = (int16_t)(input[0] * gain);
  output[samples - 1] = (int16_t)(input[samples - 1] * gain);
}

bool isButtonPressed(int pin) {
  curKey = digitalRead(pin);
  if (curKey == LOW) {
    if (millis() - whenKeyPress > 500) {
      whenKeyPress = millis();
      return true;
    }
  }
  return false;
}

void scanWiFiNetworks() {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("Scanning Wi-Fi"));
  display.setCursor(0, 9);
  display.print(F("networks..."));
  display.display();

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);

  int networkCount = WiFi.scanNetworks();
  foundNetworks = networkCount;

  if (foundNetworks <= 0) {
    Serial.println("Мереж не знайдено.");
    return;
  }

  wifiList = new WiFiNetworkInfo[foundNetworks];

  for (int i = 0; i < foundNetworks; ++i) {
    wifiList[i].ssid = WiFi.SSID(i);
    wifiList[i].rssi = WiFi.RSSI(i);
    wifiList[i].encryption = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;

    Serial.printf("%2d. SSID: %-25s RSSI: %4d dBm  Тип: %s\n",
                  i + 1,
                  wifiList[i].ssid.c_str(),
                  wifiList[i].rssi,
                  wifiList[i].encryption ? "Відкрита" : "Захищена");
  }

  WiFi.scanDelete();
}
void clearWiFiList() {
  if (wifiList != nullptr) {
    delete[] wifiList;
    wifiList = nullptr;
    foundNetworks = 0;
  }
}

void drawRSSIElements(int32_t rssi, int x, int y) {

  if (rssi >= -70) {
    display.drawLine(x, y, x, y, SSD1306_WHITE);
    display.drawLine(x + 2, y - 3, x + 2, y, SSD1306_WHITE);
    display.drawLine(x + 4, y - 6, x + 4, y, SSD1306_WHITE);
  } else if (rssi >= -80) {
    display.drawLine(x, y, x, y, SSD1306_WHITE);
    display.drawLine(x + 2, y - 3, x + 2, y, SSD1306_WHITE);
  } else if (rssi >= -90) {
    display.drawLine(x, y, x, y, SSD1306_WHITE);
  }
}
void drawBatteryElements(int value, int x, int y) {
  static int currentBars = 0;
  float voltage = (value / 4095.0) * 3.3 * 2;

  if (currentBars == 3) {
    if (voltage < 3.9) currentBars = 2;
  } else if (currentBars == 2) {
    if (voltage >= 3.95) currentBars = 3;
    else if (voltage < 3.65) currentBars = 1;
  } else if (currentBars == 1) {
    if (voltage >= 3.85) currentBars = 2;
    else if (voltage < 3.2) currentBars = 0;
  } else {
    if (voltage >= 3.25) currentBars = 1;
  }

  display.fillRect(x, y, 15, 10, SSD1306_BLACK);
  display.drawBitmap(x, y, epd_bitmap_battery, 15, 10, SSD1306_WHITE);

  for (int i = 0; i < currentBars; i++) {
    display.fillRect(x + 10 - i * 3, y + 3, 2, 4, SSD1306_WHITE);
  }
}
void drawElementsWifiList() {
  display.clearDisplay();

  if (foundNetworks == 0 || wifiList == nullptr) {
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(1, 1);
    display.print(F("No networks found"));
  }

  for (int i = 0; i < foundNetworks; ++i) {
    display.setCursor(1, 2 + i * 10 - curPosElementWifiList * 10);
    String ssidToPrint = wifiList[i].ssid;
    if (ssidToPrint.length() > 12) {
      ssidToPrint = ssidToPrint.substring(0, 12) + "..";
    }

    if (curPosElementWifiList == i) {
      display.fillRect(0, 1, 95, 10, SSD1306_WHITE);
      currentNetworkWifiList = wifiList[i].ssid;
      encryptionEncryptionWifiList = wifiList[i].encryption;
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(ssidToPrint);

    display.fillRect(98, 8 + i * 10 - curPosElementWifiList * 10, 6, 8, SSD1306_BLACK);
    drawRSSIElements(wifiList[i].rssi, 98, 8 + i * 10 - curPosElementWifiList * 10);
    if (!wifiList[i].encryption) { display.drawBitmap(108, 1 + i * 10 - curPosElementWifiList * 10, epd_bitmap_lock, 7, 8, SSD1306_WHITE); }
    if (ssid == wifiList[i].ssid) {
      display.drawBitmap(85, 2 + i * 10 - curPosElementWifiList * 10, epd_bitmap_check, 9, 8, SSD1306_BLACK);
    }
  }

  drawScroll(124, 1, curPosElementWifiList, foundNetworks);

  display.fillRect(0, 51, 128, 14, SSD1306_BLACK);
  display.drawBitmap(10, 51, epd_bitmap_enter, 12, 12, SSD1306_WHITE);
  display.drawBitmap(107, 53, epd_bitmap_back, 11, 8, SSD1306_WHITE);

  updateEncoderValue(curPosElementWifiList, 1, 0, foundNetworks - 1);

  if (isButtonPressed(LEFT_BUTTON_PIN) && foundNetworks > 0) {
    currentPage = WIFI_CONNECT;
  }

  if (isButtonPressed(RIGHT_BUTTON_PIN)) {
    currentPage = MENU;
    tryConnectWiFi(ssid.c_str(), pass.c_str());
  }

  display.display();
}
void drawElementsMenu() {
  display.clearDisplay();

  for (int i = 0; i < totalMenuPages; ++i) {
    display.setCursor(1, 2 + i * 10 - curPosMenuList * 10);

    if (curPosMenuList == i) {
      display.fillRect(0, 1, 122, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(settingsMenuItems[i]);
  }

  drawScroll(124, 1, curPosMenuList, totalMenuPages);

  display.fillRect(0, 51, 128, 14, SSD1306_BLACK);
  display.drawBitmap(10, 51, epd_bitmap_enter, 12, 12, SSD1306_WHITE);
  display.drawBitmap(106, 51, epd_bitmap_home, 12, 12, SSD1306_WHITE);

  display.setCursor(52, 53);
  display.setTextColor(SSD1306_WHITE);
  display.print("menu");

  updateEncoderValue(curPosMenuList, 1, 0, totalMenuPages - 1);
  if (isButtonPressed(LEFT_BUTTON_PIN)) {
    switch (curPosMenuList) {
      case 0:
        currentPage = WIFI_LIST;
        break;
      case 1:
        currentPage = AI_SETTINGS;
        break;
      case 2:
        currentPage = DEVICE_INFO;
        break;
      case 3:
        currentPage = SET_TIME;
        break;
      case 4:
        currentPage = BLUETOOTH;
        break;
    }
  }

  if (isButtonPressed(RIGHT_BUTTON_PIN)) {
    currentPage = MAIN_PAGE;
  }

  display.display();
}
void drawElementsWifiConnect() {
  display.clearDisplay();

  display.setCursor(7, 1);
  display.setTextColor(SSD1306_WHITE);
  display.print("Pocket assistant AI");

  display.drawBitmap(0, 20, epd_bitmap_wifi, 34, 25, SSD1306_WHITE);

  display.setFont(&FreeSans12pt7b);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(39, 35);
  display.println("Set WiFi");

  display.setFont();
  display.setTextSize(1);
  display.setCursor(39, 38);
  display.println("ip:" + WiFi.softAPIP().toString());

  display.drawBitmap(10, 51, epd_bitmap_home, 12, 12, SSD1306_WHITE);
  display.drawBitmap(107, 53, epd_bitmap_back, 11, 8, SSD1306_WHITE);

  display.display();
}

String getCoordinatesFromAPI() {
  String timezone = "";

  for (int attempt = 0; attempt < 3; attempt++) {
    HTTPClient http;
    http.begin(geoLocationApiUrl);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println(payload);

      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        timezone = doc["timezone"].as<String>();
        http.end();
        return timezone;
      } else {
        Serial.print("JSON Error: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpCode);
    }

    http.end();
  }

  return timezone;  // Поверне порожній рядок, якщо всі 3 спроби не вдалися
}

void drawTime(int x, int y) {
  char timeStr[6];
  char dateStr[11];
  sprintf(timeStr, "%02d:%02d", currentHour, currentMinute);
  sprintf(dateStr, "%02d.%02d.%04d", currentDay, currentMonth, currentYear);

  display.fillRect(x, y, 60, 30, SSD1306_BLACK);

  display.setFont(&FreeSans12pt7b);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x + 1, y + 17);
  display.println(timeStr);

  display.setFont();
  display.setTextSize(1);
  display.setCursor(x, y + 20);
  display.println(dateStr);
}

void updateTimeFromAPI() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  int attempt = 0;
  int code = -1;
  String payload = "";
  unsigned long attemptStart = 0;
  bool success = false;

  while (attempt < 3 && !success) {
    attempt++;
    Serial.print("Спроба з'єднання №");
    Serial.println(attempt);

    String timezone = getCoordinatesFromAPI();
    if (timezone != "") {
      String fullUrl = timeApiUrl + timezone;
      http.begin(fullUrl);
    } else {
      Serial.println("Не вдалося отримати часовий пояс.");
      continue;
    }

    attemptStart = millis();
    bool timeoutReached = false;

    while (!timeoutReached && payload == "") {
      code = http.GET();

      if (code == 200) {
        payload = http.getString();
        if (payload != "") {
          Serial.println(payload);
          success = true;
          break;
        } else {
          Serial.println("Отримано порожню відповідь, повторюю запит...");
        }
      } else {
        Serial.print("HTTP помилка: ");
        Serial.println(code);
      }

      if (millis() - attemptStart > 5000) {
        timeoutReached = true;
      }
    }

    http.end();
  }

  if (!success) {
    Serial.println("Не вдалося отримати дані після 3 спроб.");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("Помилка JSON: ");
    Serial.println(error.c_str());
    return;
  }

  currentDay = doc["day"];
  currentMonth = doc["month"];
  currentYear = doc["year"];
  currentHour = doc["hour"];
  currentMinute = doc["minute"];
  currentSecond = doc["seconds"];
}
unsigned long convertDateTimeToEpoch(int year, int month, int day, int hour, int minute, int second) {
  int a = (14 - month) / 12;
  int y = year + 4800 - a;
  int m = month + 12 * a - 3;
  long julian_day = day + ((153 * m + 2) / 5) + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
  long days_since_epoch = julian_day - 2440588;
  return days_since_epoch * 86400UL + hour * 3600UL + minute * 60UL + second;
}
void updateDiviceTime() {
  if (millis() - lastUpdateDeviceTime >= 1000) {
    currentSecond++;

    if (currentSecond > 59) {
      currentSecond = 0;
      currentMinute++;
    }

    if (currentMinute > 59) {
      currentMinute = 0;
      currentHour++;
    }

    if (currentHour > 23) {
      currentHour = 0;
      currentDay++;
    }

    if ((currentMonth == 1 || currentMonth == 3 || currentMonth == 5 || currentMonth == 7 || currentMonth == 8 || currentMonth == 10 || currentMonth == 12) && currentDay > 31) {
      currentDay = 1;
      currentMonth++;
    } else if ((currentMonth == 4 || currentMonth == 6 || currentMonth == 9 || currentMonth == 11) && currentDay > 30) {
      currentDay = 1;
      currentMonth++;
    } else if (currentMonth == 2 && currentDay > 28) {
      currentDay = 1;
      currentMonth++;
    }

    if (currentMonth > 12) {
      currentMonth = 1;
      currentYear++;
    }

    lastUpdateDeviceTime = millis();
  }

  unsigned long epochEeprom = convertDateTimeToEpoch(EEPROM.read(5) + 2000, EEPROM.read(4), EEPROM.read(3),
                                                     EEPROM.read(0), EEPROM.read(1), EEPROM.read(2));
  unsigned long epochApi = convertDateTimeToEpoch(currentYear, currentMonth, currentDay,
                                                  currentHour, currentMinute, currentSecond);

  unsigned long timeDifferenceMs = ((long)epochApi - (long)epochEeprom) * 1000;
  if (millis() - lastSaveTime > 1200000 || timeDifferenceMs > 1200000) {
    saveToEEPROM();
    lastSaveTime = millis();
  }
}

int readADC_GPIO33_Raw() {
  adc1_config_width(ADC_WIDTH_BIT_12); // 12-бітна точність
  adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11); // до 3.6В
  int raw = adc1_get_raw(ADC1_CHANNEL_5);
  return raw;
}

void drawStaticElementsMainPage() {
  display.clearDisplay();

  display.drawBitmap(0, 0, epd_bitmap_rssi, 7, 13, SSD1306_WHITE);
  if (WiFi.status() == WL_CONNECTED) {
    drawRSSIElements(WiFi.RSSI(), 6, 10);
  } else display.drawBitmap(6, 7, epd_bitmap_x_icon, 5, 5, SSD1306_WHITE);

  if (isBTOn) { display.drawBitmap(15, 0, epd_bitmap_bluetooth, 11, 11, SSD1306_WHITE); }

  int rawValue = readADC_GPIO33_Raw();
  drawBatteryElements(rawValue, 113, 0);  // Заглушка поки 33 пін не використовується із за конфлікту

  display.drawBitmap(100, 0, epd_bitmap_speaker, 8, 10, SSD1306_WHITE);
  display.drawLine(107, 3, 107, 6, SSD1306_WHITE);
  display.drawLine(109, 2, 109, 7, SSD1306_WHITE);

  display.drawBitmap(85, 20, epd_bitmap_home_main, 24, 24, SSD1306_WHITE);

  drawScroll(124, 14, curPosMainList, totalMainPages);

  display.drawBitmap(11, 51, epd_bitmap_menu, 11, 11, SSD1306_WHITE);
  display.drawBitmap(95, 51, epd_bitmap_display_off, 23, 12, SSD1306_WHITE);

  drawTime(20, 18);

  display.display();
}
void handleAiButton() {
  isEncoderPressed = !digitalRead(ENCODER_BUTTON_PIN);

  // Дебаунс кнопки
  if (isEncoderPressed && !aiBtnPressed && millis() - lastAiBtnPressTime > 50 && !isSending && WiFi.status() == WL_CONNECTED && wifiConnectDrawn) {
    aiTestResponse = "";
    curPosTextScroll = 0;
    lastAiBtnPressTime = millis();
    aiBtnPressed = true;

    startRecording();
    currentPage = AI_REQUEST;
    recordingStartTime = millis();  // фіксуємо момент початку запису
  }
}
void drawElementsMainRequestChatGPT() {
  if (!isEncoderPressed && aiBtnPressed && millis() - lastAiBtnPressTime > 50) {
    lastAiBtnPressTime = millis();
    aiBtnPressed = false;

    if (recording) {
      isSending = true;
      stopRecording();
      drawTextMessage("Recording finished");
      listFiles();
      sendVoiceFile();
      deleteVoiceFile();
      isSending = false;
    }
  }

  // Якщо іде запис — малюємо таймер і контролюємо ліміт 10 секунд
  if (recording) {
    recordAudio();

    unsigned long elapsed = (millis() - recordingStartTime) / 1000;  // секунд пройшло
    if (elapsed >= 10) {
      isSending = true;
      stopRecording();
      drawTextMessage("Recording finished");
      listFiles();
      sendVoiceFile();
      deleteVoiceFile();
      isSending = false;
    } else {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(34, 0);
      display.print(F("Time left:"));
      display.drawRect(18, 10, 70, 8, SSD1306_WHITE);
      display.fillRect(18, 10, map(10 - elapsed, 0, 10, 0, 70), 8, SSD1306_WHITE);
      display.setCursor(92, 10);
      display.print(10 - elapsed);
      display.print("s");
      display.fillCircle(64, 41, volumeMic, SSD1306_WHITE);
      display.fillCircle(64, 41, 11, SSD1306_BLACK);
      display.drawBitmap(53, 30, epd_bitmap_microphone, 23, 23, SSD1306_WHITE);
      display.display();
    }
  }
}
void drawElementsAISettings() {
  settingsAIItems[0] = modelsAI[modelAIIndex];
  sprintf(tokensBuffer, "Tokens: %d", tokens);
  settingsAIItems[1] = tokensBuffer;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawScroll(124, 1, curPosAISettings, 1);
  for (int i = 0; i < 2; ++i) {
    display.setCursor(1, 2 + i * 10 - curPosAISettings * 10);

    if (curPosAISettings == i) {
      display.fillRect(0, 1, 122, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.print(settingsAIItems[i]);
  }
  display.fillRect(0, 51, 128, 14, SSD1306_BLACK);
  if (!isAISettingsEdit) display.drawBitmap(10, 51, epd_bitmap_edit, 12, 12, SSD1306_WHITE);
  else display.drawBitmap(12, 53, epd_bitmap_check, 9, 8, SSD1306_WHITE);
  display.drawBitmap(107, 53, epd_bitmap_back, 11, 8, SSD1306_WHITE);
  display.display();

  if (!isAISettingsEdit) updateEncoderValue(curPosAISettings, 1, 0, 1);
  else {
    switch (curPosAISettings) {
      case 0:
        updateEncoderValue(modelAIIndex, 1, 0, 2);
        break;
      case 1:
        updateEncoderValue(tokens, 10, 50, 5000);
        break;
    }
  }

  if (isButtonPressed(LEFT_BUTTON_PIN)) {
    isAISettingsEdit = !isAISettingsEdit;
  }
  if (isButtonPressed(RIGHT_BUTTON_PIN)) {
    saveToEEPROM();
    isAISettingsEdit = false;
    currentPage = MENU;
  }
}
void drawElementsSetTime() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(40, 6);
  display.println("Set time");

  char timeStr[6];
  char dateStr[11];
  char fullTime[20];
  sprintf(timeStr, "%02d:%02d", currentHour, currentMinute);
  sprintf(dateStr, "%02d.%02d.%04d", currentDay, currentMonth, currentYear);
  sprintf(fullTime, "%s/%s", timeStr, dateStr);

  display.fillRect(16, 25, 96, 15, SSD1306_BLACK);

  display.setTextSize(1);
  display.setCursor(16, 25);
  display.println(fullTime);

  if (curPosSetTime != 4) display.fillRect(16 + curPosSetTime * 18, 33, 11, 1, SSD1306_WHITE);
  else display.fillRect(16 + curPosSetTime * 18, 33, 23, 1, SSD1306_WHITE);

  display.fillRect(0, 51, 128, 14, SSD1306_BLACK);
  if (!isSetTimeEdit) display.drawBitmap(10, 51, epd_bitmap_edit, 12, 12, SSD1306_WHITE);
  else display.drawBitmap(12, 53, epd_bitmap_check, 9, 8, SSD1306_WHITE);
  display.drawBitmap(107, 53, epd_bitmap_back, 11, 8, SSD1306_WHITE);

  if (!isSetTimeEdit) updateEncoderValue(curPosSetTime, 1, 0, 4);
  else {
    switch (curPosSetTime) {
      case 0:
        updateEncoderValue(currentHour, 1, 1, 23);
        break;
      case 1:
        updateEncoderValue(currentMinute, 1, 0, 59);
        break;
      case 2:
        updateEncoderValue(currentDay, 1, 1, 31);
        break;
      case 3:
        updateEncoderValue(currentMonth, 1, 1, 11);
        break;
      case 4:
        updateEncoderValue(currentYear, 1, 1999, 9999);
        break;
    }
  }

  display.display();

  if (isButtonPressed(LEFT_BUTTON_PIN)) {
    isSetTimeEdit = !isSetTimeEdit;
  }
  if (isButtonPressed(RIGHT_BUTTON_PIN)) {
    saveToEEPROM();
    isSetTimeEdit = false;
    currentPage = MENU;
  }
}
void drawElementsBluetooth() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(13, 5);
  display.println("PocketAssistantBT");

  display.drawBitmap(13, 16, epd_bitmap_bluetooth_big, 32, 32, SSD1306_WHITE);

  display.setFont(&FreeSans12pt7b);
  display.setCursor(60, 40);
  if (isBTOn) {
    display.println("ON");
    display.setFont();
    display.setCursor(13, 54);
    display.println("Turn OFF");
  } else {
    display.println("OFF");
    display.setFont();
    display.setCursor(13, 54);
    display.println("Turn ON");
  }

  display.drawBitmap(104, 53, epd_bitmap_back, 11, 8, SSD1306_WHITE);

  display.display();

  if (isButtonPressed(LEFT_BUTTON_PIN)) {
    if (isBTOn) {
      isBTOn = !isBTOn;
      audio.end();
      i2sDeInit();
      i2sInit();
    } else {
      isBTOn = !isBTOn;
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      i2sDeInit();
      audio.begin();
      audio.reconnect();
      audio.I2S(I2S_BCLK, I2S_DOUT, I2S_LRC);
    }
  }
}
void drawDeviceInfo() {
  String deviceInfoText = "Wi-Fi SSID:Pocket assistant AI\nCore:ESP32-WROOM-32\nAudio Input:INMP441\nAudio Out:MAX98357A\nAcoustic Output:93dbDisplay:OLED 0.96";
  drawTextMessageWithScroll(deviceInfoText);
  if (isButtonPressed(RIGHT_BUTTON_PIN)) {
    currentPage = MENU;
  }
}

void renderPage() {
  switch (currentPage) {
    case MAIN_PAGE:
      updateEncoderValue(curPosMainList, 1, 0, totalMainPages);
      switch (curPosMainList) {
        case 0:
          if (isButtonPressed(LEFT_BUTTON_PIN)) {
            curPosMenuList = 0;
            currentPage = MENU;
          }
          if (isButtonPressed(RIGHT_BUTTON_PIN)) {
          }

          drawStaticElementsMainPage();
          if (!mainPageDrawn) {
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(28, 2);
            display.print(F("Connecting.."));
            display.display();
            if (ssid.length() > 0) {
              if (tryConnectWiFi(ssid.c_str(), pass.c_str())) {
                updateTimeFromAPI();
              }
            }
            display.fillRect(28, 0, 72, 10, SSD1306_BLACK);
            mainPageDrawn = true;
          }
          break;
      }
      break;
    case AI_REQUEST:
      drawElementsMainRequestChatGPT();
      if (aiTestResponse.length() > 0) {
        drawTextMessageWithScroll(aiTestResponse);
        playAudioFile();
        aiTestResponse = "";
      }
      if (isButtonPressed(RIGHT_BUTTON_PIN) && !isSending) {
        currentPage = MAIN_PAGE;
        aiTestResponse = "";
        curPosTextScroll = 0;
      }
      break;
    case MENU:
      drawElementsMenu();
      if (!wifiConnectDrawn) {
        wifiConnectDrawn = true;
        curPosElementWifiList = 0;
        clearWiFiList();
      }
      break;
    case AI_SETTINGS:
      drawElementsAISettings();
      break;
    case SET_TIME:
      drawElementsSetTime();
      break;
    case WIFI_LIST:
      if (wifiConnectDrawn) {
        wifiConnectDrawn = false;
        scanWiFiNetworks();
      }
      drawElementsWifiList();
      break;
    case WIFI_CONNECT:
      if (isButtonPressed(LEFT_BUTTON_PIN)) {
        currentPage = MAIN_PAGE;
        tryConnectWiFi(ssid.c_str(), pass.c_str());
      }
      if (isButtonPressed(RIGHT_BUTTON_PIN)) {
        currentPage = WIFI_LIST;
      }
      if (!wifiConnectDrawn) {
        if (!encryptionEncryptionWifiList) {
          startAPServer();
          drawElementsWifiConnect();
        } else {
          drawElementsWifiConnect();
          drawConnectToWiFi("", encryptionEncryptionWifiList);
        }
        wifiConnectDrawn = true;
        curPosElementWifiList = 0;
        clearWiFiList();
      }
      break;
    case BLUETOOTH:
      drawElementsBluetooth();
      if (isButtonPressed(RIGHT_BUTTON_PIN)) {
        isBTOn = false;
        audio.end();
        i2sDeInit();
        i2sInit();
        tryConnectWiFi(ssid.c_str(), pass.c_str());
        currentPage = MENU;
      }
      break;
    case DEVICE_INFO:
      drawDeviceInfo();
      break;
  }
}
void setup() {
  Serial.begin(115200);
  delay(1000);

  EEPROM.begin(EEPROM_SIZE);

  loadFromEEPROM();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Помилка ініціалізації дисплея"));
    while (true)
      ;
  }

  if (!SD.begin(SD_CS)) {
    Serial.println("Помилка ініціалізації SD-картки!");
  }

  audio.I2S(I2S_BCLK, I2S_DOUT, I2S_LRC);
  audio.end();
  i2sDeInit();
  i2sInit();

  pinMode(Encoder_S1, INPUT);
  pinMode(Encoder_S2, INPUT);
  pinMode(Encoder_key, INPUT);

  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);

  encoderPrevS1 = digitalRead(Encoder_S1);

  WiFi.mode(WIFI_STA);

  display.clearDisplay();
  lastSaveTime = millis();
}
void loop() {
  server.handleClient();
  renderPage();
  if (!isSetTimeEdit) updateDiviceTime();
  handleAiButton();
}