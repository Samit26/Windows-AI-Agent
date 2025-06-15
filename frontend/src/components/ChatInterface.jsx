import React, { useState, useEffect, useRef } from "react";
import {
  Send,
  Loader,
  AlertCircle,
  CheckCircle,
  Clock,
  Terminal,
} from "lucide-react";
import "./ChatInterface.css";

const ChatInterface = ({ aiService }) => {
  const [messages, setMessages] = useState([]);
  const [input, setInput] = useState("");
  const [isLoading, setIsLoading] = useState(false);
  const [connectionStatus, setConnectionStatus] = useState(false);
  const [pendingExecution, setPendingExecution] = useState(null);
  const [mode, setMode] = useState("agent"); // "agent" or "chatbot"
  const messagesEndRef = useRef(null);

  useEffect(() => {
    // Check initial connection
    aiService.checkConnection();

    // Listen for connection changes
    aiService.onConnectionChange(setConnectionStatus);

    // Check connection periodically
    const connectionInterval = setInterval(() => {
      aiService.checkConnection();
    }, 5000);

    return () => clearInterval(connectionInterval);
  }, [aiService]);

  useEffect(() => {
    scrollToBottom();
  }, [messages]);

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  };

  const addMessage = (message) => {
    setMessages((prev) => [
      ...prev,
      { ...message, id: Date.now() + Math.random() },
    ]);
  };
  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!input.trim() || isLoading || !connectionStatus) return;

    const userMessage = {
      type: "user",
      content: input,
      timestamp: new Date().toLocaleTimeString(),
    };

    addMessage(userMessage);
    setInput("");
    setIsLoading(true);

    try {
      // In chatbot mode, force auto_execute to false and add mode parameter
      const autoExecute = mode === "chatbot" ? false : false; // Default to asking permission
      const response = await aiService.executeTask(input, autoExecute, mode);

      if (mode === "chatbot") {
        // In chatbot mode, just show the AI response without execution options
        const aiMessage = {
          type: "assistant",
          content:
            response.content ||
            response.message ||
            "I understand your request. In chatbot mode, I can only provide information and suggestions without executing tasks.",
          timestamp: new Date().toLocaleTimeString(),
        };
        addMessage(aiMessage);
      } else {
        // Agent mode - handle confirmation/execution flow
        if (response.response_type === "confirmation") {
          // Show conversational confirmation message
          const confirmationMessage = {
            type: "assistant",
            content: response.message,
            timestamp: new Date().toLocaleTimeString(),
            canExecute: response.can_execute,
            taskSummary: response.task_summary,
            internalPlanId: response.internal_plan_id,
          };
          addMessage(confirmationMessage);
          setPendingExecution({ input, response });
        } else if (response.response_type === "completed") {
          // Show execution result with conversational message
          const resultMessage = {
            type: "result",
            content: response.execution_result,
            timestamp: new Date().toLocaleTimeString(),
          };
          addMessage(resultMessage);
        } else if (response.execution_result) {
          // Direct execution result
          const resultMessage = {
            type: "result",
            content: response.execution_result,
            timestamp: new Date().toLocaleTimeString(),
          };
          addMessage(resultMessage);
        } else {
          // Text response
          const aiMessage = {
            type: "assistant",
            content: response.content || "No response received",
            timestamp: new Date().toLocaleTimeString(),
          };
          addMessage(aiMessage);
        }
      }
    } catch (error) {
      const errorMessage = {
        type: "error",
        content: `Error: ${error.message}`,
        timestamp: new Date().toLocaleTimeString(),
      };
      addMessage(errorMessage);
    } finally {
      setIsLoading(false);
    }
  };

  const handleExecutePlan = async () => {
    if (!pendingExecution) return;

    setIsLoading(true);
    try {
      const response = await aiService.executeTask(
        pendingExecution.input,
        true
      );

      if (response.execution_result) {
        const resultMessage = {
          type: "result",
          content: response.execution_result,
          timestamp: new Date().toLocaleTimeString(),
        };
        addMessage(resultMessage);
      }
    } catch (error) {
      const errorMessage = {
        type: "error",
        content: `Execution failed: ${error.message}`,
        timestamp: new Date().toLocaleTimeString(),
      };
      addMessage(errorMessage);
    } finally {
      setIsLoading(false);
      setPendingExecution(null);
    }
  };

  const handleCancelPlan = () => {
    setPendingExecution(null);
    const cancelMessage = {
      type: "system",
      content: "Execution cancelled by user",
      timestamp: new Date().toLocaleTimeString(),
    };
    addMessage(cancelMessage);
  };

  const renderMessage = (message) => {
    switch (message.type) {
      case "user":
        return (
          <div key={message.id} className="message user-message">
            <div className="message-content">
              <p>{message.content}</p>
              <span className="timestamp">{message.timestamp}</span>
            </div>
          </div>
        );
      case "assistant":
        return (
          <div key={message.id} className="message assistant-message">
            <div className="message-content">
              <p>{message.content}</p>

              {message.canExecute && mode === "agent" && (
                <div className="plan-actions">
                  <button
                    className="execute-btn"
                    onClick={handleExecutePlan}
                    disabled={isLoading}
                  >
                    {isLoading ? (
                      <Loader className="spinning" size={16} />
                    ) : (
                      <CheckCircle size={16} />
                    )}
                    Yes, do it!
                  </button>
                  <button
                    className="cancel-btn"
                    onClick={handleCancelPlan}
                    disabled={isLoading}
                  >
                    Not now
                  </button>
                </div>
              )}

              <span className="timestamp">{message.timestamp}</span>
            </div>
          </div>
        );
      case "plan":
        return (
          <div key={message.id} className="message plan-message">
            <div className="message-content">
              <h4>📋 Execution Plan</h4>
              <p>
                <strong>Objective:</strong> {message.content.objective}
              </p>
              <p>
                <strong>Plan ID:</strong> {message.content.plan_id}
              </p>
              <p>
                <strong>Confidence:</strong>{" "}
                {(message.content.overall_confidence * 100).toFixed(1)}%
              </p>
              <p>
                <strong>Status:</strong>{" "}
                {
                  [
                    "Pending",
                    "In Progress",
                    "Completed",
                    "Failed",
                    "Cancelled",
                  ][message.content.overall_status]
                }
              </p>

              <div className="plan-tasks">
                <h5>Tasks:</h5>
                {message.content.tasks.map((task, index) => (
                  <div
                    key={index}
                    className={`task ${
                      task.confidence_score > 0.8 ? "safe" : "risky"
                    }`}
                  >
                    <div className="task-number">{index + 1}</div>
                    <div className="task-content">
                      <p>
                        <strong>{task.description}</strong>
                      </p>
                      <div className="task-commands">
                        {task.commands.map((cmd, cmdIndex) => (
                          <code key={cmdIndex}>{cmd}</code>
                        ))}
                      </div>
                      <span className="task-confidence">
                        Confidence: {(task.confidence_score * 100).toFixed(1)}%
                      </span>
                    </div>
                  </div>
                ))}
              </div>

              <div className="plan-actions">
                <button
                  className="execute-btn"
                  onClick={handleExecutePlan}
                  disabled={isLoading}
                >
                  {isLoading ? (
                    <Loader className="spinning" size={16} />
                  ) : (
                    <CheckCircle size={16} />
                  )}
                  Execute Plan
                </button>
                <button
                  className="cancel-btn"
                  onClick={handleCancelPlan}
                  disabled={isLoading}
                >
                  Cancel
                </button>
              </div>

              <span className="timestamp">{message.timestamp}</span>
            </div>
          </div>
        );
      case "result":
        return (
          <div key={message.id} className="message result-message">
            <div className="message-content">
              {message.content.message ? (
                <>
                  <div className="ai-response">
                    <p>{message.content.message}</p>
                  </div>

                  {message.content.details && (
                    <div className="execution-details">
                      <details>
                        <summary>Technical details</summary>
                        <pre>{message.content.details}</pre>
                      </details>
                    </div>
                  )}
                </>
              ) : (
                <>
                  <h4>
                    {message.content.success ? (
                      <>
                        <CheckCircle className="success-icon" size={20} />
                        Execution Successful
                      </>
                    ) : (
                      <>
                        <AlertCircle className="error-icon" size={20} />
                        Execution Failed
                      </>
                    )}
                  </h4>

                  {message.content.output && (
                    <div className="execution-output">
                      <h5>Output:</h5>
                      <pre>{message.content.output}</pre>
                    </div>
                  )}

                  {message.content.error_message && (
                    <div className="execution-error">
                      <h5>Error:</h5>
                      <pre>{message.content.error_message}</pre>
                    </div>
                  )}
                </>
              )}

              <div className="execution-meta">
                <span>
                  <Clock size={14} /> {message.content.execution_time}s
                </span>
              </div>

              <span className="timestamp">{message.timestamp}</span>
            </div>
          </div>
        );

      case "error":
        return (
          <div key={message.id} className="message error-message">
            <div className="message-content">
              <AlertCircle className="error-icon" size={20} />
              <p>{message.content}</p>
              <span className="timestamp">{message.timestamp}</span>
            </div>
          </div>
        );

      case "system":
        return (
          <div key={message.id} className="message system-message">
            <div className="message-content">
              <p>{message.content}</p>
              <span className="timestamp">{message.timestamp}</span>
            </div>
          </div>
        );

      default:
        return null;
    }
  };

  return (
    <div className="chat-interface">
      {" "}
      <div className="chat-header">
        <h3>
          <Terminal size={20} />
          AI {mode === "agent" ? "Agent" : "Chatbot"}
        </h3>
        <div className="header-controls">
          <div className="mode-toggle">
            <button
              className={`mode-btn ${mode === "agent" ? "active" : ""}`}
              onClick={() => setMode("agent")}
              disabled={isLoading}
            >
              🤖 Agent
            </button>
            <button
              className={`mode-btn ${mode === "chatbot" ? "active" : ""}`}
              onClick={() => setMode("chatbot")}
              disabled={isLoading}
            >
              💬 Chatbot
            </button>
          </div>
          <div
            className={`connection-status ${
              connectionStatus ? "connected" : "disconnected"
            }`}
          >
            <div className="status-indicator"></div>
            {connectionStatus ? "Connected" : "Disconnected"}
          </div>
        </div>
      </div>
      <div className="messages-container">
        {" "}
        {messages.length === 0 ? (
          <div className="empty-state">
            <Terminal size={48} />
            <h3>Ready to assist you!</h3>
            {mode === "agent" ? (
              <p>
                Type a task to execute and I'll help you accomplish it safely.
              </p>
            ) : (
              <p>Ask me anything! I'm here to chat and provide information.</p>
            )}
          </div>
        ) : (
          messages.map(renderMessage)
        )}
        {isLoading && (
          <div className="loading-indicator">
            <Loader className="spinning" size={20} />
            <span>Processing...</span>
          </div>
        )}
        <div ref={messagesEndRef} />
      </div>
      <form onSubmit={handleSubmit} className="input-form">
        <div className="input-container">
          <input
            type="text"
            value={input}
            onChange={(e) => setInput(e.target.value)}
            placeholder={
              connectionStatus
                ? mode === "agent"
                  ? "Type your task..."
                  : "Ask me anything..."
                : "Waiting for backend connection..."
            }
            disabled={isLoading || !connectionStatus}
            className="message-input"
          />
          <button
            type="submit"
            disabled={isLoading || !connectionStatus || !input.trim()}
            className="send-button"
          >
            {isLoading ? (
              <Loader className="spinning" size={18} />
            ) : (
              <Send size={18} />
            )}
          </button>
        </div>
      </form>
    </div>
  );
};

export default ChatInterface;
