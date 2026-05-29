#include <EEPROM.h>
#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ctype.h>

// Motor control pins
const int leftmotorpin1 = 2;   // Left motor forward
const int leftmotorpin2 = 3;   // Left motor backward
const int rightmotorpin3 = 4;  // Right motor forward
const int rightmotorpin4 = 5;  // Right motor backward

// LED pin for lights
const int lightPin = 6;

// nRF24L01 pins
const int CE_PIN = 9;
const int CSN_PIN = 10;

// Control data structure (must match remote)
struct ControlData {
  int xAxis;
  int yAxis;
  int zAxis;
  int throttle;
  int steering;
  bool brake;
  bool lights;
  uint8_t checksum;
};

ControlData receivedData;
RF24 radio(CE_PIN, CSN_PIN);

// EEPROM addresses
const int EEPROM_ADDR_SPEED_LIMIT = 0;
const int EEPROM_ADDR_STEERING_TRIM = 1;

// Variables
int maxSpeed = 255;
int steeringTrim = 0;
unsigned long lastReceiveTime = 0;
bool connectionLost = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize motor pins
  pinMode(leftmotorpin1, OUTPUT);
  pinMode(leftmotorpin2, OUTPUT);
  pinMode(rightmotorpin3, OUTPUT);
  pinMode(rightmotorpin4, OUTPUT);
  pinMode(lightPin, OUTPUT);
  
  // Stop all motors initially
  stopMotors();
  
  // Load settings from EEPROM
  loadSettings();
  
  // Initialize radio
  initRadio();
  
  Serial.println("RC Car Receiver Ready");
  Serial.println("Waiting for remote connection...");
}

void loop() {
  // Check for incoming data
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));
    lastReceiveTime = millis();
    connectionLost = false;
    
    // Verify checksum
    if (verifyChecksum()) {
      processControls();
    } else {
      Serial.println("Checksum error - data corrupted");
    }
  }
  
  // Check for connection timeout
  if (millis() - lastReceiveTime > 500 && !connectionLost) {
    connectionLost = true;
    stopMotors();
    Serial.println("Connection lost! Stopping car.");
  }
  
  // Optional: Manual control via Serial for testing
  if (Serial.available() > 0) {
    handleSerialCommands();
  }
  
  delay(10); // Small delay for stability
}

void initRadio() {
  radio.begin();
  radio.openReadingPipe(0, 0xA8A8F0F0E1LL);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.startListening();
  
  Serial.println("Radio initialized");
}

bool verifyChecksum() {
  uint8_t sum = 0;
  sum ^= receivedData.xAxis;
  sum ^= receivedData.yAxis;
  sum ^= receivedData.zAxis;
  sum ^= receivedData.throttle;
  sum ^= receivedData.steering;
  sum ^= receivedData.brake;
  sum ^= receivedData.lights;
  return (sum == receivedData.checksum);
}

void processControls() {
  // Process throttle with brake
  if (receivedData.brake) {
    applyBrake();
  } else {
    int throttleValue = receivedData.throttle;
    
    // Apply speed limit
    throttleValue = constrain(throttleValue, 0, maxSpeed);
    
    // Process steering
    int steeringValue = receivedData.steering + steeringTrim;
    steeringValue = constrain(steeringValue, -100, 100);
    
    // Differential steering for tank-like drive
    if (steeringValue == 0) {
      // Straight line
      moveForward(throttleValue);
    } else if (steeringValue > 0) {
      // Turn right
      turnRight(throttleValue, steeringValue);
    } else {
      // Turn left
      turnLeft(throttleValue, abs(steeringValue));
    }
  }
  
  // Process lights
  digitalWrite(lightPin, receivedData.lights ? HIGH : LOW);
  
  // Debug output
  printControlStatus();
}

void moveForward(int speed) {
  analogWrite(leftmotorpin1, speed);
  analogWrite(leftmotorpin2, 0);
  analogWrite(rightmotorpin3, speed);
  analogWrite(rightmotorpin4, 0);
}

void moveBackward(int speed) {
  analogWrite(leftmotorpin1, 0);
  analogWrite(leftmotorpin2, speed);
  analogWrite(rightmotorpin3, 0);
  analogWrite(rightmotorpin4, speed);
}

void turnRight(int speed, int steering) {
  // Reduce speed on right side for right turn
  int leftSpeed = speed;
  int rightSpeed = map(steering, 0, 100, speed, 0);
  
  analogWrite(leftmotorpin1, leftSpeed);
  analogWrite(leftmotorpin2, 0);
  analogWrite(rightmotorpin3, rightSpeed);
  analogWrite(rightmotorpin4, 0);
}

void turnLeft(int speed, int steering) {
  // Reduce speed on left side for left turn
  int leftSpeed = map(steering, 0, 100, speed, 0);
  int rightSpeed = speed;
  
  analogWrite(leftmotorpin1, leftSpeed);
  analogWrite(leftmotorpin2, 0);
  analogWrite(rightmotorpin3, rightSpeed);
  analogWrite(rightmotorpin4, 0);
}

