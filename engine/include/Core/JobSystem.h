#pragma once

// STL includes.
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace Strontium
{
  //------------------------------------------------------------
  // Internal jobsystem API.
  //------------------------------------------------------------
  namespace JobSystemInternal
  {
    // The jobs to execute.
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

    // The threadpool internal data.
    struct PoolData
    {
      std::vector<std::thread> workers;
      std::queue<std::shared_ptr<Job>> tasks;
      std::condition_variable signal;
      std::mutex taskMutex;
      std::atomic_bool isActive;
    };

    inline PoolData poolData { };
  }

  //------------------------------------------------------------
  // External jobsystem API.
  //------------------------------------------------------------
  namespace JobSystem
  {
    inline void 
    init(unsigned int numThreads)
    {
      JobSystemInternal::poolData.isActive.store(true);

      auto workerFunction = []()
      {
        while (true)
        {
          std::unique_lock<std::mutex> taskLock(JobSystemInternal::poolData.taskMutex);
          JobSystemInternal::poolData.signal.wait(taskLock, []()
          {
            return !JobSystemInternal::poolData.tasks.empty() || !JobSystemInternal::poolData.isActive.load(std::memory_order_relaxed);
          });
      
          if (!JobSystemInternal::poolData.isActive.load(std::memory_order_relaxed))
            return;
      
          JobSystemInternal::poolData.tasks.front()->execute();
          JobSystemInternal::poolData.tasks.pop();
        }
      };
      
      JobSystemInternal::poolData.workers.reserve(numThreads);
      
      for (unsigned int i = 0; i < numThreads; i++)
      {
        JobSystemInternal::poolData.workers.emplace_back(workerFunction);
        JobSystemInternal::poolData.workers.back().detach();
      }
    }

    inline void
    shutdown()
    {
      JobSystemInternal::poolData.isActive.store(false, std::memory_order_relaxed);

      JobSystemInternal::poolData.signal.notify_all();
      
      for (auto& worker : JobSystemInternal::poolData.workers)
      {
        if (worker.joinable())
          worker.join();
      }
      
      std::unique_lock<std::mutex> taskLock(JobSystemInternal::poolData.taskMutex);
    }
  
    // Queue up jobs for the workers to execute.
    template <typename Function, typename... Args >
    auto push(Function&& func, Args&&... args)
    {
      typedef decltype(func(args...)) retType;
  
      std::packaged_task<retType()> newTask(std::move(std::bind(func, args...)));
  
      std::unique_lock<std::mutex> lock(JobSystemInternal::poolData.taskMutex);
  
      std::future<retType> returnValue = newTask.get_future();
  
      JobSystemInternal::poolData.tasks.emplace(createShared<JobSystemInternal::ReturnJob<retType>>(std::move(newTask)));
      JobSystemInternal::poolData.signal.notify_one();
  
      return returnValue;
    }
  }
}
