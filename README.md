# 🤖 Pocket Assistant — Experimental Autonomous Voice Assistant

**Pocket Assistant** is an experimental autonomous voice assistant prototype built around **ESP32**, combining embedded hardware, UI design, audio processing, and an AI-powered backend.

The project is split into two tightly connected parts:

* 🔌 **ESP32 Client (Firmware / PlatformIO)** — handles UI, menu navigation, audio input/output, Bluetooth, and communication with the server.
* 🖥️ **Python Server (Flask)** — processes voice input, communicates with OpenAI, and generates both text and voice responses.

This project focuses on **real hardware interaction**, not just software simulation.

---

## 🎨 Design & Hardware References

* 🧩 **UI / UX (Figma):**
  [https://www.figma.com/design/nQ5vYbNvR1JXNJ7YZx3jHn/Pocket-Assistant-%E2%80%94-Experimental-Autonomous-Voice-Assistant](https://www.figma.com/design/nQ5vYbNvR1JXNJ7YZx3jHn/Pocket-Assistant-%E2%80%94-Experimental-Autonomous-Voice-Assistant)

* 🛠️ **Hardware schematic (EasyEDA):**
  [https://oshwlab.com/olexo884/pocket-assistant-experimental-autonomous-voice-assistant](https://oshwlab.com/olexo884/pocket-assistant-experimental-autonomous-voice-assistant)

---

## ✨ Core Features

### 📟 Device Interface & Menu System

The device is controlled using a **rotary encoder with a button** and additional physical buttons.
All interaction happens directly on the device — **no phone or PC required**.

The menu allows you to:

* 📶 Configure **Wi-Fi** (SSID, password, connection status)
* 🧠 Adjust **AI-related settings**
* 🕒 Manually set **date and time**
* 🎵 Use **Bluetooth audio mode** to listen to music

---

### 🔊 Bluetooth Audio

The device works as a Bluetooth speaker.

* 📡 **Bluetooth name:** `PocketAssistantBT`
* 🔈 Audio output via **MAX98357A (I2S amplifier) + speaker**

---

## 🎙️ Voice Assistant Mode (ESP32 → Server)

Once the device is connected to Wi-Fi, it can work as a full voice assistant.

**How it works:**

* ⏺️ Long press on the encoder button starts voice interaction
* 🎤 ESP32 records an audio file
* 🌐 Audio is sent to the server
* 🧠 Server pipeline:

  * Voice → Text (STT)
  * Text → OpenAI
  * Text → Voice (TTS)
* 📥 ESP32 receives:

  * recognized input text
  * AI response text
  * audio response
* 📺 Device:

  * displays text on OLED
  * plays audio via speaker

➡️ This creates a full **voice → AI → voice** pipeline using real hardware.

---

## 🔧 Hardware Overview (Prototype)

This is a **working prototype**, not a finalized commercial design.

Main components:

* ⚙️ ESP32
* 🖥️ 0.96" monochrome OLED display (128×64)
* 🎙️ **INMP441** I2S microphone
* 🔊 **MAX98357A** I2S mono amplifier
* 🔈 Speaker (4Ω / 8Ω)
* 🎛️ Rotary encoder with button
* ⏹️ Control buttons
* 🔋 Li-ion 3.7V battery + charging/protection
* 💾 Optional SD-card module

Power management and PCB layout are still evolving.

---

## 🗂️ Project Structure (Recommended)

```
pocket-assistant/
  firmware-esp32/   → ESP32 firmware (PlatformIO)
  server/           → Python Flask backend
  docs/             → diagrams, screenshots, photos
  README.md
```

---

## 🧩 System Architecture (High Level)

1. 📶 ESP32 connects to Wi-Fi
2. ⏺️ User presses and holds encoder button
3. 🎤 ESP32 records voice
4. 🌐 Audio is sent to Flask server
5. 🧠 Server performs:

   * Speech-to-Text
   * OpenAI request
   * Text-to-Speech
6. 📥 ESP32 receives response
7. 📺 Text displayed, 🔊 audio played

---

## 🖥️ Server Setup (Python / Flask)

The backend server handles AI and audio processing.

OpenAI client initialization:

```python
client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))
```

### 🔑 OpenAI API Key Setup

1. Create an API key in OpenAI dashboard
2. Add it to `.env` inside `server` folder:

```env
OPENAI_API_KEY=your_openai_api_key_here
```

> ⚠️ `.env` is excluded from git.

---

## 🔌 ESP32 Firmware

* Built with **PlatformIO**
* Handles:

  * 🎨 UI rendering
  * 🎛️ Button & encoder input
  * 🎤 Audio recording / playback
  * 📶 Wi-Fi & Bluetooth
  * 🌐 Server communication

Most logic is currently monolithic due to rapid prototyping.

---

## 🚧 Project Status

* 🧪 Experimental R&D project
* ✅ Functional hardware & software
* 🧱 Some monolithic code by design
* 🛠️ Planned improvements:

  * Code modularization
  * Power optimization
  * PCB refinement
  * UI polish & animation

---

## 🌍 Why This Project Matters

This project demonstrates:

* ⚡ Embedded systems (ESP32)
* 🔌 Real hardware interaction
* 🎧 Audio processing (I2S)
* 🌐 Client-server architecture
* 🤖 AI in physical devices
* 🎨 UI/UX beyond pure software

**Hands-on. Practical. Real.**

---

## 👤 Author

**Oleksii Shevchuk**
ESP32 • C++ • Python • EasyEDA • IoT / Embedded

---
