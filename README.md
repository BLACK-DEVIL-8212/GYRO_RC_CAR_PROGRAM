# Gyro-Controlled RC Car System

A wireless remote control car system using gyroscope (MPU6050) for intuitive tilt-based control and nRF24L01 for communication.

## 📋 Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Wiring Diagrams](#wiring-diagrams)
- [Software Setup](#software-setup)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [Technical Details](#technical-details)
- [Safety Notes](#safety-notes)
- [License](#license)

## 🎮 Overview

This project consists of two Arduino programs:
1. **Remote Controller (`car_gyro_remote.ino`)** - Uses MPU6050 gyroscope to detect hand movements and sends control data via nRF24L01
2. **RC Car Receiver (`rc_car.ino`)** - Receives wireless commands and controls motors accordingly

The car responds to natural hand movements:
- Tilt forward/backward → Accelerate/Brake
- Tilt left/right → Steering control
- Buttons for lights and emergency brake

## ✨ Features

### Remote Controller
- **Intuitive Gyro Control** - Natural tilt-based driving
- **Auto-calibration** - Gyro offset calibration on startup
- **Emergency Brake** - Instant stop button
- **LED Light Control** - Toggle headlights
- **Data Integrity** - Checksum verification for reliable communication
- **Real-time Feedback** - Serial monitor debug output

### RC Car Receiver
- **Differential Steering** - Tank-like turning mechanism
- **Connection Loss Protection** - Auto-stop if signal lost
- **Adjustable Speed Limit** - EEPROM-stored max speed
- **Steering Trim** - Fine-tune straight-line tracking
- **Motor Test Mode** - Diagnostic testing
- **Serial Command Interface** - Easy configuration without reflashing

## 🔧 Hardware Requirements

### Remote Controller
| Component | Quantity | Notes |
|-----------|----------|-------|
| Arduino Uno/Nano | 1 | Any 5V Arduino |
| MPU6050 Gyroscope | 1 | 3-axis gyro + accelerometer |
| nRF24L01 Module | 1 | 2.4GHz wireless transceiver |
| Push Buttons | 2 | For lights and emergency brake |
| 10kΩ Resistors | 2 | Pull-up for buttons (optional) |
| Breadboard & Jumper Wires | As needed | - |

### RC Car
| Component | Quantity | Notes |
|-----------|----------|-------|
| Arduino Uno/Nano | 1 | Any 5V Arduino |
| nRF24L01 Module | 1 | Same frequency as remote |
| L298N/L293D Motor Driver | 1 | For motor control |
| DC Motors | 2 | For left/right drive |
| LED (for lights) | 1 | Optional |
| 220Ω Resistor | 1 | For LED |
| Battery Pack | 1 | 7.4V-12V for motors |
| Jumper Wires | As needed | - |
| RC Car Chassis | 1 | With wheels |

### Required Libraries
Install these libraries via Arduino Library Manager:
- `RF24` by TMRh20
- `MPU6050` by Electronic Cats (or use Wire library directly)
- `SPI` (built-in)
- `EEPROM` (built-in)
- `Wire` (built-in)

## 🔌 Wiring Diagrams

### Remote Controller Wiring
