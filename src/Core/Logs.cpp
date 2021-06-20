#include "Core/Logs.h"

// STL includes.
#include <ctime>

namespace SciRenderer
{
  Logger* Logger::appLogs = nullptr;
  std::mutex Logger::logMutex;

  // Get the logger instance.
  Logger*
  Logger::getInstance()
  {
    std::lock_guard<std::mutex> guard(logMutex);

    if (appLogs == nullptr)
    {
      appLogs = new Logger();
      return appLogs;
    }
    else
      return appLogs;
  }

  // Initialize the logs.
  void
  Logger::init()
  {
    std::lock_guard<std::mutex> guard(logMutex);

    // Fetch the local time.
    std::time_t current = std::time(0);
    std::tm* local = std::localtime(&current);

    // The init message.
    std::string message;
    message += "[" + std::to_string(local->tm_hour) + ":"
             + std::to_string(local->tm_min)
             + ":" + std::to_string(local->tm_sec) + "]"
             + " Initialized application logging.";

    // Push the init message.
    this->logs += message;
    std::cout << message << std::endl;
  }

  // Push a message to the logs.
  void
  Logger::logMessage(const LogMessage &msg)
  {
    std::lock_guard<std::mutex> guard(logMutex);

    std::string message = "";
    if (msg.logTime)
    {
      std::time_t current = std::time(0);
      std::tm* local = std::localtime(&current);

      message += "[" + std::to_string(local->tm_hour) + ":"
                + std::to_string(local->tm_min)
                + ":" + std::to_string(local->tm_sec) + "] " + msg.message;

      this->logs += "\n" + message;
    }
    else
      this->logs += "\n" + msg.message;

    if (msg.consoleOutput)
      std::cout << message << std::endl;
  }
}
