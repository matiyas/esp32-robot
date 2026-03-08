// API client for robot control
class RobotAPI {
  constructor(baseUrl = '') {
    this.baseUrl = baseUrl;
  }

  async move(direction, duration = 1000) {
    return this.post('/api/v1/move', { direction, duration });
  }

  async turret(direction, duration = 500) {
    return this.post('/api/v1/turret', { direction, duration });
  }

  async stop() {
    return this.post('/api/v1/stop', {});
  }

  async getStatus() {
    return this.get('/api/v1/status');
  }

  async getCameraUrl() {
    return this.get('/api/v1/camera');
  }

  async post(endpoint, data) {
    try {
      const response =
        await fetch(this.baseUrl + endpoint, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify(data),
        });

      if (!response.ok) {
        const error = await response.json();
        throw new Error(error.error || 'Request failed');
      }

      return await response.json();
    } catch (error) {
      console.error(`API Error [${endpoint}]:`, error);
      throw error;
    }
  }

  async get(endpoint) {
    try {
      const response =
        await fetch(this.baseUrl + endpoint, {
          method: 'GET',
          headers: {
            'Content-Type': 'application/json',
          },
        });

      if (!response.ok) {
        const error = await response.json();
        throw new Error(error.error || 'Request failed');
      }

      return await response.json();
    } catch (error) {
      console.error(`API Error [${endpoint}]:`, error);
      throw error;
    }
  }
}
