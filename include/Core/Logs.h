// Include guard.
#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// STL includes.
#include <mutex>

namespace SciRenderer
{
  // A struct to contain the parameters for a logged message.
  struct LogMessage
  {
    std::string message;
    bool        logTime;
    bool        consoleOutput;

    LogMessage()
      : message(std::string(""))
      , logTime(true)
      , consoleOutput(false)
    { };

    LogMessage(std::string msg)
      : message(msg)
      , logTime(true)
      , consoleOutput(true)
    { };

    LogMessage(std::string msg, bool logTime, bool console)
      : message(msg)
      , logTime(logTime)
      , consoleOutput(console)
    { };
  };

  // Singleton application logging class.
  class Logger
  {
  public:
    // Destructor.
    ~Logger() = default;

    // Log mutex for thread safety.
    static std::mutex logMutex;

    // Get the logger instance.
    static Logger* getInstance();

    // Initialize the application logs.
    void init();

    // Add a message to the logger.
    void logMessage(const LogMessage &msg);

    // Fetches all the logged messages
    // from the previous frame as a
    // single string. This clears the log.
    std::string& getLogs() { return this->logs; }

  private:
    // The instance.
    static Logger* appLogs;

    // Queue to store the logs from the previous frame.
    std::string logs;
  };
}
