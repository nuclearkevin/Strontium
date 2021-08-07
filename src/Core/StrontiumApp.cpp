// Project includes.
#include "Core/EntryPoint.h"
#include "Layers/EditorLayer.h"

namespace Strontium
{
  class StrontiumApp : public Application
  {
  public:
    StrontiumApp()
      : Application("SR - Editor")
    {
      this->pushLayer(new EditorLayer());
    }

    ~StrontiumApp()
    { }
  };

  Application* makeApplication()
  {
    return new StrontiumApp();
  }
}
