#pragma once

// Macro include file.
#include "SciRenderPCH.h"

namespace SciRenderer
{
  // A layer class which determines the order things are drawn to the screen.
  class Layer
  {
  public:
    // Constructor-destructor pair.
    Layer(const std::string &layerName);
    virtual ~Layer();

    // Functions to determine layer behavior. Must override these.
    virtual void onAttach();
    virtual void onDetach();
    virtual void onUpdate(float dt);

    // Get the name of the layer.
    auto getName() -> std::string;

  protected:
    std::string layerName;
  };
}
