// Project includes.
#include "Core/EntryPoint.h"
#include "EditorLayer.h"

// For loading in the task bar icon.
#include "stb/stb_image.h"

namespace Strontium
{
  class StrontiumApp : public Application
  {
  public:
    StrontiumApp()
      : Application("Strontium 90")
    {
      this->pushLayer(new EditorLayer());
      this->appWindow->setVSync(false);

      // Load in the window icon.
      unsigned char* dataU = nullptr;
      int width, height, n;
      stbi_set_flip_vertically_on_load(true);
      dataU = stbi_load("./assets/.icons/strontium.png", &width, &height, &n, 0);
      if (dataU && n == 4)
      {
        this->appWindow->setIcon(width, height, dataU);
        stbi_image_free(dataU);
      }
    }

    ~StrontiumApp()
    { }
  };

  Application* makeApplication()
  {
    return new StrontiumApp();
  }
}
