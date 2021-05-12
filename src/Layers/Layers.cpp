#include "Layers/Layers.h"

namespace SciRenderer
{
  //----------------------------------------------------------------------------
  // Layer begins here.
  //----------------------------------------------------------------------------
  Layer::Layer(const std::string &layerName)
    : layerName(layerName)
  { }

  // Virtual functions which must be overrided.
  Layer::~Layer()
  { }

  void
  Layer::onAttach()
  { }

  void
  Layer::onDetach()
  { }

  void
  Layer::onImGuiRender()
  { }

  void
  Layer::onEvent(Event &event)
  { }

  void
  Layer::onUpdate(float dt)
  { }

  // Getter for the name.
  std::string Layer::getName()
  {
    return this->layerName;
  }

  //----------------------------------------------------------------------------
  // Layer collection begins here.
  //----------------------------------------------------------------------------
  LayerCollection::LayerCollection()
    : insertIndex(0)
  { }

  LayerCollection::~LayerCollection()
	{
		for (auto layer : this->layers)
		{
			layer->onDetach();
			delete layer;
		}
	}

  void
  LayerCollection::pushLayer(Layer* layer)
  {
    // Insert the new layer before the overlays in the collection.
    this->layers.emplace(this->layers.begin() + this->insertIndex, layer);
		this->insertIndex++;
  }

  void
  LayerCollection::pushOverlay(Layer* overlay)
  {
    // Push the new overlay to the end of the collection.
    this->layers.emplace_back(overlay);
  }

  void
  LayerCollection::popLayer(Layer* layer)
  {
    // Try to find the layer in the collection.
    auto location = std::find(this->layers.begin(),
                             this->layers.begin() + this->insertIndex, layer);

    // If found, call its detach function and remove it. If not found, do nothing.
    if (location != this->layers.begin() + this->insertIndex)
    {
      layer->onDetach();
      this->layers.erase(location);
      this->insertIndex--;
    }
  }

  void
  LayerCollection::popOverlay(Layer* overlay)
  {
    // Try to find the overlay in the collection.
    auto location = std::find(this->layers.begin() + this->insertIndex,
                              this->layers.end(), overlay);

    // If found, call its detach function and remove it. If not found, do nothing.
    if (location != this->layers.end())
    {
      overlay->onDetach();
      this->layers.erase(location);
    }
  }
}
