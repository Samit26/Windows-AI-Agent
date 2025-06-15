import React, { useState, useEffect } from "react";
import {
  Bot,
  Settings,
  MessageSquare,
  Activity,
  Menu,
  Wifi,
  WifiOff,
} from "lucide-react";
import ChatInterface from "./components/ChatInterface.jsx";
import AIAgentService from "./AIAgentService.js";
import "./App.css";

function App() {
  const [appVersion, setAppVersion] = useState("");
  const [platform, setPlatform] = useState("");
  const [sidebarOpen, setSidebarOpen] = useState(true);
  const [activeTab, setActiveTab] = useState("chat");
  const [aiService] = useState(() => new AIAgentService());
  const [backendConnected, setBackendConnected] = useState(false);

  useEffect(() => {
    // Get app info from Electron if available
    if (window.electronAPI) {
      window.electronAPI.getAppVersion().then(setAppVersion);
      window.electronAPI.getPlatform().then(setPlatform);
    }

    // Monitor backend connection
    aiService.onConnectionChange(setBackendConnected);
    aiService.checkConnection();
  }, [aiService]);

  return (
    <div className="app">
      {/* Header */}
      <header className="app-header">
        <div className="header-left">
          <button
            className="menu-button"
            onClick={() => setSidebarOpen(!sidebarOpen)}
          >
            <Menu size={20} />
          </button>
          <h1 className="app-title">
            <Bot size={24} />
            Windows AI Agent
          </h1>
        </div>{" "}
        <div className="header-right">
          <div className="backend-status">
            {backendConnected ? (
              <>
                <Wifi size={16} className="connected" /> Backend Connected
              </>
            ) : (
              <>
                <WifiOff size={16} className="disconnected" /> Backend Offline
              </>
            )}
          </div>
          <span className="version-info">
            v{appVersion} • {platform}
          </span>
        </div>
      </header>

      <div className="app-body">
        {/* Sidebar */}{" "}
        <aside className={`sidebar ${sidebarOpen ? "open" : "closed"}`}>
          <nav className="sidebar-nav">
            <button
              className={`nav-item ${activeTab === "chat" ? "active" : ""}`}
              onClick={() => setActiveTab("chat")}
            >
              <MessageSquare size={20} />
              {sidebarOpen && "Chat"}
            </button>
            <button
              className={`nav-item ${activeTab === "tasks" ? "active" : ""}`}
              onClick={() => setActiveTab("tasks")}
            >
              <Activity size={20} />
              {sidebarOpen && "Tasks"}
            </button>
            <button
              className={`nav-item ${activeTab === "settings" ? "active" : ""}`}
              onClick={() => setActiveTab("settings")}
            >
              <Settings size={20} />
              {sidebarOpen && "Settings"}
            </button>
          </nav>
        </aside>{" "}
        {/* Main Content */}
        <main className="main-content">
          {activeTab === "chat" && <ChatInterface aiService={aiService} />}
          {activeTab === "tasks" && (
            <div className="tab-content">
              <h2>Task Management</h2>
              <p>View and manage your automated tasks here.</p>
            </div>
          )}
          {activeTab === "settings" && (
            <div className="tab-content">
              <h2>Settings</h2>
              <p>Configure your AI agent preferences here.</p>
            </div>
          )}
        </main>
      </div>
    </div>
  );
}

export default App;
