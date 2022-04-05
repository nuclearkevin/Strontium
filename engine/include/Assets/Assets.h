#pragma once

#include "StrontiumPCH.h"

// STL includes.
#include <filesystem>

namespace Strontium
{
  class Asset
  {
  public:
	enum class Type
    {
	  Image2D = 0u,
	  Model = 1u,
	  Material = 2u
    };

	using Handle = std::string;

	Asset(Type type)
	  : path()
	  , type(type)
	{ }

	virtual ~Asset() = default;

	virtual void load(const std::filesystem::path &path) = 0;
	virtual void unload() = 0;

	Type getType() const { return this->type; }
	std::filesystem::path getPath() const { return this->path; };
  protected:
    std::filesystem::path path;
	Type type;

	friend class AssetManager;
	friend class AssetPool;
  };
}