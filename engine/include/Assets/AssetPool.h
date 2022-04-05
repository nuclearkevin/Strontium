#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

#include "Assets/Assets.h"

namespace Strontium
{
  class AssetPool
  {
  public:
	AssetPool(Asset* defaultAsset)
	  : defaultAsset(defaultAsset)
	{ }

	AssetPool(AssetPool&& other) = default;

	~AssetPool() = default;

	bool has(const Asset::Handle &handle)
	{
	  return this->storage.find(handle) != this->storage.end();
	}

	template <typename T>
	void attach(T* asset, const Asset::Handle &handle)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  assert((!this->has(handle), "Cannot have two assets with the same handle."));

	  this->storage.emplace(handle, Unique<Asset>(asset));
	}

	template <typename T, typename ... Args>
	T* emplace(const Asset::Handle &handle, Args ... args)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  assert((!this->has(handle), "Cannot have two assets with the same handle."));

	  this->storage.emplace(handle, Unique<Asset>(new T(std::forward<Args>(args)...)));
	  return static_cast<T*>(this->storage.at(handle).get());
	}

	template <typename T, typename ... Args>
	T* emplaceReplace(const Asset::Handle &handle, Args ... args)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  if (this->has(handle))
		this->storage.erase(handle);

	  this->storage.emplace(handle, Unique<Asset>(new T(std::forward<Args>(args)...)));
	  return static_cast<T*>(this->storage.at(handle).get());
	}

	template <typename T>
	T* get(const Asset::Handle &handle)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");
	  return static_cast<T*>(this->has(handle) ? this->storage.at(handle).get() : this->getDefaultAsset<T>());
	}

	template <typename T>
	void setDefaultAsset(T* asset)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");
	  this->defaultAsset.reset(asset);
	}

	template <typename T, typename ... Args>
	T* emplaceDefaultAsset(Args ... args)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");
	  auto temp = Unique<Asset>(new T(std::forward<Args>(args)...));
	  this->defaultAsset.swap(temp);
	  return static_cast<T*>(this->defaultAsset.get());
	}

	template <typename T>
	T* getDefaultAsset()
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  return static_cast<T*>(defaultAsset.get());
	}

	robin_hood::unordered_flat_map<Asset::Handle, Unique<Asset>>::iterator begin() { return this->storage.begin(); }
	robin_hood::unordered_flat_map<Asset::Handle, Unique<Asset>>::iterator end() { return this->storage.end(); }
  private:
	robin_hood::unordered_flat_map<Asset::Handle, Unique<Asset>> storage;
	Unique<Asset> defaultAsset;
  };
}