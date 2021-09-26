#include "Core/ThreadPool.h"

namespace Strontium
{
  //----------------------------------------------------------------------------
  // Singleton thread pool.
  //----------------------------------------------------------------------------
  ThreadPool* ThreadPool::instance = nullptr;

  ThreadPool::ThreadPool(unsigned int numThreads)
  {
    this->isActive.store(true);

    auto workerFunction = [](ThreadPool* parentPool)
    {
      while (true)
      {
        std::unique_lock<std::mutex> taskLock(parentPool->taskMutex);
        parentPool->signal.wait(taskLock, [parentPool]()
        {
          return !parentPool->tasks.empty() || !parentPool->isActive.load(std::memory_order_relaxed);
        });

        if (!parentPool->isActive.load(std::memory_order_relaxed))
          break;

        (*parentPool->tasks.front()).execute();
        parentPool->tasks.pop();
      }
    };

    this->workers.reserve(numThreads);

    for (unsigned int i = 0; i < numThreads; i++)
    {
      this->workers.emplace_back(workerFunction, this);
      this->workers.back().detach();
    }
  }

  ThreadPool::~ThreadPool()
  {
    this->isActive.store(false, std::memory_order_relaxed);

    this->signal.notify_all();

    for (auto& worker : this->workers)
    {
      if (worker.joinable())
        worker.join();
    }
  }

  ThreadPool*
  ThreadPool::getInstance(unsigned int numThreads)
  {
    if (instance == nullptr)
    {
      instance = new ThreadPool(numThreads);
      return instance;
    }
    else
      return instance;
  }
}
