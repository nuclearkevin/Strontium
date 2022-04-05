#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Logs.h"

#include "Assets/Assets.h"
#include "Assets/AssetPool.h"

// STL includes.
#include <filesystem>
#include <mutex>

namespace Strontium
{
  class AssetManager
  {
  public:
	AssetManager(const std::filesystem::path &assetRegistryPath);
	~AssetManager();

	template <typename T>
	bool has(const Asset::Handle &handle)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  auto typeHash = typeid(T).hash_code();
	  if (this->assetStorage.find(typeHash) != this->assetStorage.end())
		return this->assetStorage.at(typeHash).has(handle);

	  return false;
	}

	template <typename T>
	void attach(T* asset, const Asset::Handle &handle)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  std::size_t typeHash = this->createPoolIfRequired<T>();
	  auto& pool = this->assetStorage.at(typeHash);
	  pool.attach<T>(asset, handle);
	  this->registerAsset(assetPath, handle, asset);
	}

	template <typename T, typename ... Args>
	T* emplace(const std::filesystem::path &assetPath, const Asset::Handle &handle, Args ... args)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  std::size_t typeHash = this->createPoolIfRequired<T>();
	  auto& pool = this->assetStorage.at(typeHash);
	  T* result = pool.emplace<T>(handle, std::forward<Args>(args)...);
	  this->registerAsset(assetPath, handle, result);

	  return result;
	}

	template <typename T, typename ... Args>
	T* emplaceReplace(const std::filesystem::path &assetPath, const Asset::Handle &handle, Args ... args)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  std::size_t typeHash = this->createPoolIfRequired<T>();
	  auto& pool = this->assetStorage.at(typeHash);
	  T* result = pool.emplaceReplace<T>(handle, std::forward<Args>(args)...);
	  this->registerAsset(assetPath, handle, result);

	  return result;
	}

	template <typename T>
	T* get(const Asset::Handle& handle)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  T* result = this->assetStorage.at(typeid(T).hash_code()).get<T>(handle);

	  return result;
	}

	template <typename T>
	void setDefaultAsset(T* asset)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  std::size_t typeHash = this->createPoolIfRequired<T>();
	  this->assetStorage.at(typeHash).setDefaultAsset<T>(asset);
	}

	template <typename T, typename ... Args>
	T* emplaceDefaultAsset(Args ... args)
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  std::size_t typeHash = this->createPoolIfRequired<T>();
	  T* result = this->assetStorage.at(typeHash).emplaceDefaultAsset<T>(std::forward<Args>(args)...);

	  return result;
	}

	template <typename T>
	T* getDefaultAsset()
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  T* result = this->assetStorage.at(typeid(T).hash_code()).getDefaultAsset<T>();

	  return result;
	}

	template <typename T>
	AssetPool& getPool()
	{
	  static_assert(std::is_base_of<Asset, T>::value, "Class must derive from Asset.");

	  auto& result = this->assetStorage.at(typeid(T).hash_code());

	  return result;
	}

  private:
	template <typename T>
	std::size_t createPoolIfRequired()
	{
	  auto typeHash = typeid(T).hash_code();
	  if (this->assetStorage.find(typeHash) == this->assetStorage.end())
        this->assetStorage.emplace(typeHash, AssetPool(nullptr));

	  return typeHash;
	}

	void registerAsset(const std::filesystem::path& assetPath, const Asset::Handle& handle, 
					   Asset* asset);

	void bakeAsset(const std::filesystem::path& assetPath, const Asset::Handle& handle, 
				  Asset::Type type);

	robin_hood::unordered_node_map<std::size_t, AssetPool> assetStorage;
	std::filesystem::path registryPath;
  };
}
