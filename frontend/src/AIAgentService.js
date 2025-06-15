class AIAgentService {
  constructor(baseUrl = "http://localhost:8080") {
    this.baseUrl = baseUrl;
    this.isConnected = false;
    this.connectionCallbacks = [];
  }

  // Connection management
  async checkConnection() {
    try {
      const response = await fetch(`${this.baseUrl}/api/system-info`);
      this.isConnected = response.ok;
      this.notifyConnectionChange();
      return this.isConnected;
    } catch (error) {
      this.isConnected = false;
      this.notifyConnectionChange();
      return false;
    }
  }

  onConnectionChange(callback) {
    this.connectionCallbacks.push(callback);
  }

  notifyConnectionChange() {
    this.connectionCallbacks.forEach((callback) => callback(this.isConnected));
  }
  // API methods
  async executeTask(input, autoExecute = false, mode = "agent") {
    try {
      const response = await fetch(`${this.baseUrl}/api/execute`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          input,
          auto_execute: autoExecute,
          mode: mode, // "agent" or "chatbot"
        }),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error("Execute task error:", error);
      throw error;
    }
  }

  async getHistory() {
    try {
      const response = await fetch(`${this.baseUrl}/api/history`);
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      return await response.json();
    } catch (error) {
      console.error("Get history error:", error);
      throw error;
    }
  }

  async getSystemInfo() {
    try {
      const response = await fetch(`${this.baseUrl}/api/system-info`);
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      return await response.json();
    } catch (error) {
      console.error("Get system info error:", error);
      throw error;
    }
  }

  async updatePreferences(preferences) {
    try {
      const response = await fetch(`${this.baseUrl}/api/preferences`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(preferences),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error("Update preferences error:", error);
      throw error;
    }
  }

  async getActiveProcesses() {
    try {
      const response = await fetch(`${this.baseUrl}/api/processes`);
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      return await response.json();
    } catch (error) {
      console.error("Get processes error:", error);
      throw error;
    }
  }

  async rollbackLastAction() {
    try {
      const response = await fetch(`${this.baseUrl}/api/rollback`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error("Rollback error:", error);
      throw error;
    }
  }

  async getSuggestions() {
    try {
      const response = await fetch(`${this.baseUrl}/api/suggestions`);
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      return await response.json();
    } catch (error) {
      console.error("Get suggestions error:", error);
      throw error;
    }
  }

  async handleVoiceInput(audioData) {
    try {
      const response = await fetch(`${this.baseUrl}/api/voice`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          audio_data: audioData,
        }),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error("Voice input error:", error);
      throw error;
    }
  }

  async handleImageInput(imageData) {
    try {
      const response = await fetch(`${this.baseUrl}/api/image`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          image_data: imageData,
        }),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error("Image input error:", error);
      throw error;
    }
  }
}

export default AIAgentService;
