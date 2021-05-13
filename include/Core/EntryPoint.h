#pragma once

// Project includes.
#include "Core/Application.h"
#include "Core/Logs.h"

extern SciRenderer::Application* SciRenderer::makeApplication();

int main(int argc, char** argv)
{
  // Start the application.
  SciRenderer::Application* app = SciRenderer::makeApplication();

  // Run the application.
  app->run();

  // Shutdown and delete the application.
  delete app;
}
