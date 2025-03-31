#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "Singidunum";
const char* password = "student";

// SMTP server credentials
const char* smtpServer = "smtp.gmail.com";
const int smtpPort = 465;
const char* smtpUser = "ltrunic@gmail.com";
const char* smtpPassword = "1234";

// Email details
const char* recipient = "ltrunic@gmail.com";

// ThingSpeak details
const char* thingSpeakServer = "https://api.thingspeak.com/update";
const char* apiKey = "YOUR_API_KEY";  // Write ThingSpeak API Key

// SMTP client 
SMTPSession smtp;

// Report
bool dailyReportSent = false;
unsigned long reportStartTime;

// Variables for temperature and illumination stats for updating
float minTemp = 100.0;
float maxTemp = -100.0;
float totalTemp = 0.0;
int tempCount = 0;

int minIllumination = 1023;
int maxIllumination = 0;
int totalIllumination = 0;
int illuminationCount = 0;

int motionDetections = 0;
unsigned long secureModeDuration = 0;
unsigned long lightAutoModeDuration = 0;

float tempMeasurements[144];  // Store 144 measurements (every 10 minutes for 24 hours)
int illumMeasurements[144];
int motionMeasurments[144];
int measurementIndex = 0;

// Define pins
const int greenLEDPin = 8;    // Green LED for Normal Mode
const int redLEDPin = 7;      // Red LED for Emergency Mode
const int buttonPin = 6;      // Emergency Mode Button
const int whiteLEDPin = 5;    // White LED for Light Indicator
const int motorRelayPin = 4;  // Relay for DC motor
const int lightBulbRelayPin = 3;  // Relay for light bulb
const int motionPin = 2;      // PIR Motion Sensor
const int lightSensorPin = A1; // Photoresistor sensor
const int tempSensorPin = A0;  // TMP36 temperature sensor

// Temperature thresholds
const float upperThreshold = 23.0;
const float lowerThreshold = 17.0;

// Modes
bool autoModeActive = true;
bool secureModeActive = false;
bool emergencyModeActive = false;
bool emergencyEmailSent = false;

// Time variables
unsigned long previousThingSpeakMillis = 0;
unsigned long previousTempMillis = 0;  // Timer for temperature and illumination
unsigned long lightOnMillis = 0;        // Timer to turn off light after motion
const unsigned long thingSpeakInterval = 15000;  // 15 seconds (minimum ThingSpeak update interval)
const unsigned long tempInterval = 100000;  // 10 minutes for temperature and light
const unsigned long lightOnDuration = 10000; // 10 seconds for light after motion

