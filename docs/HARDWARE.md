# Hardware Setup Guide

## Components

### ESP32-S3-CAM Module

The ESP32-S3-CAM is the main controller. It includes:
- ESP32-S3 dual-core processor
- OV2640 camera module
- Built-in PSRAM for frame buffers
- WiFi connectivity

### DRV8833 Motor Driver

Dual H-bridge motor driver for controlling two DC motors.

**Specifications:**
- Operating voltage: 2.7V - 10.8V
- Output current: 1.5A per channel
- PWM capable

**Truth Table:**

| IN1 | IN2 | Mode |
|-----|-----|------|
| LOW | LOW | Coast (freewheeling) |
| HIGH | LOW | Forward |
| LOW | HIGH | Reverse |
| HIGH | HIGH | Brake |

### SG90 Servo Motor

Micro servo for camera turret rotation.

**Specifications:**
- Operating voltage: 4.8V - 6V
- Rotation angle: 0° - 180°
- PWM frequency: 50Hz
- Pulse width: 500µs - 2400µs

## Wiring Diagram

```
ESP32-S3-CAM          DRV8833
-----------          -------
GPIO12 (IN1) -----> AIN1
GPIO13 (IN2) -----> AIN2
GPIO14 (IN1) -----> BIN1
GPIO15 (IN2) -----> BIN2
GPIO2  (EEP) -----> nSLEEP (enable)

ESP32-S3-CAM          SG90
-----------          ----
GPIO4 (PWM) -------> Signal (orange)
GND ----------------> GND (brown)
5V -----------------> VCC (red)
```

## Power Considerations

1. **ESP32-S3-CAM**: 5V via USB or external power
2. **Motors**: Connect to DRV8833 VMOT (separate from logic)
3. **Servo**: 5V (can share with ESP32)

**Important:** Use adequate decoupling capacitors near the motor driver.

## GPIO Pin Configuration

All GPIO pins are configurable via `idf.py menuconfig`:

```
Robot Configuration ->
  Motor Configuration ->
    Motor Left IN1: 12
    Motor Left IN2: 13
    Motor Right IN1: 14
    Motor Right IN2: 15
    Motors Enable: 2
    PWM Frequency: 1000 Hz
  Servo Configuration ->
    Servo GPIO: 4
    Min Pulse: 500 µs
    Max Pulse: 2400 µs
```

## Camera Pins (Fixed)

The ESP32-S3-CAM has fixed camera pin assignments:

| Signal | GPIO |
|--------|------|
| XCLK | 10 |
| SIOD | 40 |
| SIOC | 39 |
| D7 | 48 |
| D6 | 11 |
| D5 | 12 |
| D4 | 14 |
| D3 | 16 |
| D2 | 18 |
| D1 | 17 |
| D0 | 15 |
| VSYNC | 38 |
| HREF | 47 |
| PCLK | 13 |

**Note:** GPIO 12, 13, 14, 15 are shared with motor control. Ensure camera uses different pins or disable camera when motors are active.

## Troubleshooting

### Motors not working
1. Check DRV8833 power connections
2. Verify GPIO pin assignments
3. Check PWM output with oscilloscope

### Servo jittering
1. Add decoupling capacitor (100µF) near servo
2. Check power supply stability
3. Reduce PWM frequency if needed

### Camera not streaming
1. Verify camera module is properly seated
2. Check PSRAM is detected
3. Reduce frame size or quality
