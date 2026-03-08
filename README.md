# ESP32-CAM Robot Controller

A robot tank controller running on ESP32-CAM (AI-Thinker) with REST API, camera streaming, and web UI.

## Features

- **Motor Control**: DRV8833 dual H-bridge driver with PWM soft-start ramp
- **Servo Control**: SG90 micro servo for camera turret with smooth interpolation
- **Camera Streaming**: MJPEG stream via built-in OV2640 camera
- **REST API**: Full API for robot control (OpenAPI documented)
- **Web UI**: Mobile-friendly control interface
- **Safety Features**: Watchdog timer, movement timeout, emergency stop

## Hardware Requirements

- ESP32-CAM module (AI-Thinker)
- DRV8833 motor driver
- SG90 servo motor
- DC motors (2x)
- Power supply (5V for ESP32, motor voltage for DRV8833)

## GPIO Pinout

| Function | Default GPIO | Notes |
|----------|--------------|-------|
| Motor Left IN1 | 12 | SD card D2 (unused) |
| Motor Left IN2 | 13 | SD card D3 (unused) |
| Motor Right IN1 | 14 | SD card CLK (unused) |
| Motor Right IN2 | 15 | SD card CMD (unused) |
| Motors Enable (PWM) | 2 | SD card D0 (unused) |
| Servo Signal | 16 | U2RXD |

**Note**: SD card functionality is sacrificed to free GPIO pins for motor/servo control.

### Camera Pins (Fixed - AI-Thinker)

| Function | GPIO |
|----------|------|
| PWDN | 32 |
| XCLK | 0 |
| SDA | 26 |
| SCL | 27 |
| D0-D7 | 5,18,19,21,36,39,34,35 |
| VSYNC | 25 |
| HREF | 23 |
| PCLK | 22 |

## Building

### Prerequisites

- ESP-IDF v5.2 or later
- Python 3.8+

### Build Commands

```bash
# Set target to ESP32
idf.py set-target esp32

# Configure (optional)
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Configuration

All settings are configurable via `idf.py menuconfig`:

- **WiFi**: SSID and password
- **GPIO**: Pin assignments for motors and servo
- **PWM**: Frequency and ramp settings
- **Safety**: Timeout values
- **HTTP**: Server port

## API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| POST | /api/v1/move | Move robot (direction, duration) |
| POST | /api/v1/turret | Rotate turret (direction) |
| POST | /api/v1/stop | Emergency stop |
| GET | /api/v1/status | Get robot status |
| GET | /api/v1/camera | Get camera stream URL |
| GET | /health | Health check |
| GET | / | Web UI |
| GET | /docs | API documentation |
| GET | /stream | MJPEG camera stream |

## Project Structure

```
esp32-robot/
├── main/                    # Application entry point
├── components/
│   ├── robot_hal/           # Hardware abstraction layer
│   ├── robot_core/          # Business logic facade
│   ├── motor_control/       # DRV8833 driver
│   ├── servo_control/       # SG90 servo control
│   ├── http_server/         # REST API server
│   ├── camera_stream/       # MJPEG streaming
│   ├── wifi_manager/        # WiFi connection
│   └── safety_handler/      # Safety features
├── spiffs_data/             # Web UI static files
├── test/                    # Unit tests
└── docs/                    # Documentation
```

## License

MIT License
