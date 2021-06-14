#include "GuiElements/AssetBrowserWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"
#include "Scenes/Entity.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// STL includes.
#include <filesystem>
#include <sstream>

namespace SciRenderer
{
  AssetBrowserWindow::AssetBrowserWindow()
    : GuiWindow()
    , currentDir("./assets")
  {
    Texture2D* folder = Texture2D::loadTexture2D("./assets/icons/folder2.png", Texture2DParams(), false);
    Texture2D* file = Texture2D::loadTexture2D("./assets/icons/file.png", Texture2DParams(), false);
    this->icons.insert({ "folder", folder });
    this->icons.insert({ "file", file });
  }

  AssetBrowserWindow::~AssetBrowserWindow()
  {
    for (auto& pair : this->icons)
      delete pair.second;
  }

  void
  AssetBrowserWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    ImGui::Begin("Content Browser:", &isOpen);

    // A check to make sure the current directory exists. If it doesn't, we move
    // back one directory and try again. If 'assets' doesn't exist we quit early,
    // there are bigger problems if that folder is gone.
    if (!std::filesystem::exists(this->currentDir))
    {
      if (this->currentDir != "./assets")
        this->currentDir = this->currentDir.substr(0, this->currentDir.find_last_of('/'));

      ImGui::End();
      return;
    }

    float fontSize = ImGui::GetFontSize();
    ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick
                               | ImGuiSelectableFlags_AllowItemOverlap;

    ImVec2 textSize;
    ImVec2 cursorPos = ImGui::GetCursorPos();

