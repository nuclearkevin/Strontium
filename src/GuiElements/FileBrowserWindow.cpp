#include "GuiElements/FileBrowserWindow.h"

namespace SciRenderer
{
  FileBrowserWindow::FileBrowserWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , isOpen(false)
    , format(".")
    , mode(imgui_addons::ImGuiFileBrowser::DialogMode::SELECT)
  {

  }

  FileBrowserWindow::~FileBrowserWindow()
  {

  }

  void
  FileBrowserWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    bool openFileBrowser = this->isOpen;

    if (openFileBrowser)
      ImGui::OpenPopup("File Browser");

    if (this->fileHandler.showFileDialog("File Browser", this->mode,
        ImVec2(700, 310), this->format))
    {
      std::string name = this->fileHandler.selected_fn;
      std::string path = this->fileHandler.selected_path;

      EventDispatcher* dispatcher = EventDispatcher::getInstance();

      if (this->mode == imgui_addons::ImGuiFileBrowser::DialogMode::OPEN)
        dispatcher->queueEvent(new LoadFileEvent(path, name));
      else if (this->mode == imgui_addons::ImGuiFileBrowser::DialogMode::SAVE)
        dispatcher->queueEvent(new SaveFileEvent(path, name));

      this->isOpen = false;
    }

    if (this->fileHandler.shouldClose)
      this->isOpen = false;
  }

  void
  FileBrowserWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

  void
  FileBrowserWindow::onEvent(Event &event)
  {
    switch(event.getType())
    {
      case EventType::OpenDialogueEvent:
      {
        auto openEvent = *(static_cast<OpenDialogueEvent*>(&event));

        this->isOpen = true;
        this->format = openEvent.getFormat();

        switch (openEvent.getDialogueType())
        {
          case DialogueEventType::FileOpen: this->mode =
            imgui_addons::ImGuiFileBrowser::DialogMode::OPEN; break;
          case DialogueEventType::FileSave: this->mode =
            imgui_addons::ImGuiFileBrowser::DialogMode::SAVE; break;
          case DialogueEventType::FileSelect: this->mode =
            imgui_addons::ImGuiFileBrowser::DialogMode::SELECT; break;
          default: break;
        }
        break;
      }
      default:
        break;
    }
  }
}
