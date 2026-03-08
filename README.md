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
- USB-to-Serial adapter (FTDI FT232RL, CP2102, or CH340) for programming
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

## Development Environment Setup

### Prerequisites

| Tool | Version | Description |
|------|---------|-------------|
| ESP-IDF | v5.2+ | Espressif IoT Development Framework |
| Python | 3.8+ | Required by ESP-IDF |
| Git | 2.x | Version control |
| CMake | 3.16+ | Build system (included in ESP-IDF) |
| Ninja | 1.10+ | Build tool (included in ESP-IDF) |

### Installing ESP-IDF (Linux/macOS)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install git wget flex bison gperf python3 python3-pip \
    python3-venv cmake ninja-build ccache libffi-dev libssl-dev \
    dfu-util libusb-1.0-0

# Install dependencies (macOS)
brew install cmake ninja dfu-util python3

# Clone ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone -b v5.2 --recursive https://github.com/espressif/esp-idf.git

# Install ESP-IDF tools
cd ~/esp/esp-idf
./install.sh esp32

# Set up environment (add to ~/.bashrc or ~/.zshrc for persistence)
. ~/esp/esp-idf/export.sh
```

### Installing ESP-IDF (Windows)

Download and run the [ESP-IDF Tools Installer](https://dl.espressif.com/dl/esp-idf/?idf=4.4) from Espressif.

## Building the Project

### 1. Clone the Repository

```bash
git clone https://github.com/matiyas/esp32-robot.git
cd esp32-robot
```

### 2. Set Up ESP-IDF Environment

```bash
# Linux/macOS
. ~/esp/esp-idf/export.sh

# Windows (use ESP-IDF Command Prompt)
```

### 3. Configure the Project

```bash
# Set target to ESP32
idf.py set-target esp32

# Configure WiFi and other settings (optional)
idf.py menuconfig
```

In menuconfig, navigate to:
- `Robot Controller Configuration → WiFi Settings` - Set your WiFi SSID and password
- `Robot Controller Configuration → Motor Control` - Adjust GPIO pins if needed
- `Robot Controller Configuration → Servo Control` - Adjust servo settings

### 4. Build

```bash
idf.py build
```

Build output:
- `build/esp32_robot.bin` - Main application
- `build/bootloader/bootloader.bin` - Bootloader
- `build/partition_table/partition-table.bin` - Partition table
- `build/spiffs.bin` - Web UI filesystem

## Deploying to ESP32-CAM

### Wiring for Programming

Connect USB-to-Serial adapter to ESP32-CAM:

| USB-Serial | ESP32-CAM |
|------------|-----------|
| TX | U0R (GPIO 3) |
| RX | U0T (GPIO 1) |
| GND | GND |
| 5V | 5V |

**Important**: Connect GPIO 0 to GND to enter flash mode before powering on.

### Flashing

```bash
# Flash all images (bootloader, partition table, app, SPIFFS)
idf.py -p /dev/ttyUSB0 flash

# Flash and open serial monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Exit monitor: Ctrl+]
```

**Port names:**
- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
- macOS: `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART`
- Windows: `COM3` (check Device Manager)

### Manual Flashing with esptool

```bash
python -m esptool --chip esp32 -p /dev/ttyUSB0 -b 460800 \
    --before default_reset --after hard_reset write_flash \
    --flash_mode dio --flash_size 4MB --flash_freq 40m \
    0x1000 build/bootloader/bootloader.bin \
    0x8000 build/partition_table/partition-table.bin \
    0x10000 build/esp32_robot.bin \
    0x310000 build/spiffs.bin
```

### After Flashing

1. Disconnect GPIO 0 from GND
2. Press RESET button or power cycle the board
3. Connect to the configured WiFi network
4. Find ESP32's IP address in serial monitor or router
5. Open `http://<ESP32_IP>/` in a browser

## Configuration

All settings are configurable via `idf.py menuconfig`:

| Menu Path | Settings |
|-----------|----------|
| Robot Controller → WiFi Settings | SSID, password, retry count |
| Robot Controller → Motor Control | GPIO pins, PWM frequency, ramp duration |
| Robot Controller → Servo Control | GPIO pin, pulse widths, angles |
| Robot Controller → Safety Settings | Timeouts, watchdog |
| Robot Controller → HTTP Server | Port, mock mode |
| Robot Controller → Camera Settings | Resolution, quality |

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

## Troubleshooting

### Flash fails with "Failed to connect"
- Ensure GPIO 0 is connected to GND
- Press and hold RESET, then release while flashing starts
- Try lower baud rate: `idf.py -p /dev/ttyUSB0 -b 115200 flash`

### No serial output
- Check TX/RX connections (try swapping)
- Ensure correct port and baud rate (115200)
- Verify power supply (5V, sufficient current)

### Camera not working
- Check PSRAM is enabled in sdkconfig
- Verify camera ribbon cable is properly seated
- Try lower resolution in menuconfig

### WiFi connection fails
- Verify SSID and password in menuconfig
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check signal strength

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
