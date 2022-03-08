#pragma once

// STL includes.
#include <string>
#include <iostream>
#include <fstream>
#include <array>
#include <mutex>

namespace Strontium::LogInternals
{
  // The logs internal data.
  struct LogData
  {
    std::size_t messagePointer { 0u };
    std::size_t messageCount { 0u };
    std::array<std::string, 50> messages;
    std::ofstream logFile;
    std::mutex logMutex;
  };

  inline LogData logData { };
}

namespace Strontium::Logs
{
  inline void init(const std::string &filepath)
  {
    LogInternals::logData.logMutex.lock();

    // Fetch the local time.
    std::time_t current = std::time(0);
    std::tm* local = std::localtime(&current);

    // The init message.
    std::string message("");
    message += "[" + std::to_string(local->tm_hour) + ":"
             + std::to_string(local->tm_min)
             + ":" + std::to_string(local->tm_sec) + "]"
             + " Initialized application logging.\n";

    LogInternals::logData.logFile.open(filepath, std::ios::out | std::ios::app);
    std::cout << message;
    LogInternals::logData.logFile << message;
    LogInternals::logData.messages[LogInternals::logData.messagePointer] = message;
    LogInternals::logData.messagePointer++;
    LogInternals::logData.messageCount++;

    std::cout.flush();
    LogInternals::logData.logFile.flush();

    LogInternals::logData.logMutex.unlock();
  }

  inline void log(const std::string& message, bool logTime = true, bool console = true)
  {
    LogInternals::logData.logMutex.lock();

    std::string outMessage("");
    if (logTime)
    {
      std::time_t current = std::time(0);
      std::tm* local = std::localtime(&current);

      outMessage += "[" + std::to_string(local->tm_hour) + ":"
                 + std::to_string(local->tm_min)
                 + ":" + std::to_string(local->tm_sec) + "] ";
    }
    outMessage += message + "\n";

    if (console)
      std::cout << outMessage;

    LogInternals::logData.logFile << outMessage;

    LogInternals::logData.messages[LogInternals::logData.messagePointer] = outMessage;
    LogInternals::logData.messagePointer++;

    LogInternals::logData.messageCount++;
    LogInternals::logData.messageCount = std::max(LogInternals::logData.messageCount, 
                                                  static_cast<std::size_t>(50));

    if (LogInternals::logData.messagePointer >= 50)
      LogInternals::logData.messagePointer = 0;

    std::cout.flush();
    LogInternals::logData.logFile.flush();

    LogInternals::logData.logMutex.unlock();
  }

  inline void shutdown()
  {
    LogInternals::logData.logMutex.lock();

    // Fetch the local time.
    std::time_t current = std::time(0);
    std::tm* local = std::localtime(&current);

    // The init message.
    std::string message("");
    message += "[" + std::to_string(local->tm_hour) + ":"
            + std::to_string(local->tm_min)
            + ":" + std::to_string(local->tm_sec) + "]"
            + " Initialized application logging.\n";

    std::cout << message;
    LogInternals::logData.logFile << message;

    std::cout.flush();
    LogInternals::logData.logFile.flush();

    LogInternals::logData.logFile.close();

    LogInternals::logData.logMutex.unlock();
  }

  inline std::size_t getMessageCount()
  {
    LogInternals::logData.logMutex.lock();

    std::size_t count = LogInternals::logData.messageCount;

    LogInternals::logData.logMutex.unlock();

    return count;
  }

  inline std::string getMessage(std::size_t pos)
  {
    LogInternals::logData.logMutex.lock();

    std::string message = LogInternals::logData.messages[pos];

    LogInternals::logData.logMutex.unlock();

    return message;
  }

  inline std::size_t getMessagePointer()
  {
    LogInternals::logData.logMutex.lock();

    std::size_t ptr = LogInternals::logData.messagePointer;

    LogInternals::logData.logMutex.unlock();

    return ptr;
  }
}