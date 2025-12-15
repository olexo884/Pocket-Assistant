# Pocket Assistant — Experimental Autonomous Voice Assistant

**Pocket Assistant** is an experimental autonomous voice assistant prototype built around **ESP32**, combining embedded hardware, UI design, audio processing, and an AI-powered backend.

The project is split into two tightly connected parts:

* **ESP32 Client (Firmware / PlatformIO)** — handles UI, menu navigation, audio input/output, Bluetooth, and communication with the server.
* **Python Server (Flask)** — processes voice input, communicates with OpenAI, and generates both text and voice responses.

This project focuses on **real hardware interaction**, not just software simulation.

---

## Design & Hardware References

* **UI / UX (Figma):**
  [https://www.figma.com/design/nQ5vYbNvR1JXNJ7YZx3jHn/Pocket-Assistant-%E2%80%94-Experimental-Autonomous-Voice-Assistant](https://www.figma.com/design/nQ5vYbNvR1JXNJ7YZx3jHn/Pocket-Assistant-%E2%80%94-Experimental-Autonomous-Voice-Assistant)

* **Hardware schematic (EasyEDA):**
  [https://oshwlab.com/olexo884/pocket-assistant-experimental-autonomous-voice-assistant](https://oshwlab.com/olexo884/pocket-assistant-experimental-autonomous-voice-assistant)

---

## Core Features

### Device Interface & Menu System

The device is controlled using a **rotary encoder with a button** and additional physical buttons.
All interaction happens directly on the device — no phone or PC required.

The menu allows you to:

* Configure **Wi-Fi** (SSID, password, connection status)
* Adjust **AI-related settings**
* Manually set **date and time**
* Use **Bluetooth audio mode** to listen to music

### Bluetooth Audio

The device works as a Bluetooth speaker.

* **Bluetooth name:** `PocketAssistantBT`
* Audio is played through **MAX98357A (I2S amplifier) + speaker**

---

## Voice Assistant Mode (ESP32 → Server)

Once the device is connected to Wi-Fi, it can work as a voice assistant.

**How it works:**

* A **long press on the encoder button** starts voice interaction
* ESP32 records an audio file
* The audio file is sent to the server
* The server:

  * Converts voice → text (STT)
  * Sends the text to OpenAI
  * Converts the response text → audio (TTS)
* ESP32 receives:

  * recognized input text
  * AI response text
  * audio response
* The device:

  * displays the text on the OLED screen
  * plays the audio response through the speaker

This creates a full **voice → AI → voice** pipeline using real hardware.

---

## Hardware Overview (Prototype)

This is a **working prototype**, not a finalized commercial design.

Main components:

* ESP32
* 0.96" monochrome OLED display (128×64)
* **INMP441** I2S microphone
* **MAX98357A** I2S mono audio amplifier
* Speaker (4Ω or 8Ω)
* Rotary encoder with button
* Control buttons
* Li-ion 3.7V battery with protection/charging circuitry
* Optional SD-card module

Power management and PCB layout are based on previous working revisions and are still evolving.

---

## Project Structure (Recommended)

```
pocket-assistant/
  firmware-esp32/   → ESP32 firmware (PlatformIO)
  server/           → Python Flask backend
  docs/             → diagrams, screenshots, photos
  README.md
```

---

## System Architecture (High Level)

1. ESP32 connects to Wi-Fi
2. User presses and holds the encoder button
3. ESP32 records voice input
4. Audio file is sent to the Flask server
5. Server performs:

   * Speech-to-Text
   * AI request (OpenAI)
   * Text-to-Speech
6. ESP32 receives text + audio response
7. Text is shown on display, audio is played via speaker

---

## Server Setup (Python / Flask)

The backend server handles AI and audio processing.

The OpenAI client is initialized like this:

```python
client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"))
```

### OpenAI API Key Setup

1. Create an API key in the OpenAI dashboard
2. Add it to an `.env` file inside the `server` folder:

```env
OPENAI_API_KEY=your_openai_api_key_here
```

> The `.env` file is intentionally excluded from git.

The server uses Flask and handles endpoints for receiving audio, processing it, and returning text and audio responses.

---

## ESP32 Firmware

* Built with **PlatformIO**
* Handles:

  * UI rendering
  * Button and encoder input
  * Audio recording and playback
  * Wi-Fi and Bluetooth management
  * Communication with the server

Most logic currently lives in a single main file due to active prototyping and rapid iteration.

---

## Project Status

* This is an **experimental R&D project**
* Hardware and software are functional but still evolving
* Some parts are intentionally monolithic to avoid breaking working features
* Planned future improvements:

  * Code modularization
  * Power optimization
  * PCB refinement
  * UI polish and animation

---

## Why This Project Matters

This project demonstrates:

* Embedded systems development (ESP32)
* Real hardware interaction
* Audio processing (I2S, microphones, amplifiers)
* Client-server architecture
* AI integration in physical devices
* UI/UX thinking beyond pure software

It is designed to be **hands-on, practical, and real**, not just theoretical.

---

## Author

**Oleksii Shevchuk**
ESP32 • C++ • Python • EasyEDA • IoT / Embedded

---
