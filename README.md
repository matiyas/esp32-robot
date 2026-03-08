# ESP32-S3-CAM Robot Controller

A robot tank controller running on ESP32-S3-CAM with REST API, camera streaming, and web UI.

## Features

- **Motor Control**: DRV8833 dual H-bridge driver with PWM soft-start ramp
- **Servo Control**: SG90 micro servo for camera turret with smooth interpolation
- **Camera Streaming**: MJPEG stream via built-in camera
- **REST API**: Full API for robot control (OpenAPI documented)
- **Web UI**: Mobile-friendly control interface
- **Safety Features**: Watchdog timer, movement timeout, emergency stop

## Hardware Requirements

- ESP32-S3-CAM module
- DRV8833 motor driver
- SG90 servo motor
- DC motors (2x)
- Power supply (5V for ESP32, motor voltage for DRV8833)

## GPIO Pinout

| Function | Default GPIO |
|----------|--------------|
| Motor Left IN1 | 12 |
| Motor Left IN2 | 13 |
| Motor Right IN1 | 14 |
| Motor Right IN2 | 15 |
| Motors Enable (PWM) | 2 |
| Servo Signal | 4 |

## Building

### Prerequisites

- ESP-IDF v5.2 or later
- Python 3.8+

### Build Commands

```bash
# Set target to ESP32-S3
idf.py set-target esp32s3

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
│   ├── hal/                 # Hardware abstraction layer
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
