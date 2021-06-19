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

    // Setup the queue and push the init message.
    this->lastFrameLogs = std::queue<std::string>();
    this->lastFrameLogs.push(message);

    this->globalLogs = std::queue<std::string>();
    this->globalLogs.push(message);

    std::cout << message << std::endl;
  }

  // Push a message to the logs.
  void
  Logger::logMessage(const LogMessage &msg)
  {
    std::lock_guard<std::mutex> guard(logMutex);

    std::string messages = "";

    if (msg.logTime)
    {
      std::time_t current = std::time(0);
      std::tm* local = std::localtime(&current);

      messages += "[" + std::to_string(local->tm_hour) + ":"
                + std::to_string(local->tm_min)
                + ":" + std::to_string(local->tm_sec) + "] " + msg.message;

      this->lastFrameLogs.push(messages);

      if (msg.addToGlobal)
      {
        if (globalLogs.size() < 1000)
          globalLogs.push(messages);
        else
        {
          globalLogs.pop();
          globalLogs.push(messages);
        }
      }
    }
    else
    {
      messages = msg.message;

      this->lastFrameLogs.push(messages);

      if (msg.addToGlobal)
      {
        if (globalLogs.size() < 1000)
          globalLogs.push(messages);
        else
        {
          globalLogs.pop();
          globalLogs.push(messages);
        }
      }
    }

    if (msg.consoleOutput)
      std::cout << messages << std::endl;
  }

  // Get the last frame logged messages.
  std::string
  Logger::getLastMessages()
  {
    std::lock_guard<std::mutex> guard(logMutex);

    std::string messages = "";

    while (!this->lastFrameLogs.empty())
    {
      messages += this->lastFrameLogs.front();
      this->lastFrameLogs.pop();
      messages += "\n";
    }

    return messages;
  }

  // Get the global logged messages.
  std::string
  Logger::getGlobalLogs()
  {
    std::lock_guard<std::mutex> guard(logMutex);

    std::string messages = "";

    while (!this->globalLogs.empty())
    {
      messages += this->globalLogs.front();
      this->globalLogs.pop();
      messages += "\n";
    }

    return messages;
  }
}
