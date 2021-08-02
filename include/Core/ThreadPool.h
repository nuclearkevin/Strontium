#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

// STL includes.
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace Strontium
{
  // A thread pool to support safe concurrency in SciRender. Its a singleton to
  // force all modes of execution to go through one pipeline, preventing unnecessary spawns.
  class ThreadPool
  {
  public:
    // Delete the copy constructor and the assignment operator. Shouldn't be
    // able to create duplicates of the pool.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool &operator=(const ThreadPool&) = delete;

    ~ThreadPool();

    // Fetch the pool.
    static ThreadPool* getInstance(unsigned int numThreads);

    // Queue up jobs for the workers to execute.
    template <typename Function, typename... Args >
    auto push(Function&& func, Args&&... args)
    {
      // Fetch the return type of the function.
      typedef decltype(func(args...)) retType;

      // Package up the function and its arguements.
      std::packaged_task<retType()> newTask(std::move(std::bind(func, args...)));

      // Lock out the task queue so the new task can be added async.
      std::unique_lock<std::mutex> lock(taskMutex);

      // Get the future for tasks which return non-void.
      std::future<retType> returnValue = newTask.get_future();

      // Push the task and notify one of the workers a job is available.
      this->tasks.emplace(createShared<ReturnJob<retType>>(std::move(newTask)));
      this->signal.notify_one();

      return returnValue;
    }

  private:
    friend class Job;
    friend class ReturnJob;

    // Inner classes to abuse function polymorphism.
    class Job
    {
    public:
      virtual ~Job()
      { }

      virtual void execute()
      { }
    };

    template <typename ReturnType>
    class ReturnJob : public Job
    {
    public:
      ReturnJob(std::packaged_task<ReturnType()> function)
        : function(std::move(function))
      { }

      void execute() override
      {
        this->function();
      }
    private:
      std::packaged_task<ReturnType()> function;
    };

    // Construct the thread pool.
    ThreadPool(unsigned int numThreads);

    static ThreadPool* instance;

    // Member variables for the pool.
    std::vector<std::thread> workers;
    std::queue<Shared<Job>> tasks;
    std::condition_variable signal;
    std::mutex taskMutex;
    std::atomic_bool isActive;
  };
}
