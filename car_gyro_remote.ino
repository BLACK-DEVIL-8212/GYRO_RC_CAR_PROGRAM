#include<EEPROM.h>
#include<gyro.h>
#include<SPI.h>
#include<nRF24L01.h>
#include<RF24.h>

// Gyro sensor pins (MPU6050)
#define GYRO_SDA A4
#define GYRO_SCL A5

// nRF24L01 pins
#define CE_PIN 9
#define CSN_PIN 10

// Control parameters
#define THRESHOLD 15      // Gyro sensitivity threshold
#define MAX_SPEED 255     // Maximum motor speed
#define DEADZONE 5        // Joystick deadzone
#define CALIBRATION_SAMPLES 100

// Create radio object
RF24 radio(CE_PIN, CSN_PIN);

// Data structure to send
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

ControlData data;

// Gyro calibration values
int gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize gyro
  initGyro();
  calibrateGyro();
  
  // Initialize radio
  radio.begin();
  radio.openWritingPipe(0xA8A8F0F0E1LL);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.stopListening();
  
  // Initialize control data
  memset(&data, 0, sizeof(data));
  
  Serial.println("Car Gyro Remote Ready!");
  Serial.println("Tilt remote forward/backward for throttle");
  Serial.println("Tilt left/right for steering");
  Serial.println("Press buttons for brake/lights");
}

void loop() {
  // Read gyro values
  int gyroX = readGyroX() - gyroOffsetX;
  int gyroY = readGyroY() - gyroOffsetY;
  int gyroZ = readGyroZ() - gyroOffsetZ;
  
  // Calculate throttle (forward/backward tilt)
  data.throttle = mapThrottle(gyroX);
  
  // Calculate steering (left/right tilt)
  data.steering = mapSteering(gyroY);
  
  // Read buttons (if connected)
  readButtons();
  
  // Calculate checksum for data integrity
  data.checksum = calculateChecksum();
  
  // Send data to car
  sendData();
  
  // Debug output
  printDebug(gyroX, gyroY, gyroZ);
  
  delay(20); // 50Hz update rate
}

void initGyro() {
  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  
  // Configure gyro scale (250 deg/s)
  Wire.beginTransmission(0x68);
  Wire.write(0x1B);
  Wire.write(0x00);
  Wire.endTransmission(true);
}

void calibrateGyro() {
  Serial.println("Calibrating gyro... Keep remote still");
  delay(1000);
  
  long sumX = 0, sumY = 0, sumZ = 0;
  
  for(int i = 0; i < CALIBRATION_SAMPLES; i++) {
    sumX += readGyroX();
    sumY += readGyroY();
    sumZ += readGyroZ();
    delay(5);
  }
  
  gyroOffsetX = sumX / CALIBRATION_SUMES;
  gyroOffsetY = sumY / CALIBRATION_SAMPLES;
  gyroOffsetZ = sumZ / CALIBRATION_SAMPLES;
  
  Serial.println("Calibration complete!");
  Serial.print("Offsets: X=");
  Serial.print(gyroOffsetX);
  Serial.print(" Y=");
  Serial.print(gyroOffsetY);
  Serial.print(" Z=");
  Serial.println(gyroOffsetZ);
}

int readGyroX() {
  Wire.beginTransmission(0x68);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 2, true);
  return (Wire.read() << 8 | Wire.read());
}

int readGyroY() {
  Wire.beginTransmission(0x68);
  Wire.write(0x45);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 2, true);
  return (Wire.read() << 8 | Wire.read());
}

int readGyroZ() {
  Wire.beginTransmission(0x68);
  Wire.write(0x47);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 2, true);
  return (Wire.read() << 8 | Wire.read());
}

int mapThrottle(int gyroValue) {
  // Forward tilt = positive throttle, Backward tilt = negative throttle (brake)
  gyroValue = abs(gyroValue) < THRESHOLD ? 0 : gyroValue;
  
  if(gyroValue > 0) {
    // Forward
    data.brake = false;
    return map(constrain(gyroValue, 0, 250), 0, 250, 0, MAX_SPEED);
  } else if(gyroValue < 0) {
    // Backward (brake/reverse)
    data.brake = true;
    return map(constrain(abs(gyroValue), 0, 250), 0, 250, 0, MAX_SPEED);
  }
  
  return 0;
}

int mapSteering(int gyroValue) {
  // Left tilt = negative steering, Right tilt = positive steering
  gyroValue = abs(gyroValue) < THRESHOLD ? 0 : gyroValue;
  return constrain(gyroValue, -100, 100);
}

void readButtons() {
  // Digital pins for buttons (modify according to your wiring)
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  
  // Button 1: Lights toggle
  static bool lastLightState = HIGH;
  bool lightState = digitalRead(2);
  if(lightState == LOW && lastLightState == HIGH) {
    data.lights = !data.lights;
    Serial.print("Lights: ");
    Serial.println(data.lights ? "ON" : "OFF");
  }
  lastLightState = lightState;
  
  // Button 2: Emergency brake
  bool brakeState = digitalRead(3);
  if(brakeState == LOW) {
    data.brake = true;
    data.throttle = 0;
  }
}

uint8_t calculateChecksum() {
  uint8_t sum = 0;
  sum ^= data.xAxis;
  sum ^= data.yAxis;
  sum ^= data.zAxis;
  sum ^= data.throttle;
  sum ^= data.steering;
  sum ^= data.brake;
  sum ^= data.lights;
  return sum;
}

void sendData() {
  radio.write(&data, sizeof(data));
  
  // Verify transmission (optional)
  static unsigned long lastAck = 0;
  if(millis() - lastAck > 1000) {
    Serial.println("Data sent");
    lastAck = millis();
  }
}

void printDebug(int gyroX, int gyroY, int gyroZ) {
  static unsigned long lastPrint = 0;
  
  if(millis() - lastPrint > 100) {
    Serial.print("Gyro: X=");
    Serial.print(gyroX);
    Serial.print(" Y=");
    Serial.print(gyroY);
    Serial.print(" Z=");
    Serial.print(gyroZ);
    Serial.print(" | Throttle=");
    Serial.print(data.throttle);
    Serial.print(" Steering=");
    Serial.print(data.steering);
    Serial.print(" Brake=");
    Serial.print(data.brake);
    Serial.print(" Lights=");
    Serial.println(data.lights);
    lastPrint = millis();
  }
}
