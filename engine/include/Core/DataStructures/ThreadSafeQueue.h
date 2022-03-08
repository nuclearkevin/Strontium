#pragma once

// STL includes.
#include <queue>
#include <mutex>

namespace Strontium
{
  template <typename T>
  class ThreadSafeQueue
  {
  public:
	ThreadSafeQueue() = default;
	~ThreadSafeQueue() = default;

	// Shouldn't be able to move or copy this.
	ThreadSafeQueue(const ThreadSafeQueue& other) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue &other) = delete;
	ThreadSafeQueue(ThreadSafeQueue &&other) = delete;
	ThreadSafeQueue& operator=(ThreadSafeQueue &&other) = delete;

	T front()
	{
	  this->internalMutex.lock();
	  T value = this->internalQueue.front();
	  this->internalMutex.unlock();

	  return value;
	}

	T back()
	{
	  this->internalMutex.lock();
	  T value = this->internalQueue.back();
	  this->internalMutex.unlock();

	  return value;
	}

	bool empty()
	{
	  this->internalMutex.lock();
	  std::size_t size = this->internalQueue.size();
	  this->internalMutex.unlock();

	  return size == 0;
	}

	std::size_t size()
	{
	  this->internalMutex.lock();
	  std::size_t size = this->internalQueue.size();
	  this->internalMutex.unlock();

	  return size;
	}

	T pop()
	{
	  this->internalMutex.lock();
	  T value = this->internalQueue.front();
	  this->internalQueue.pop();
	  this->internalMutex.unlock();

	  return value;
	}

	void push(const T &value)
	{
	  this->internalMutex.lock();
	  this->internalQueue.push(value);
	  this->internalMutex.unlock();
	}

	template <typename ... Args>
	void emplace(Args ... args)
	{
	  this->internalMutex.lock();
	  this->internalQueue.emplace(std::forward<Args>(args)...);
	  this->internalMutex.unlock();
	}
  private:
	std::queue<T> internalQueue;
	std::mutex internalMutex;
  };
}