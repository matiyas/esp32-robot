# Hardware Setup Guide

## Components

### ESP32-CAM Module (AI-Thinker)

The ESP32-CAM is the main controller. It includes:
- ESP32 dual-core processor
- OV2640 camera module
- Built-in 4MB PSRAM for frame buffers
- WiFi connectivity
- Flash LED on GPIO 4

### DRV8833 Motor Driver

Dual H-bridge motor driver for controlling two DC motors with direct PWM control.

**Specifications:**
- Operating voltage: 2.7V - 10.8V
- Output current: 1.5A per channel
- PWM capable on IN1/IN2 pins

**Truth Table (PWM on IN1/IN2):**

| IN1 | IN2 | Mode |
|-----|-----|------|
| LOW | LOW | Coast (freewheeling) |
| PWM | LOW | Forward (speed control) |
| LOW | PWM | Reverse (speed control) |
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
ESP32-CAM             DRV8833
---------             -------
GPIO12 (PWM) -------> AIN1
GPIO13 (PWM) -------> AIN2
GPIO14 (PWM) -------> BIN1
GPIO15 (PWM) -------> BIN2
                      nSLEEP --> VCC (always enabled)

ESP32-CAM             SG90 Servo
---------             ----------
GPIO2 (PWM) --------> Signal (orange)
GND -----------------> GND (brown)
5V ------------------> VCC (red)

ESP32-CAM             LED
---------             ---
GPIO4 (built-in) ---> Flash LED
```

## Power Considerations

1. **ESP32-CAM**: 5V via USB or external power (min 2A recommended)
2. **Motors**: Connect to DRV8833 VMOT (separate from logic, 2.7V-10.8V)
3. **Servo**: 5V (can share with ESP32)

**Important:**
- Use adequate decoupling capacitors near the motor driver
- Ensure stable 5V supply - ESP32-CAM with camera streaming draws significant current

## GPIO Pin Configuration

All GPIO pins are configurable via `idf.py menuconfig`:

```
Robot Controller Configuration ->
  Motor Control ->
    Motor Left IN1: 12
    Motor Left IN2: 13
    Motor Right IN1: 14
    Motor Right IN2: 15
    PWM Frequency: 1000 Hz
  Servo Control ->
    Servo GPIO: 2
    Min Pulse: 500 µs
    Max Pulse: 2400 µs
    Step Angle: 10°
    Smooth Step: 2°
    Smooth Delay: 15 ms
  Camera Settings ->
    Frame Size: 6 (QVGA 320x240)
    JPEG Quality: 4 (best)
    Frame Buffer Count: 1
```

## Camera Pins (Fixed - AI-Thinker ESP32-CAM)

| Signal | GPIO |
|--------|------|
| PWDN | 32 |
| RESET | -1 (not used) |
| XCLK | 0 |
| SIOD (SDA) | 26 |
| SIOC (SCL) | 27 |
| D7 | 35 |
| D6 | 34 |
| D5 | 39 |
| D4 | 36 |
| D3 | 21 |
| D2 | 19 |
| D1 | 18 |
| D0 | 5 |
| VSYNC | 25 |
| HREF | 23 |
| PCLK | 22 |

**Camera Optimization:** XCLK is set to 8MHz (instead of default 20MHz) to avoid WiFi interference.

## Troubleshooting

### Motors not working
1. Check DRV8833 power connections
2. Verify nSLEEP is tied to VCC
3. Verify GPIO pin assignments
4. Check PWM output with oscilloscope

### Servo jittering
1. Add decoupling capacitor (100µF) near servo
2. Check power supply stability
3. Ensure no GPIO conflicts

### Camera not streaming
1. Verify camera module ribbon cable is properly seated
2. Check PSRAM is detected in boot log
3. Reduce frame size or quality if needed
4. Check for WiFi interference (XCLK should be 8MHz)

### Camera stream laggy
1. Use QVGA (320x240) resolution
2. Use 1 frame buffer to prevent queue buildup
3. Ensure XCLK is 8MHz to avoid WiFi interference
4. Check WiFi signal strength