void setup() {
  // Initialize pins
  pinMode(motorRelayPin, OUTPUT);
  pinMode(lightBulbRelayPin, OUTPUT);
  pinMode(tempSensorPin, INPUT);
  pinMode(lightSensorPin, INPUT);
  pinMode(motionPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(redLEDPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(whiteLEDPin, OUTPUT);

  // Start serial communication for debugging
  Serial.begin(9600);

  // Connect to WiFi
  connectToWiFi();

  // Set the report start time
  reportStartTime = millis();

  // Initialize the SMTP session
  smtp.debug(1);
  smtp.callback(smtpCallback);

  // Initial State
  digitalWrite(redLEDPin, LOW);    // Red LED OFF
  digitalWrite(greenLEDPin, HIGH); // Green LED ON
  digitalWrite(whiteLEDPin, LOW);  // White LED OFF

  // Initial message
  Serial.println("Smart Home System Initialized.");
}

void loop() {
  
  // Reconnect WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectToWiFi();
  }
  
  // Check for incoming email commands
  checkEmail();

  if (emergencyModeActive) {
    handleMotion();
    checkEmergencyStatus();  // Check if Emergency Mode can be turned off
    return;  // Skip the rest of the loop
  }

  unsigned long currentMillis = millis();

  // Check temperature and illumination every 10 minutes
  if (currentMillis - previousTempMillis >= tempInterval) {
    previousTempMillis = currentMillis;
    measureTemperatureAndIllumination();
  }

  // Send data to ThingSpeak every 15 seconds
  if (currentMillis - previousThingSpeakMillis >= thingSpeakInterval) {
    previousThingSpeakMillis = currentMillis;

    if (tempCount > 0 && illuminationCount > 0) {
      float avgTemp = totalTemp / tempCount;
      float avgIllumination = totalIllumination / illuminationCount;

      if (!isnan(avgTemp) && !isnan(avgIllumination)) {
        sendDataToThingSpeak(avgTemp, avgIllumination, motionDetections, secureModeDuration, lightAutoModeDuration);
      } else {
        Serial.println("Invalid data. Skipping ThingSpeak update.");
      }
    } else {
      Serial.println("No valid data to send to ThingSpeak.");
    }
  }


  handleMotion();
  handleSerialInput();
  checkEmergencyButton();

  // Send daily report at midnight
  if (millis() - reportStartTime >= 86400000) {
    if (!dailyReportSent) {
      sendDailyReport();
      dailyReportSent = true;

      // Reset daily stats
      minTemp = 100.0;
      maxTemp = -100.0;
      totalTemp = 0.0;
      tempCount = 0;

      minIllumination = 1023;
      maxIllumination = 0;
      totalIllumination = 0;
      illuminationCount = 0;

      motionDetections = 0;
      secureModeDuration = 0;
      lightAutoModeDuration = 0;

      reportStartTime = millis();  // Reset timer
    }
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  // Keep trying to connect for 20 seconds
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi!");
  } else {
    Serial.println("Failed to connect to WiFi.");
  }
}


// Function to measure temperature and illumination
void measureTemperatureAndIllumination() {
  // Read temperature from TMP36 sensor
  int sensorValue = analogRead(tempSensorPin);
  float voltage = sensorValue * (5.0 / 1023.0);
  float temperatureC = (voltage - 0.5) * 100.0;

  // Print the temperature to the serial monitor
  Serial.println("===================================");
  Serial.println("Temperature and Illumination Report");
  Serial.println("===================================");
  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.write(176);  // ASCII code for the degree symbol
  Serial.println("C");

  // Control the motor and light bulb based on temperature
  if (temperatureC >= upperThreshold) {
    digitalWrite(motorRelayPin, HIGH);
    digitalWrite(lightBulbRelayPin, LOW);
    Serial.println("Fan ON, Heater OFF");
  } else if (temperatureC <= lowerThreshold) {
    digitalWrite(motorRelayPin, LOW);
    digitalWrite(lightBulbRelayPin, HIGH);
    Serial.println("Fan OFF, Heater ON");
  }

  // Read light level from the photoresistor
  int lightLevel = analogRead(lightSensorPin);
  int lightPercentage = (lightLevel * 100) / 1023;

  Serial.print("Illumination: ");
  Serial.print(lightPercentage);
  Serial.println("%");

  // Control the white LED based on the light sensor
  if (autoModeActive) {
    if (lightPercentage < 30) {
      digitalWrite(whiteLEDPin, HIGH);  // Turn on the White LED
      Serial.println("White LED ON");
    } else {
      digitalWrite(whiteLEDPin, LOW);   // Turn off the White LED
      Serial.println("White LED OFF");
    }
  }

  Serial.println("===================================");
  
  // Update temperature stats
  if (temperatureC < minTemp) minTemp = temperatureC;
  if (temperatureC > maxTemp) maxTemp = temperatureC;
  totalTemp += temperatureC;
  tempCount++;

  // Update illumination stats
  if (lightLevel < minIllumination) minIllumination = lightLevel;
  if (lightLevel > maxIllumination) maxIllumination = lightLevel;
  totalIllumination += lightLevel;
  illuminationCount++;
  
  if (measurementIndex < 144) {
    tempMeasurements[measurementIndex] = temperatureC;
    illumMeasurements[measurementIndex] = lightPercentage;
    measurementIndex++;
  }


}

// Function to handle motion detection and emergency mode
void handleMotion() {
  int motionDetected = digitalRead(motionPin);
  unsigned long currentMillis = millis();

  if (secureModeActive && motionDetected) {
    // Check if 10 seconds have passed since the last notification
    if (currentMillis - lightOnMillis >= lightOnDuration) {
      Serial.println("Motion Detected!");
      digitalWrite(whiteLEDPin, HIGH);  // Turn on the White LED
      lightOnMillis = currentMillis;    // Record the time when light was turned on

      // Send one email notification within a 10-second window
      if (!emergencyEmailSent) {
        sendEmail("Motion Alert", "Motion detected in your home! Secure mode is active.");
        emergencyEmailSent = true;
      }
    }
  }

  // Turn off the light after 10 seconds if no motion
  if (secureModeActive && (currentMillis - lightOnMillis >= lightOnDuration)) {
    digitalWrite(whiteLEDPin, LOW);
    Serial.println("No Motion Detected. White LED OFF (Secure Mode)");
    emergencyEmailSent = false;  // Reset email flag after 10 seconds
  }
}


// Function to check for emergency button press
void checkEmergencyButton() {
  int buttonState = digitalRead(buttonPin);

  if (buttonState == LOW && !emergencyModeActive) {
    // Activate Emergency Mode
    emergencyModeActive = true;
    autoModeActive = false;
    secureModeActive = true;

    // Turn on the red LED and turn off the green LED
    digitalWrite(redLEDPin, HIGH);
    digitalWrite(greenLEDPin, LOW);

    // Turn off all devices
    digitalWrite(motorRelayPin, LOW);
    digitalWrite(lightBulbRelayPin, LOW);
    digitalWrite(whiteLEDPin, LOW);

    Serial.println("EMERGENCY MODE ACTIVATED!");

    // Send emergency email only once
    if (!emergencyEmailSent) {
      sendEmail("EMERGENCY ALERT", "Emergency Mode Activated! The system is now in Emergency Mode.");
      Serial.println("Emergency email sent.");
      emergencyEmailSent = true;
    }
  }
}

void checkEmergencyStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // ThingSpeak URL to read the latest field value (replace with your channel ID and read API key)
    String url = "https://api.thingspeak.com/channels/YOUR_CHANNEL_ID/fields/6/last.json?api_key=YOUR_READ_API_KEY";
    http.begin(url);

    int httpCode = http.GET();  // Make the GET request
    if (httpCode > 0) {
      String payload = http.getString();  // Get the response payload
      Serial.println("Checking ThingSpeak for Emergency Off Command...");
      
      // Check if the response contains the value "0"
      if (payload.indexOf("\"field6\":\"0\"") != -1) {
        emergencyModeActive = false;
        digitalWrite(redLEDPin, LOW);  // Turn off the red LED
        digitalWrite(greenLEDPin, HIGH);  // Turn on the green LED
        Serial.println("Emergency Mode Deactivated via ThingSpeak!");
      }
    } else {
      Serial.println("Error fetching data from ThingSpeak.");
    }
    http.end();  // Close the connection
  } else {
    Serial.println("WiFi not connected. Cannot check ThingSpeak.");
  }
}



