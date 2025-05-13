# Smart Home Control System – IoT Final Exam Project

This project is a simulation of an intelligent Smart Home environment using IoT principles and sensors. The system integrates temperature regulation, lighting control, motion detection, remote access, and cloud reporting. It was developed as a final exam project for the Internet of Things course.

## Features

### 🔥 Temperature Control
- **Heating System**: Controlled via relay and simulated with a resistor.
- **Cooling System**: Controlled via relay and simulated with a fan (DC motor).
- **Automatic Mode**: When enabled, turns cooling ON above 23°C and OFF at 17°C. Heating activates below 17°C and turns off at 23°C.
- Can be toggled ON/OFF via email.

### 💡 Lighting Control
- White LED controlled via serial commands.
- **Light Auto Mode**: Automatically turns ON if illumination is below 30%; OFF otherwise.
- Manual commands override and deactivate auto mode.

### 🕵️ Motion Detection & Secure Mode
- PIR motion sensor (HC-SR501).
- **Secure Mode**:
  - Turns ON the light when motion is detected.
  - Sends one email notification within a 10-second motion window.
  - Automatically turns OFF light after 10 seconds of no motion.

### 📧 Remote Control via Email
Control all system components by sending an email with the corresponding subject:
- Heating ON/OFF
- Cooling ON/OFF
- Auto Temp Control ON/OFF
- Light ON/OFF
- Light Auto Mode ON/OFF
- Secure Mode ON/OFF
- Emergency Mode OFF (Emergency mode is activated via button)

### ☁️ Cloud Integration – ThingSpeak
- Sends the following to ThingSpeak every 10 minutes:
  - Temperature
  - Illumination
  - Motion detection logs
- Daily report includes:
  1. Min/Max/Average temperature
  2. Temperature graph
  3. Min/Max/Average illumination
  4. Illumination graph
  5. Number of daily motion detections
  6. Detection graph
  7. Duration of Secure Mode and Light Auto Mode

### 🚨 Emergency Mode (Bonus)
- Activated by **emergency button** only.
- When active:
  - All systems shut down
  - **Red LED** turns ON
  - Secure Mode turns ON
  - Sends **emergency email**
- Deactivated **only via email**
- **Green LED** indicates system is running normally

## Hardware Components

- LM35 Temperature Sensor
- Photoresistor (Illumination)
- PIR Motion Sensor (HC-SR501)
- White LED
- Red & Green LEDs
- Resistor (Heater Simulation)
- DC Motor or Fan (Cooler Simulation)
- 3x Relays
- Emergency Push Button
- Microcontroller board (e.g., Arduino)
- Email Communication Module (via SMTP)
- ThingSpeak API Integration

## How It Works

- The system continuously monitors sensors every 10 minutes.
- Users can interact with it via serial or email.
- Email subjects are parsed to enable or disable modes.
- Daily summary is compiled and sent to ThingSpeak + optionally via email.

## Setup Instructions

1. Connect all components based on the circuit schematic (not included).
2. Upload the Arduino code to the microcontroller.
3. Configure:
   - Email account (SMTP server, credentials)
   - ThingSpeak API keys
4. Open Serial Monitor for manual light control (optional).

## Dependencies

- Arduino IDE
- `ESP8266WiFi.h`, `WiFiClientSecure.h`, `SMTPClient`, or equivalent libraries
- ThingSpeak library
- Timer-based scheduling (e.g., `SimpleTimer`)

## Author

**Luka Trunić**  
Student ID: 2021/230020  
Faculty of Technical Sciences, Novi Sad

---

