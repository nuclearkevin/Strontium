#include "Core/Layers.h"

namespace SciRenderer
{
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
  Layer::onUpdate(float dt)
  { }

  // Getter for the name.
  std::string Layer::getName()
  {
    return this->layerName;
  }
}
