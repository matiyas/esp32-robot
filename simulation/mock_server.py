#!/usr/bin/env python3
"""
Mock server for testing ESP32 Robot web dashboard locally.

Usage:
    python simulation/mock_server.py

Then open http://localhost:4567 in your browser.
"""

import json
import os
from http.server import HTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse

PORT = 4567
SPIFFS_DIR = os.path.join(os.path.dirname(__file__), '..', 'spiffs_data')

# Mock state
state = {
    'led': False,
    'connected': True,
    'gpio_enabled': True
}


class MockAPIHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=SPIFFS_DIR, **kwargs)

    def do_GET(self):
        path = urlparse(self.path).path

        if path == '/api/v1/status':
            self.send_json({'success': True, 'connected': state['connected'], 'gpio_enabled': state['gpio_enabled']})
        elif path == '/api/v1/camera':
            self.send_json({'success': True, 'stream_url': '/placeholder-camera.svg'})
        elif path == '/health':
            self.send_json({'status': 'ok'})
        elif path == '/placeholder-camera.svg':
            self.send_placeholder_camera()
        else:
            super().do_GET()

    def do_POST(self):
        path = urlparse(self.path).path
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8') if content_length else '{}'

        try:
            data = json.loads(body) if body else {}
        except json.JSONDecodeError:
            data = {}

        if path == '/api/v1/move':
            direction = data.get('direction', 'unknown')
            duration = data.get('duration', 0)
            print(f"[MOCK] Move: {direction} for {duration}ms")
            self.send_json({'success': True, 'action': direction, 'duration': duration})

        elif path == '/api/v1/turret':
            direction = data.get('direction', 'unknown')
            print(f"[MOCK] Turret: {direction}")
            self.send_json({'success': True, 'action': f'turret_{direction}'})

        elif path == '/api/v1/stop':
            print("[MOCK] Emergency stop")
            self.send_json({'success': True, 'action': 'stop'})

        elif path == '/api/v1/led':
            state['led'] = data.get('state', False)
            print(f"[MOCK] LED: {'ON' if state['led'] else 'OFF'}")
            self.send_json({'success': True, 'state': state['led']})

        else:
            self.send_error(404, 'Not Found')

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_cors_headers()
        self.end_headers()

    def send_json(self, data):
        response = json.dumps(data).encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', len(response))
        self.send_cors_headers()
        self.end_headers()
        self.wfile.write(response)

    def send_cors_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')

    def send_placeholder_camera(self):
        svg = '''<svg xmlns="http://www.w3.org/2000/svg" width="640" height="480" viewBox="0 0 640 480">
            <rect fill="#111" width="640" height="480"/>
            <text x="320" y="230" text-anchor="middle" fill="#00ff41" font-family="monospace" font-size="20">[ CAMERA SIMULATION ]</text>
            <text x="320" y="260" text-anchor="middle" fill="#00aa2a" font-family="monospace" font-size="14">No live feed in mock mode</text>
        </svg>'''
        response = svg.encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'image/svg+xml')
        self.send_header('Content-Length', len(response))
        self.end_headers()
        self.wfile.write(response)

    def log_message(self, format, *args):
        if '/api/' in args[0] or args[0].startswith('POST'):
            print(f"[HTTP] {args[0]}")


def main():
    os.chdir(SPIFFS_DIR)
    server = HTTPServer(('0.0.0.0', PORT), MockAPIHandler)
    print(f"Mock server running at http://localhost:{PORT}")
    print(f"Serving files from: {os.path.abspath(SPIFFS_DIR)}")
    print("Press Ctrl+C to stop\n")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped")


if __name__ == '__main__':
    main()