// Function to handle serial input from the user
void handleSerialInput() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();  // Remove any extra whitespace
    command.toUpperCase();  // Make the command case-insensitive

    Serial.print("Received Command: ");
    Serial.println(command);

    if (command.equalsIgnoreCase("ON")) {
      autoModeActive = false;
      secureModeActive = false;
      digitalWrite(whiteLEDPin, HIGH);
      Serial.println("White LED ON (Manual Mode)");
    } else if (command.equalsIgnoreCase("OFF")) {
      autoModeActive = false;
      secureModeActive = false;
      digitalWrite(whiteLEDPin, LOW);
      Serial.println("White LED OFF (Manual Mode)");
    } else if (command.equalsIgnoreCase("AUTO")) {
      autoModeActive = true;
      secureModeActive = false;
      Serial.println("Home Light Auto Mode is ACTIVE.");
    } else if (command.equalsIgnoreCase("SECURE")) {
      secureModeActive = true;
      autoModeActive = false;
      Serial.println("Home Secure Mode is ACTIVE.");
    } else {
      Serial.println("Invalid Command. Use 'ON', 'OFF', 'AUTO', 'SECURE', or 'EMERGENCY_OFF'.");
    }
  }
}

// Function to generate graphs from stored measurements
void generateGraphs() {
  Serial.println("Generating Temperature Graph...");
  for (int i = 0; i < measurementIndex; i++) {
    Serial.print("[ ");
    Serial.print(i);
    Serial.print(" ] Temp: ");
    Serial.print(tempMeasurements[i]);
    Serial.println(" C");
  }

  Serial.println("Generating Illumination Graph...");
  for (int i = 0; i < measurementIndex; i++) {
    Serial.print("[ ");
    Serial.print(i);
    Serial.print(" ] Illumination: ");
    Serial.print(illumMeasurements[i]);
    Serial.println(" %");
  }

  Serial.println("Generating Motion Detection Graph...");
  for (int i = 0; i < measurementIndex; i++) {
    Serial.print("[ ");
    Serial.print(i);
    Serial.print(" ] Motion: ");
    Serial.println(motionMeasurements[i]);
  }
}

