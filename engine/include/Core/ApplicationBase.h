#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// STL includes.
#include <cstdint>
#include <memory>

// Basic typedefs.
typedef std::uint16_t ushort;
typedef std::uint32_t uint;
typedef std::uint64_t ulong;

namespace Strontium
{
  // Rewrapping smart pointers to make using them easier.
  template <typename T>
  using Shared = std::shared_ptr<T>;

  template <typename T, typename ... Args>
  constexpr Shared<T> createShared(Args ... args)
  {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }

  template <typename T>
  using Unique = std::unique_ptr<T>;

  template <typename T, typename ... Args>
  constexpr Unique<T> createUnique(Args ... args)
  {
    return std::make_unique<T>(std::forward<Args>(args)...);
  }

  // A memory pool for managing resources. Uses a vector to minimize cache
  // misses (I guess?).
  template <class U, class T>
  class MemoryPool
  {
  public:
    MemoryPool() = default;
    ~MemoryPool() = default;

    // Check to see if the pool has an item given a key.
    bool has(const U &key)
    {
      auto loc = std::find_if(this->managed.begin(), this->managed.end(),
                              [&key](const std::pair<U, T> &pair)
                              { return pair.first == key; });

      return loc != this->managed.end();
    }

    // Construct a managed item in-place.
    template <typename ... Args>
    T* emplace(const U &key, Args ... args)
    {
      this->managed.emplace_back(std::make_pair(key, std::forward<Args>(args)...));
      return &(this->managed.back().second);
    }

    // Get a managed object.
    T* get(const U &key)
    {
      auto loc = std::find_if(this->managed.begin(), this->managed.end(),
                              [&key](const std::pair<U, T> &pair)
                              { return pair.first == key; });

      if (loc != this->managed.end())
        return &(loc->second);
      else
        return nullptr;
    }

    // Delete a managed object.
    void erase(const U &key)
    {
      auto loc = std::find_if(this->managed.begin(), this->managed.end(),
                              [&key](const std::pair<U, T> &pair)
                              { return pair.first == key; });
      if (loc != this->managed.end())
        this->managed.erase(loc);
    }

    unsigned int size() { return this->managed.size(); }
  private:
    std::vector<std::pair<U, T>> managed;
  };
}
