// Project includes.
#include "Core/EntryPoint.h"
#include "Layers/EditorLayer.h"

namespace SciRenderer
{
  class SciRenderApp : public Application
  {
  public:
    SciRenderApp()
      : Application("SciRenderApp")
    {
      this->pushLayer(new EditorLayer());
    }

    ~SciRenderApp()
    { }
  };

  Application* makeApplication()
  {
    return new SciRenderApp();
  }
}
