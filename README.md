# ESP32-CAM Robot Controller

[![CI](https://github.com/matiyas/esp32-robot/actions/workflows/ci.yml/badge.svg)](https://github.com/matiyas/esp32-robot/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/matiyas/esp32-robot)](https://github.com/matiyas/esp32-robot/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.2-blue)](https://github.com/espressif/esp-idf)

An ESP32-CAM (AI-Thinker) robot controller with REST API, camera streaming, and a web UI.
It supports two kinematics modes, selectable in `menuconfig`:

- **TANK** (default): two motors, skid-steer — the original behavior.
- **CAR**: one rear drive motor + one steering servo, driven by an ELRS/CRSF radio
  (e.g. a RadioMaster Pocket), with the camera available for low-rate FPV.

## Features

- **Motor Control**: DRV8833 dual H-bridge driver with direct PWM on motor pins
- **RC / ELRS Control (CAR mode)**: Drive from an ExpressLRS transmitter over CRSF —
  proportional throttle (forward/reverse), linear steering, optional arm switch, and a
  frame-timeout failsafe. The RC link is primary and overrides REST/web while connected.
- **Servo Control**: SG90 micro servo (camera turret in TANK mode, steering in CAR mode)
- **Camera Streaming**: Optimized MJPEG stream via built-in OV2640 camera (8MHz XCLK to avoid WiFi interference)
- **LED Control**: Toggleable flash LED for illumination
- **REST API**: Full API for robot control (OpenAPI documented)
- **Web UI**: Mobile-friendly terminal-style control dashboard with fullscreen camera view
- **Safety Features**: Watchdog timer, movement timeout, emergency stop, RC failsafe

## Hardware Requirements

- ESP32-CAM module (AI-Thinker)
- DRV8833 motor driver
- SG90 servo motor
- DC motors (2x)
- USB-to-Serial adapter (FTDI FT232RL, CP2102, or CH340) for programming
- Power supply (5V for ESP32, motor voltage for DRV8833)

## GPIO Pinout

### TANK mode (default)

| Function | Default GPIO | Notes |
|----------|--------------|-------|
| Motor Left IN1 | 12 | PWM controlled, SD card D2 (unused) |
| Motor Left IN2 | 13 | PWM controlled, SD card D3 (unused) |
| Motor Right IN1 | 14 | PWM controlled, SD card CLK (unused) |
| Motor Right IN2 | 15 | PWM controlled, SD card CMD (unused) |
| Servo Signal | 2 | SD card D0 (unused) |
| Flash LED | 4 | Built-in flash LED |

### CAR mode (ELRS / CRSF)

| Function | Default GPIO | Notes |
|----------|--------------|-------|
| Drive motor IN1 | 13 | DRV8833 AIN1, PWM |
| Drive motor IN2 | 14 | DRV8833 AIN2, PWM |
| Steering servo | 2 | SG90 signal, 50 Hz, 1000–2000 µs |
| CRSF RX | 15 | ELRS RX **TX** → ESP32 RX (UART1, GPIO matrix) |
| Flash LED / headlight | 4 | Built-in flash LED |
| Drive motor **(GPIO12)** | — | **Intentionally unused** (see strapping note) |

**DRV8833 parallel (~3A):** to double the current, tie the two channel inputs together
(AIN1+BIN1 and AIN2+BIN2) and parallel the outputs externally, then select
*Parallel channels* in menuconfig. The firmware drives the same two GPIOs (13/14) in both
SINGLE and PARALLEL modes — no extra pins are needed. Keep DRV8833 nSLEEP tied to VCC.

**Strapping-pin notes (ESP32-CAM):**

- **GPIO12 is avoided for the drive motor.** It selects flash voltage at reset; if it is HIGH
  at boot the chip tries 1.8 V flash and fails to boot. Leave it unconnected (its internal
  pulldown holds it low) — no eFuse change required.
- **GPIO2** (servo) is a boot strap and must be low/floating at reset — do not add an external
  pull-up. The unused `ROBOT_MOTORS_ENABLE` (also GPIO2) is never driven by the firmware.
- **GPIO15** (CRSF RX) is a boot strap, but a UART input idles HIGH, so boot is normal; power
  the ELRS RX together with the ESP32.
- **GPIO16/17** are used by PSRAM on the ESP32-CAM — do not reassign the servo there (the
  default was moved from GPIO16 to GPIO2 for this reason).

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
3. ESP32 will connect to the configured WiFi network
4. Find ESP32's IP address in serial monitor or router
5. Open `http://<ESP32_IP>/` in a browser

## WiFi Access Point

The ESP32 creates its own WiFi Access Point. Connect your phone/laptop directly to the robot's network.

### Default Settings

| Setting | Value |
|---------|-------|
| SSID | `ESP32` |
| Password | `eEspetrzyjsci2a` |
| IP Address | `10.42.0.1` |
| HTTP Port | `4567` |

### Connecting to the Robot

1. Power on the ESP32-CAM
2. On your phone/laptop, scan for WiFi networks
3. Connect to `ESP32` with password `eEspetrzyjsci2a`
4. Open `http://10.42.0.1:4567/` in a browser

### Custom Credentials

To change the AP credentials, edit `sdkconfig.defaults`:

```
CONFIG_ROBOT_WIFI_SSID="MyRobot"
CONFIG_ROBOT_WIFI_PASSWORD="MyPassword123"
```

Then rebuild: `rm sdkconfig && idf.py build`

**Note:** Password must be at least 8 characters for WPA2. Leave empty for open network.

### Verifying AP Startup

After boot, the serial monitor will show:
```
I (xxxx) wifi_manager: AP started - SSID: ESP32, Password: ***
I (xxxx) wifi_manager: Connect to WiFi and open http://10.42.0.1:4567/
```

## Configuration

All settings are configurable via `idf.py menuconfig`:

| Menu Path | Settings |
|-----------|----------|
| Robot Controller → WiFi Settings | SSID, password, retry count |
| Robot Controller → Motor Control | GPIO pins, PWM frequency, ramp duration |
| Robot Controller → Servo Control | GPIO pin, pulse widths, angles |
| Robot Controller → RC / CRSF | Kinematics (TANK/CAR), DRV8833 mode, CRSF UART/RX/channels, throttle & steering mapping, arm channel, failsafe timeout |
| Robot Controller → Safety Settings | Timeouts, watchdog |
| Robot Controller → HTTP Server | Port, mock mode |
| Robot Controller → Camera Settings | Resolution, quality |

### RC / CRSF options (CAR mode)

| Option | Default | Meaning |
|--------|---------|---------|
| Kinematics mode | TANK | `CAR` enables RC drive-motor + steering-servo control |
| DRV8833 drive mode | Single (~1.5A) | `Parallel` (~3A) ties both channels (same GPIOs) |
| CAR drive motor IN1 / IN2 | 13 / 14 | Drive motor pins (GPIO12 avoided) |
| CRSF UART / RX GPIO / baud | 1 / 15 / 420000 | Hardware UART for the ELRS receiver |
| Steering / Throttle / Arm channel | 1 / 2 / 5 | CRSF channels (Arm = 0 disables arm gating) |
| Throttle deadband / max duty / reverse | 20 / 100 / off | Center deadband (ticks), speed limit (%), direction |
| Steering trim / max angle / reverse | 0 / 45 / off | Center trim (deg), endpoint limit (deg), direction |
| Failsafe timeout | 500 ms | No-frame window before motor 0 + servo center |

## API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| POST | /api/v1/move | Move robot (direction, duration) |
| POST | /api/v1/turret | Rotate turret (direction) |
| POST | /api/v1/stop | Emergency stop |
| POST | /api/v1/led | Toggle LED (state: true/false) |
| GET | /api/v1/status | Get robot status |
| GET | /api/v1/camera | Get camera stream URL |
| GET | /health | Health check |
| GET | / | Web UI |
| GET | /docs | API documentation |
| GET | /stream | MJPEG camera stream |

## RC / ELRS Control (CAR mode)

In CAR mode the robot is driven by an ExpressLRS transmitter (e.g. a RadioMaster Pocket)
over CRSF. The onboard camera still streams over Wi-Fi for low-rate FPV.

### Wiring

- **ELRS receiver** (RadioMaster RP2 or any serial ELRS RX): RX **TX pin → ESP32 GPIO15**
  (CRSF RX). Power the RX from the 5 V rail; common ground with the ESP32.
- **Drive motor**: brushed DC via DRV8833 — AIN1=GPIO13, AIN2=GPIO14, nSLEEP→VCC. Motor
  supply from 4×AA NiMH (~4.8–6 V); for ~3 A use the parallel wiring above.
- **Steering servo**: SG90 signal → GPIO2; power from the 5 V rail.
- **Power**: a separate 5 V buck-boost feeds the ESP32-CAM, the ELRS RX, and the servo; the
  motor supply is raw battery. **All grounds common.** Keep the RX antenna away from the
  camera module to limit 2.4 GHz coexistence interference.

### Transmitter / receiver setup (ExpressLRS)

1. Bind the RX to the Pocket (matching ELRS firmware version and region, e.g. EU/LBT).
2. Set the RX output protocol to **Serial / CRSF at 420000 baud** (not SBUS/PWM).
3. **Set the RX failsafe to "Cut / No Pulses"** so frames stop on signal loss — this is what
   triggers the firmware failsafe. (Holding last position would not.)
4. Map your sticks/switches to the configured channels (defaults): **CH1 steering**,
   **CH2 throttle**, **CH5 arm** (a 2- or 3-position AUX switch).

### Arm, failsafe, and arbitration

- **Arm gating**: the robot boots **disarmed**; the motor stays at zero until you perform a
  **disarm → arm** cycle on the arm switch (CH5). The steering servo always follows the stick.
  Set the arm channel to `0` in menuconfig to disable arm gating entirely.
- **Failsafe**: if no valid CRSF frame arrives for `ROBOT_CRSF_FAILSAFE_TIMEOUT_MS` (default
  500 ms), an independent timer forces **motor 0 + servo center**, regardless of the control
  task. After the link returns you must re-establish neutral throttle (and re-arm) before the
  motor will spin again.
- **Arbitration**: the RC link is **primary**. While it is connected, REST/web movement and
  turret commands are ignored (the `/api/v1/status` response exposes `"rc_active": true`).
  Camera, status, and configuration endpoints stay available. When the RC link is absent,
  the REST API and web UI control the robot exactly as in TANK mode.

### FPV

Connect your phone/laptop to the ESP32 Wi-Fi AP and open the MJPEG stream (web UI camera
view, or `GET /api/v1/camera` for the URL). Expect ~150–300 ms of latency; this is a
situational-awareness feed, not a low-latency FPV link.

> **Note:** CRSF input cannot be simulated in Wokwi. The protocol parsing and mapping are
> covered by the host unit tests (`test/host`); end-to-end behavior must be validated on
> hardware.

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
