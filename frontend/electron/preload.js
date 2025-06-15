const { contextBridge, ipcRenderer } = require("electron");

// Expose protected methods that allow the renderer process to use
// the ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld("electronAPI", {
  getAppVersion: () => ipcRenderer.invoke("get-app-version"),
  getPlatform: () => ipcRenderer.invoke("get-platform"),

  // Add more API methods here as needed for your AI agent
  // For example:
  // executeCommand: (command) => ipcRenderer.invoke('execute-command', command),
  // getSystemInfo: () => ipcRenderer.invoke('get-system-info'),
});
