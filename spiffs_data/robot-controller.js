// Robot controller - main UI logic
class RobotController {
  constructor() {
    this.api = new RobotAPI();
    this.isConnected = false;
    this.movementDuration = 250;
    this.turretDuration = 350;
    this.ledState = false;
    this.moveAbortController = null;

    this.init();
  }

  async init() {
    console.log('Initializing Robot Controller...');

    this.setupMovementControls();
    this.setupTurretControls();
    this.setupEmergencyStop();
    this.setupLedToggle();
    this.setupFullscreen();

    await this.loadCameraStream();
    await this.checkStatus();

    setInterval(() => this.checkStatus(), 5000);

    console.log('Robot Controller initialized');
  }

  setupMovementControls() {
    const directions = ['forward', 'backward', 'left', 'right'];

    directions.forEach(direction => {
      const btn = document.getElementById(`btn${this.capitalize(direction)}`);
      if (btn) {
        this.setupButton(btn, () => this.move(direction));
      }
    });
  }

  setupTurretControls() {
    const btnLeft = document.getElementById('btnTurretLeft');
    const btnRight = document.getElementById('btnTurretRight');

    if (btnLeft) this.setupButton(btnLeft, () => this.turret('left'));
    if (btnRight) this.setupButton(btnRight, () => this.turret('right'));
  }

  setupEmergencyStop() {
    const btnStop = document.getElementById('btnStop');
    if (btnStop) btnStop.addEventListener('click', () => this.emergencyStop());
  }

  setupLedToggle() {
    const btnLed = document.getElementById('btnLed');
    if (btnLed) btnLed.addEventListener('click', () => this.toggleLed());
  }

  setupFullscreen() {
    const btnFullscreen = document.getElementById('btnFullscreen');
    const cameraSection = document.getElementById('cameraSection');

    if (btnFullscreen && cameraSection) {
      btnFullscreen.addEventListener('click', () => this.toggleFullscreen());

      document.addEventListener('fullscreenchange', () => {
        if (!document.fullscreenElement) {
          cameraSection.classList.remove('fullscreen');
          btnFullscreen.textContent = '⛶';
        }
      });

      document.addEventListener('keydown', e => {
        if (e.key === 'Escape' && cameraSection.classList.contains('fullscreen')) {
          this.toggleFullscreen();
        }
      });
    }
  }

  toggleFullscreen() {
    const cameraSection = document.getElementById('cameraSection');
    const btnFullscreen = document.getElementById('btnFullscreen');

    if (cameraSection.classList.contains('fullscreen')) {
      cameraSection.classList.remove('fullscreen');
      btnFullscreen.textContent = '⛶';
      if (document.fullscreenElement) {
        document.exitFullscreen();
      }
    } else {
      cameraSection.classList.add('fullscreen');
      btnFullscreen.textContent = '✕';
      if (cameraSection.requestFullscreen) {
        cameraSection.requestFullscreen().catch(() => {});
      }
    }
  }

  setupButton(button, action) {
    let pressTimer = null;
    let isPressed = false;

    const startPress = e => {
      e.preventDefault();
      if (isPressed) return;

      isPressed = true;
      button.classList.add('active');

      this.moveAbortController = new AbortController();

      action();

      pressTimer = setInterval(() => {
        if (isPressed) action();
      }, 100);

      console.log(`Button pressed: ${button.id}`);
    };

    const endPress = e => {
      if (!isPressed) return;

      isPressed = false;
      button.classList.remove('active');

      if (pressTimer) {
        clearInterval(pressTimer);
        pressTimer = null;
      }

      if (this.moveAbortController) {
        this.moveAbortController.abort();
        this.moveAbortController = null;
      }

      this.stopMovement();

      console.log(`Button released: ${button.id}`);
    };

    button.addEventListener('mousedown', startPress);
    button.addEventListener('mouseup', endPress);
    button.addEventListener('mouseleave', endPress);

    button.addEventListener('touchstart', startPress);
    button.addEventListener('touchend', endPress);
    button.addEventListener('touchcancel', endPress);
  }

  async move(direction) {
    try {
      const result =
        await this.api.move(direction, this.movementDuration, this.moveAbortController?.signal);
      console.log('Move command sent:', result);
    } catch (error) {
      if (error.name !== 'AbortError') {
        console.error('Move failed:', error);
      }
    }
  }

  async turret(direction) {
    try {
      const result =
        await this.api.turret(direction, this.turretDuration, this.moveAbortController?.signal);
      console.log('Turret command sent:', result);
    } catch (error) {
      if (error.name !== 'AbortError') {
        console.error('Turret command failed:', error);
        this.showError('Turret command failed');
      }
    }
  }

  async emergencyStop() {
    try {
      const result = await this.api.stop();
      console.log('Emergency stop:', result);
      this.showStatus('STOPPED', 'error');
    } catch (error) {
      console.error('Emergency stop failed:', error);
      this.showError('Emergency stop failed');
    }
  }

  async toggleLed() {
    try {
      this.ledState = !this.ledState;
      const btnLed = document.getElementById('btnLed');
      const result = await this.api.led(this.ledState);
      btnLed.classList.toggle('active', this.ledState);
      console.log('LED toggled:', result);
    } catch (error) {
      this.ledState = !this.ledState;
      console.error('LED toggle failed:', error);
      this.showError('LED toggle failed');
    }
  }

  async stopMovement() {
    try {
      const result = await this.api.stop();
      console.log('Movement stopped');
    } catch (error) {
      console.error('Stop movement failed:', error);
    }
  }

  async loadCameraStream() {
    try {
      const response = await this.api.getCameraUrl();
      const streamUrl = response.stream_url;

      const cameraImg = document.getElementById('cameraStream');
      const cameraError = document.getElementById('cameraError');

      cameraImg.onload = () => {
        cameraError.style.display = 'none';
        console.log('Camera stream loaded');
      };

      cameraImg.onerror = () => {
        cameraError.style.display = 'block';
        console.warn('Camera stream unavailable');
      };

      cameraImg.src = streamUrl;
    } catch (error) {
      console.error('Failed to load camera stream:', error);
      document.getElementById('cameraError').style.display = 'block';
    }
  }

  async checkStatus() {
    try {
      const status = await this.api.getStatus();
      this.isConnected = status.connected;
      this.updateStatusUI(true);
    } catch (error) {
      this.isConnected = false;
      this.updateStatusUI(false);
      console.error('Status check failed:', error);
    }
  }

  updateStatusUI(connected) {
    const statusElement = document.getElementById('status');
    const statusText = document.getElementById('statusText');

    if (connected) {
      statusElement.classList.remove('disconnected');
      statusElement.classList.add('connected');
      statusText.textContent = 'Connected';
    } else {
      statusElement.classList.remove('connected');
      statusElement.classList.add('disconnected');
      statusText.textContent = 'Disconnected';
    }
  }

  showStatus(message, type = 'info') {
    console.log(`[${type.toUpperCase()}] ${message}`);
  }

  showError(message) {
    this.showStatus(message, 'error');
  }

  capitalize(str) {
    return str.charAt(0).toUpperCase() + str.slice(1);
  }
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', () => {
    window.robotController = new RobotController();
  });
} else {
  window.robotController = new RobotController();
}
