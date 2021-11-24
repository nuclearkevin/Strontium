#pragma once

#include "StrontiumPCH.h"

namespace Strontium
{
  class Asset
  {
  public:
	  virtual ~Asset() = default;

	  virtual void load(const std::string& filepath) = 0;
	  virtual void unload() = 0;
  private:
	  std::string path;
  };
}