void applyBrake() {
  // Emergency brake - stop all motors
  stopMotors();
  
  // Optional: Apply reverse thrust for braking effect
  // This creates a "braking" feel
  for(int i = 0; i < 3; i++) {
    analogWrite(leftmotorpin1, 0);
    analogWrite(leftmotorpin2, 100);
    analogWrite(rightmotorpin3, 0);
    analogWrite(rightmotorpin4, 100);
    delay(20);
    stopMotors();
    delay(20);
  }
}

void stopMotors() {
  analogWrite(leftmotorpin1, 0);
  analogWrite(leftmotorpin2, 0);
  analogWrite(rightmotorpin3, 0);
  analogWrite(rightmotorpin4, 0);
}

void loadSettings() {
  // Load max speed limit from EEPROM
  int savedSpeed = EEPROM.read(EEPROM_ADDR_SPEED_LIMIT);
  if (savedSpeed >= 0 && savedSpeed <= 255) {
    maxSpeed = savedSpeed;
  } else {
    maxSpeed = 255; // Default max speed
  }
  
  // Load steering trim
  int savedTrim = EEPROM.read(EEPROM_ADDR_STEERING_TRIM);
  if (savedTrim >= -50 && savedTrim <= 50) {
    steeringTrim = savedTrim;
  } else {
    steeringTrim = 0; // Default no trim
  }
  
  Serial.print("Loaded settings - Max Speed: ");
  Serial.print(maxSpeed);
  Serial.print(", Steering Trim: ");
  Serial.println(steeringTrim);
}

void saveSettings() {
  EEPROM.write(EEPROM_ADDR_SPEED_LIMIT, maxSpeed);
  EEPROM.write(EEPROM_ADDR_STEERING_TRIM, steeringTrim);
  Serial.println("Settings saved to EEPROM");
}

void handleSerialCommands() {
  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toLowerCase();
  
  if (command.startsWith("speed ")) {
    int newSpeed = command.substring(6).toInt();
    if (newSpeed >= 0 && newSpeed <= 255) {
      maxSpeed = newSpeed;
      saveSettings();
      Serial.print("Max speed set to: ");
      Serial.println(maxSpeed);
    } else {
      Serial.println("Invalid speed. Use 0-255");
    }
  }
  else if (command.startsWith("trim ")) {
    int newTrim = command.substring(5).toInt();
    if (newTrim >= -50 && newTrim <= 50) {
      steeringTrim = newTrim;
      saveSettings();
      Serial.print("Steering trim set to: ");
      Serial.println(steeringTrim);
    } else {
      Serial.println("Invalid trim. Use -50 to 50");
    }
  }
  else if (command == "stop") {
    stopMotors();
    Serial.println("Emergency stop");
  }
  else if (command == "test") {
    testMotors();
  }
  else if (command == "status") {
    printStatus();
  }
  else if (command == "help") {
    printHelp();
  }
}

void testMotors() {
  Serial.println("Testing motors...");
  
  Serial.println("Forward");
  moveForward(150);
  delay(1000);
  
  Serial.println("Stop");
  stopMotors();
  delay(500);
  
  Serial.println("Backward");
  moveBackward(150);
  delay(1000);
  
  Serial.println("Stop");
  stopMotors();
  delay(500);
  
  Serial.println("Turn Right");
  turnRight(150, 50);
  delay(1000);
  
  Serial.println("Stop");
  stopMotors();
  delay(500);
  
  Serial.println("Turn Left");
  turnLeft(150, 50);
  delay(1000);
  
  Serial.println("Stop");
  stopMotors();
  
  Serial.println("Motor test complete");
}

void printControlStatus() {
  static unsigned long lastPrint = 0;
  
  if (millis() - lastPrint > 200) {
    Serial.print("Throttle: ");
    Serial.print(receivedData.throttle);
    Serial.print(" | Steering: ");
    Serial.print(receivedData.steering);
    Serial.print(" | Brake: ");
    Serial.print(receivedData.brake ? "ON" : "OFF");
    Serial.print(" | Lights: ");
    Serial.print(receivedData.lights ? "ON" : "OFF");
    Serial.print(" | Connected: ");
    Serial.println(!connectionLost ? "YES" : "NO");
    lastPrint = millis();
  }
}

void printStatus() {
  Serial.println("=== RC Car Status ===");
  Serial.print("Connection: ");
  Serial.println(connectionLost ? "LOST" : "ACTIVE");
  Serial.print("Max Speed: ");
  Serial.println(maxSpeed);
  Serial.print("Steering Trim: ");
  Serial.println(steeringTrim);
  Serial.print("Last Receive: ");
  Serial.print(millis() - lastReceiveTime);
  Serial.println("ms ago");
  Serial.println("====================");
}

void printHelp() {
  Serial.println("=== Available Commands ===");
  Serial.println("speed <0-255> - Set max speed");
  Serial.println("trim <-50-50>  - Set steering trim");
  Serial.println("stop           - Emergency stop");
  Serial.println("test           - Test all motors");
  Serial.println("status         - Show car status");
  Serial.println("help           - Show this help");
  Serial.println("========================");
}