// Function to send daily report via email
void sendDailyReport() {
  String report = "Daily Smart Home Report\n";
  report += "-----------------------------------\n";
  report += "Minimal Temperature: " + String(minTemp) + " °C\n";
  report += "Maximal Temperature: " + String(maxTemp) + " °C\n";
  report += "Average Temperature: " + String(totalTemp / tempCount) + " °C\n";
  report += "Minimal Illumination: " + String(minIllumination) + "\n";
  report += "Maximal Illumination: " + String(maxIllumination) + "\n";
  report += "Average Illumination: " + String(totalIllumination / illuminationCount) + "\n";
  report += "Total Motion Detections: " + String(motionDetections) + "\n";
  report += "Secure Mode Duration: " + String(secureModeDuration / 1000) + " seconds\n";
  report += "Light Auto Mode Duration: " + String(lightAutoModeDuration / 1000) + " seconds\n";

  Serial.println("Sending daily report...");
  sendEmail("Daily Report", report);
  
  dailyReportSent = true;
  Serial.println("Daily report sent and stats reset.");

  // Generate graphs for daily report
  generateGraphs();
}

// Function to send an email
void sendEmail(const String& subject, const String& message) {
  SMTP_Message msg;

  msg.sender.name = "Smart Home";
  msg.sender.email = smtpUser;
  msg.subject = subject;
  msg.addRecipient("Luka", recipient);

  msg.text.content = message;

  if (!MailClient.sendMail(smtp, msg)) {
    Serial.println("Error sending email, " + MailClient.smtpErrorReason());
  } else {
    Serial.println("Email sent successfully!");
  }
}

// Function to check email for commands
void checkEmail() {
  if (MailClient.readMail(smtp)) {
    Serial.println("Checking for new emails...");

    for (size_t i = 0; i < smtp.availableMessages(); i++) {
      SMTP_Message message = smtp.getMessage(i);

      String subject = message.subject;

      if (subject == "Start Auto Temp Control") {
        autoModeActive = true;
        Serial.println("Automatic Temperature Control Activated!");
        sendEmail("Auto Temp Control", "Automatic Temperature Control has been turned ON.");
      }
      else if (subject == "Stop Auto Temp Control") {
        autoModeActive = false;
        Serial.println("Automatic Temperature Control Deactivated!");
        sendEmail("Auto Temp Control", "Automatic Temperature Control has been turned OFF.");
      }
      else if (subject == "Activate Secure Mode") {
        secureModeActive = true;
        Serial.println("Secure Mode Activated!");
        sendEmail("Secure Mode", "Secure Mode has been turned ON.");
      }
      else if (subject == "Deactivate Secure Mode") {
        secureModeActive = false;
        Serial.println("Secure Mode Deactivated!");
        sendEmail("Secure Mode", "Secure Mode has been turned OFF.");
      }
      else {
        Serial.println("Unknown command received via email.");
      }
    }
  } else {
    Serial.println("No new emails.");
  }
}


// Callback function to handle email status
void smtpCallback(SMTP_Status status) {
  if (status.success()) {
    Serial.println("Email sent successfully!");
  } else {
    Serial.println("Email sending failed.");
  }
}

void sendDataToThingSpeak(float temperature, int illumination, int motion, unsigned long secureDuration, unsigned long autoModeDuration) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Create the ThingSpeak URL
    String url = thingSpeakServer;
    url += "?api_key=" + String(apiKey);
    url += "&field1=" + String(temperature);
    url += "&field2=" + String(illumination);
    url += "&field3=" + String(motion);
    url += "&field4=" + String(secureDuration / 1000);  // Convert to seconds
    url += "&field5=" + String(autoModeDuration / 1000);  // Convert to seconds

    http.begin(url);  // Specify the URL
    int httpCode = http.GET();  // Send the request

    if (httpCode == 200) {
      Serial.println("Data successfully sent to ThingSpeak!");
    } else {
      Serial.print("Error sending data to ThingSpeak. HTTP code: ");
      Serial.println(httpCode);
    }


    http.end();  // Close the connection
  } else {
    Serial.println("WiFi not connected. Cannot send data to ThingSpeak.");
  }
}