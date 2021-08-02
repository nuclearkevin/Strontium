#pragma once

// Project includes.
#include "Core/Application.h"
#include "Core/Logs.h"

extern Strontium::Application* Strontium::makeApplication();

int main(int argc, char** argv)
{
  // Start the application.
  Strontium::Application* app = Strontium::makeApplication();

  // Run the application.
  app->run();

  // Shutdown and delete the application.
  delete app;
}
