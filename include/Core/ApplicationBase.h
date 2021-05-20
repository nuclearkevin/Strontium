#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Memory management library.
#include <memory>

namespace SciRenderer
{
  // This is heavily inspired by the work done in Hazel, a 2D game engine by
  // TheCherno.
  // https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Core/Base.h
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
}
