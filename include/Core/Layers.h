#pragma once

// Macro include file.
#include "SciRenderPCH.h"

namespace SciRenderer
{
  // A layer class which determines the order things are drawn to the screen.
  // Also holds the scenes and other fun stuff.
  class Layer
  {
  public:
    // Constructor-destructor pair.
    Layer(const std::string &layerName);
    virtual ~Layer();

    // Functions to determine layer behavior. Must override these.
    virtual void onAttach();
    virtual void onDetach();
    virtual void onImGuiRender();
    virtual void onEvent();
    virtual void onUpdate(float dt);

    // Get the name of the layer.
    auto getName() -> std::string;

  protected:
    std::string layerName;
  };

  // A collection of layers which will be handled front to back.
  class LayerCollection
  {
  public:
    LayerCollection();
    ~LayerCollection();

    void pushLayer(Layer* layer);
    void pushOverlay(Layer* overlay);
    void popLayer(Layer* layer);
    void popOverlay(Layer* overlay);

    std::vector<Layer*>::iterator begin() { return layers.begin(); }
		std::vector<Layer*>::iterator end() { return layers.end(); }
		std::vector<Layer*>::reverse_iterator rbegin() { return layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return layers.rend(); }

		std::vector<Layer*>::const_iterator begin() const { return layers.begin(); }
		std::vector<Layer*>::const_iterator end()	const { return layers.end(); }
		std::vector<Layer*>::const_reverse_iterator rbegin() const { return layers.rbegin(); }
		std::vector<Layer*>::const_reverse_iterator rend() const { return layers.rend(); }

  private:
    std::vector<Layer*> layers;
    GLuint              insertIndex;
  };
}
