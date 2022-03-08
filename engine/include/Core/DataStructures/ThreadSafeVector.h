#pragma once

// STL includes.
#include <vector>
#include <mutex>

namespace Strontium
{
  template <typename T>
  class ThreadSafeVector
  {
  public:
	ThreadSafeVector() = default;
	~ThreadSafeVector() = default;

	// Shouldn't be able to move or copy this.
	ThreadSafeVector(const ThreadSafeVector& other) = delete;
	ThreadSafeVector& operator=(const ThreadSafeVector& other) = delete;
	ThreadSafeVector(ThreadSafeVector&& other) = delete;
	ThreadSafeVector& operator=(ThreadSafeVector&& other) = delete;
	
	T at(std::size_t loc)
	{
	  this->internalMutex.lock();
	  T value = this->internalVector.at(loc);
	  this->internalMutex.unlock();

	  return value;
	}

	T operator [](std::size_t pos)
	{
	  this->internalMutex.lock();
	  T value = this->internalVector[loc];
	  this->internalMutex.unlock();

	  return value;
	}

	T front()
	{
	  this->internalMutex.lock();
	  T value = this->internalVector.front();
	  this->internalMutex.unlock();

	  return value;
	}

	T back()
	{
	  this->internalMutex.lock();
	  T value = this->internalVector.back();
	  this->internalMutex.unlock();

	  return value;
	}

	bool empty()
	{
	  this->internalMutex.lock();
	  bool value = this->internalVector.size() == 0;
	  this->internalMutex.unlock();

	  return value;
	}

	std::size_t size()
	{
	  this->internalMutex.lock();
	  std::size_t value = this->internalVector.size();
	  this->internalMutex.unlock();

	  return value;
	}

	std::size_t maxSize()
	{
	  this->internalMutex.lock();
	  std::size_t value = this->internalVector.max_size();
	  this->internalMutex.unlock();

	  return value;
	}

	void reserve()
	{
	  this->internalMutex.lock();
	  this->internalVector.reserve();
	  this->internalMutex.unlock();
	}

	std::size_t capacity()
	{
	  this->internalMutex.lock();
	  std::size_t value = this->internalVector.capacity();
	  this->internalMutex.unlock();

	  return value;
	}

	void shrinkToFit()
	{
	  this->internalMutex.lock();
	  this->internalVector.shrink_to_fit();
	  this->internalMutex.unlock();
	}

	void clear()
	{
	  this->internalMutex.lock();
	  this->internalVector.clear();
	  this->internalMutex.unlock();
	}

	void insert(std::size_t pos, const T &value)
	{
	  this->internalMutex.lock();
	  this->internalVector.insert(pos, value);
	  this->internalMutex.unlock();
	}

	template <typename ... Args>
	void emplace(Args ... args)
	{
	  this->internalMutex.lock();
	  this->internalVector.emplace(std::forward<Args>(args)...);
	  this->internalMutex.unlock();
	}

	void erase(std::size_t pos)
	{
	  this->internalMutex.lock();
	  this->internalVector.erase(pos);
	  this->internalMutex.unlock();
	}

	void erase(std::size_t start, std::size_t end)
	{
	  this->internalMutex.lock();
	  this->internalVector.erase(start, end);
	  this->internalMutex.unlock();
	}

	void pushBack(const T& value)
	{
	  this->internalMutex.lock();
	  this->internalVector.push_back(value);
	  this->internalMutex.unlock();
	}

	template <typename ... Args>
	void emplaceBack(Args ... args)
	{
	  this->internalMutex.lock();
	  this->internalVector.emplace_back(std::forward<Args>(args)...);
	  this->internalMutex.unlock();
	}

	T popBack()
	{
	  this->internalMutex.lock();
	  T value = this->internalVector.back();
	  this->internalVector.pop_back();
	  this->internalMutex.unlock();

	  return value;
	}

	void resize(std::size_t size)
	{
	  this->internalMutex.lock();
	  this->internalVector.resize(size);
	  this->internalMutex.unlock();
	}
  private:
	std::vector<T> internalVector;
	std::mutex internalMutex;
  };
}