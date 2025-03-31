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

// Home Light Auto Mode flag
bool autoModeActive = true;
bool secureModeActive = false;
bool emergencyModeActive = false;

// Time variables
unsigned long previousTempMillis = 0;  // Timer for temperature and illumination
unsigned long lightOnMillis = 0;        // Timer to turn off light after motion
const unsigned long tempInterval = 100000;  // 10 minutes
const unsigned long lightOnDuration = 10000; // 10 seconds for light after motion

void setup() {
  // Initialize relay pins as outputs
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

  // Initial State
  digitalWrite(redLEDPin, LOW);    // Red LED OFF
  digitalWrite(greenLEDPin, HIGH); // Green LED ON
  digitalWrite(whiteLEDPin, LOW);  // White LED OFF

  // Initial message
  Serial.println("Smart Home System Initialized.");
}

void loop() {
  if (emergencyModeActive) {
    handleMotion();
    return;  // Skip the rest of the loop
  }
  
  unsigned long currentMillis = millis();

  // Check temperature and illumination every 10 seconds
  if (currentMillis - previousTempMillis >= tempInterval) {
    previousTempMillis = currentMillis;
    measureTemperatureAndIllumination();
  }

  
  handleMotion();
  
  handleSerialInput();
  
  checkEmergencyButton();
  
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
}

// Function to handle motion detection and emergency mode
void handleMotion() {
  int motionDetected = digitalRead(motionPin);
  unsigned long currentMillis = millis();

  if (secureModeActive && motionDetected) {
    Serial.println("Motion Detected!");
    digitalWrite(whiteLEDPin, HIGH);  // Turn on the White LED
    lightOnMillis = currentMillis;    // Record the time when light was turned on
  }

  // Turn off the light after 10 seconds if no motion
  if (secureModeActive && (currentMillis - lightOnMillis >= lightOnDuration)) {
    digitalWrite(whiteLEDPin, LOW);
    Serial.println("No Motion Detected. White LED OFF (Secure Mode)");
  }
}

// Function to check for emergency button press
void checkEmergencyButton() {
  int buttonState = digitalRead(buttonPin);

  if (buttonState == LOW) {
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
  }
}

// Function to handle serial input from the user
void handleSerialInput() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();  // Remove any extra whitespace

    Serial.print("Received Command: ");
    Serial.println(command);

    if (command.equalsIgnoreCase("ON")) {
      autoModeActive = false;
      secureModeActive = false;
      digitalWrite(whiteLEDPin, HIGH);
      Serial.println("White LED ON (Manual Mode)");
      Serial.println("Home Light Auto Mode and Secure Mode are DEACTIVATED.");
    } else if (command.equalsIgnoreCase("OFF")) {
      autoModeActive = false;
      secureModeActive = false;
      digitalWrite(whiteLEDPin, LOW);
      Serial.println("White LED OFF (Manual Mode)");
      Serial.println("Home Light Auto Mode and Secure Mode are DEACTIVATED.");
    } else if (command.equalsIgnoreCase("AUTO")) {
      autoModeActive = true;
      secureModeActive = false;
      Serial.println("Home Light Auto Mode is ACTIVE.");
    } else if (command.equalsIgnoreCase("SECURE")) {
      secureModeActive = true;
      autoModeActive = false;
      Serial.println("Home Secure Mode is ACTIVE.");
    } else {
      Serial.println("Invalid Command. Use 'ON', 'OFF', 'AUTO', or 'SECURE'.");
    }
  }
}