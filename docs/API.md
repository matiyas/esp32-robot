# Robot Tank Control API

REST API documentation for the ESP32-CAM robot controller.

## Base URL

```
http://robot.local
```

## Authentication

No authentication required (local network only).

## Endpoints

### Health Check

```
GET /health
```

Simple health check endpoint.

**Response:**
```json
{
  "status": "ok"
}
```

### Get Status

```
GET /api/v1/status
```

Returns current robot status.

**Response:**
```json
{
  "success": true,
  "connected": true,
  "gpio_enabled": true
}
```

### Get Camera URL

```
GET /api/v1/camera
```

Returns the camera stream URL.

**Response:**
```json
{
  "success": true,
  "stream_url": "/stream"
}
```

### Move Robot

```
POST /api/v1/move
```

Move the robot in the specified direction.

**Request Body:**
```json
{
  "direction": "forward",
  "duration": 1000
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| direction | string | Yes | "forward", "backward", "left", "right" |
| duration | integer | No | Duration in milliseconds (0 = continuous) |

**Response:**
```json
{
  "success": true,
  "action": "forward",
  "duration": 1000
}
```

### Control Turret

```
POST /api/v1/turret
```

Step the camera turret left or right.

**Request Body:**
```json
{
  "direction": "left"
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| direction | string | Yes | "left" or "right" |
| duration | integer | No | Not used (step mode) |

**Response:**
```json
{
  "success": true,
  "action": "turret_left"
}
```

### Emergency Stop

```
POST /api/v1/stop
```

Immediately stop all motors.

**Response:**
```json
{
  "success": true,
  "action": "stop"
}
```

### Control LED

```
POST /api/v1/led
```

Toggle the flash LED on/off.

**Request Body:**
```json
{
  "state": true
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| state | boolean | Yes | true = on, false = off |

**Response:**
```json
{
  "success": true
}
```

### Camera Stream

```
GET /stream
```

MJPEG camera stream (available on port 4568).

**Note:** The camera stream runs on a separate HTTP server (main port + 1) to avoid blocking the control API.

## Error Responses

All errors return:

```json
{
  "success": false,
  "error": "Error message"
}
```

## CORS

All endpoints support CORS with the following headers:
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS`
- `Access-Control-Allow-Headers: Content-Type, Authorization`