    // Draws the go back icon to return to the parent directory of the current
    // directory. Stops at 'assets'.
    if (this->currentDir != "./assets")
    {
      std::string prevFolderName = this->currentDir.substr(0, this->currentDir.find_last_of('/'));
      prevFolderName = prevFolderName.substr(prevFolderName.find_last_of('/') + 1);

      textSize = ImGui::CalcTextSize(prevFolderName.c_str());

      ImGui::Selectable("##goback", false, flags, ImVec2(64.0f, 64.0f));

      if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
        this->currentDir = this->currentDir.substr(0, this->currentDir.find_last_of('/'));

      ImGui::SetCursorPos(cursorPos);
      ImGui::Image((ImTextureID) (unsigned long) this->icons["folder"]->getID(),
                   ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + 64.0f + fontSize));
      ImGui::Text(prevFolderName.c_str());
      ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + 64.0f + 2 * fontSize));
    }
    else
    {
      textSize = ImGui::CalcTextSize("Previous Folder");

      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Selectable("##goback", false, flags, ImVec2(64.0f, 64.0f));
      ImGui::SetCursorPos(cursorPos);
      ImGui::Image((ImTextureID) (unsigned long) this->icons["folder"]->getID(),
                   ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + 64.0f + fontSize));
      ImGui::Text("Previous Folder");
      ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + 64.0f + 2 * fontSize));
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }

    this->drawFolders(activeScene);
    this->drawFiles(activeScene);

    ImGui::End();
  }

  void
  AssetBrowserWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

  void
  AssetBrowserWindow::onEvent(Event &event)
  {

  }

  void
  AssetBrowserWindow::drawFolders(Shared<Scene> activeScene)
  {
    float fontSize = ImGui::GetFontSize();
    ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick
                               | ImGuiSelectableFlags_AllowItemOverlap;

    ImVec2 textSize, cursorPos;

    // Iterate over the directory and find all the folders.
    for (const auto& entry : std::filesystem::directory_iterator(this->currentDir))
    {
      std::stringstream stream;
      if (entry.is_directory())
      {
        // Extract the folder path.
        stream << entry;
        std::string folderPath = stream.str();

        // Extract the folder name.
        auto lastQuote = folderPath.find_last_of('"');
        auto lastSlash = folderPath.find_last_of('/');
        std::string folderName = folderPath.substr(lastSlash + 1, lastQuote - lastSlash - 1);

        // Ignore hidden folders.
        if (folderName[0] == '.')
          continue;

        textSize = ImGui::CalcTextSize(folderName.c_str());
        cursorPos = ImGui::GetCursorPos();

        ImGui::Selectable((std::string("##") + folderName).c_str(), false, flags, ImVec2(64.0f, 64.0f));

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
          this->currentDir = this->currentDir + "/" + folderName;

        ImGui::SetCursorPos(cursorPos);
        ImGui::Image((ImTextureID) (unsigned long) this->icons["folder"]->getID(),
                     ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + 64.0f + fontSize));
        ImGui::Text(folderName.c_str());
        ImGui::SetCursorPos(ImVec2(cursorPos.x + (textSize.x > 64.0f ? textSize.x + 10.0f : 64.0f), cursorPos.y));
      }
    }
  }

  void
  AssetBrowserWindow::drawFiles(Shared<Scene> activeScene)
  {
    float fontSize = ImGui::GetFontSize();
    ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick
                               | ImGuiSelectableFlags_AllowItemOverlap;

    ImVec2 textSize, cursorPos;

    // Iterate over the directory and find all the files.
    for (const auto& entry : std::filesystem::directory_iterator(this->currentDir))
    {
      std::stringstream stream;
      if (entry.is_regular_file())
      {
        // Extract the file path.
        stream << entry;
        std::string filepath = stream.str();

        // Extract the file name.
        auto lastQuote = filepath.find_last_of('"');
        auto lastSlash = filepath.find_last_of('/');
        std::string fileName = filepath.substr(lastSlash + 1, lastQuote - lastSlash - 1);

        // Ignore hidden files.
        if (fileName[0] == '.')
          continue;

        textSize = ImGui::CalcTextSize(fileName.c_str());
        cursorPos = ImGui::GetCursorPos();

        ImGui::Selectable((std::string("##") + fileName).c_str(), false, flags, ImVec2(64.0f, 64.0f));

        // Setting up the drag and drop source for the filepath.
        if (ImGui::BeginDragDropSource())
        {
          // The items to be dragged along with the cursor.
          ImGui::Image((ImTextureID) (unsigned long) this->icons["file"]->getID(),
                       ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
          ImGui::Text(fileName.c_str());

          // The things I'll do to get a proper payload...
          std::string tempPath = this->currentDir + "/" + fileName;
          char pathBuffer[256];
          memset(pathBuffer, 0, sizeof(pathBuffer));
          std::strncpy(pathBuffer, tempPath.c_str(), sizeof(pathBuffer));

          // Set the payload.
          ImGui::SetDragDropPayload("ASSET_PATH", &pathBuffer, sizeof(pathBuffer));
          ImGui::EndDragDropSource();
        }

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
          this->loadAssetFromFile(fileName, this->currentDir + "/" + fileName, activeScene);

        ImGui::SetCursorPos(cursorPos);
        ImGui::Image((ImTextureID) (unsigned long) this->icons["file"]->getID(),
                     ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + 64.0f + fontSize));
        ImGui::Text(fileName.c_str());
        ImGui::SetCursorPos(ImVec2(cursorPos.x + (textSize.x > 64.0f ? textSize.x + 10.0f : 64.0f), cursorPos.y));
      }
    }
  }

  void
  AssetBrowserWindow::loadAssetFromFile(const std::string &name,
                                        const std::string &path,
                                        Shared<Scene> activeScene)
  {
    std::string fileType = name.substr(name.find_last_of('.'));

    // If its a supported model file, load it as a new entity in the scene.
    if (fileType == ".obj" || fileType == ".FBX" || fileType == ".fbx")
    {
      auto modelAssets = AssetManager<Model>::getManager();

      auto model = activeScene->createEntity(name.substr(0, name.find_last_of('.')));
      model.addComponent<TransformComponent>();
      model.addComponent<RenderableComponent>(modelAssets->loadAssetFile(path, name), name);
    }

    // If its a supported image, load and cache it.
    if (fileType == ".jpg" || fileType == ".tga" || fileType == ".png")
    {
      Texture2D::loadTexture2D(path);
    }
  }
}
