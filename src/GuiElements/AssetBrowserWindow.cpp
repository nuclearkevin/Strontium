#include "GuiElements/AssetBrowserWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"
#include "Scenes/Entity.h"
#include "GuiElements/Styles.h"

namespace Strontium
{
  AssetBrowserWindow::AssetBrowserWindow(EditorLayer* parentLayer)
    : GuiWindow(parentLayer)
    , currentDir("./assets")
    , loadingAsset(false)
    , loadingAssetText("")
    , searched("")
  {
    // Load in the icons.
    Texture2D* tex;
    tex = Texture2D::loadTexture2D("./assets/.icons/folder.png", Texture2DParams(), false);
    this->icons.insert({ "folder", tex });
    tex = Texture2D::loadTexture2D("./assets/.icons/file.png", Texture2DParams(), false);
    this->icons.insert({ "file", tex });
    tex = Texture2D::loadTexture2D("./assets/.icons/srfile.png", Texture2DParams(), false);
    this->icons.insert({ "srfile", tex });
    tex = Texture2D::loadTexture2D("./assets/.icons/sfabfile.png", Texture2DParams(), false);
    this->icons.insert({ "sfabfile", tex });
  }

  AssetBrowserWindow::~AssetBrowserWindow()
  {
    for (auto& pair : this->icons)
      delete pair.second;
  }

  void
  AssetBrowserWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  {
    ImGui::Begin("Content Browser", &isOpen);

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

    ImGui::BeginChild("DirTree", ImVec2(256.0f, 0.0f));
    this->drawDirectoryTree();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("Content");

    float cellSize = 64.0f + 16.0f;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int numColumns = (int) (panelWidth / cellSize);
    numColumns = numColumns < 1 ? 1 : numColumns;

    // Draws the go back icon to return to the parent directory of the current
    // directory. Stops at 'assets'.
    if (this->currentDir != "./assets")
    {
      std::string prevFolderName = this->currentDir.substr(0, this->currentDir.find_last_of('/'));
      prevFolderName = prevFolderName.substr(prevFolderName.find_last_of('/') + 1);

      ImGui::Button(ICON_FA_ARROW_LEFT);

      if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
      {
        this->currentDir = this->currentDir.substr(0, this->currentDir.find_last_of('/'));
      }
    }
    else
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Button(ICON_FA_ARROW_LEFT);
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_RIGHT))
    {

    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_REFRESH))
    {

    }

    char searchBuffer[256];
    memset(searchBuffer, 0, sizeof(searchBuffer));
    std::strncpy(searchBuffer, searched.c_str(), sizeof(searchBuffer));

    ImGui::SameLine();
    if (ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer)))
      searched = std::string(searchBuffer);

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_SEARCH))
      searched = std::string(searchBuffer);

    ImGui::Columns(numColumns, 0, false);
    this->drawFolders(activeScene);
    this->drawFiles(activeScene);

    if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight, false))
    {
      if (ImGui::BeginMenu("New"))
      {
        if (ImGui::MenuItem("Folder"))
        {

        }
        if (ImGui::MenuItem("Material"))
        {

        }
        if (ImGui::MenuItem("Prefab"))
        {

        }
        ImGui::EndMenu();
      }
      ImGui::EndPopup();
    }

    ImGui::EndChild();
    ImGui::End();

    if (this->loadingAsset)
    {
      auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

      float fontSize = ImGui::GetFontSize();
      ImGui::Begin("Loading...", nullptr, flags);
      ImGui::Spinner("##loadingspinner", fontSize / 2.0f, 4, ImGui::GetColorU32(ImGuiCol_Button));
      ImGui::SameLine();
      ImGui::Text((std::string("Loading file ") + this->loadingAssetText).c_str());
      ImGui::End();
    }
  }

  void
  AssetBrowserWindow::onUpdate(float dt, Shared<Scene> activeScene)
  {

  }

  void
  AssetBrowserWindow::onEvent(Event &event)
  {
    switch (event.getType())
    {
      case EventType::GuiEvent:
      {
        auto guiEvent = *(static_cast<GuiEvent*>(&event));

        switch (guiEvent.getGuiEventType())
        {
          case GuiEventType::StartSpinnerEvent:
          {
            this->loadingAsset = true;
            this->loadingAssetText = guiEvent.getText();
            break;
          }

          case GuiEventType::EndSpinnerEvent:
          {
            this->loadingAsset = false;
            this->loadingAssetText = "";
            break;
          }

          default: break;
        }
        break;
      }

      default: break;
    }
  }

  void
  AssetBrowserWindow::drawDirectoryTree(const std::string &root)
  {
    // Exit if the root directory doesn't exist.
    if (!std::filesystem::exists(root))
      return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                             | ImGuiTreeNodeFlags_DefaultOpen;

    std::filesystem::path rootPath(root);

    bool opened = ImGui::TreeNodeEx(rootPath.filename().string().c_str(), flags);

    if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
      this->currentDir = root;

    if (opened)
    {
      this->drawDirectoryNode(root, 0);
      ImGui::TreePop();
    }
  }

  void
  AssetBrowserWindow::drawDirectoryNode(const std::string &path, unsigned int level)
  {
    ImGuiTreeNodeFlags leafFlag = ImGuiTreeNodeFlags_Leaf
                                | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
      std::string name = entry.path().filename().string();

      if (entry.is_directory() && name[0] != '.')
      {
        bool opened = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow);

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
          this->currentDir = entry.path().string();

        if (opened)
        {
          this->drawDirectoryNode(entry.path().string(), level + 1);
          ImGui::TreePop();
        }
      }
    }

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
      std::string name = entry.path().filename().string();

      if (entry.is_regular_file() && name[0] != '.')
      {
        ImGui::TreeNodeEx(name.c_str(), leafFlag);

        // Setting up the drag and drop source for the filepath.
        if (ImGui::BeginDragDropSource())
        {
          // The items to be dragged along with the cursor.
          ImGui::Text(name.c_str());

          std::string filepath = entry.path().string();
          char pathBuffer[256];
          memset(pathBuffer, 0, sizeof(pathBuffer));
          std::strncpy(pathBuffer, filepath.c_str(), sizeof(pathBuffer));

          // Set the payload.
          ImGui::SetDragDropPayload("ASSET_PATH", &pathBuffer, sizeof(pathBuffer));
          ImGui::EndDragDropSource();
        }
      }
    }
  }

  // This segfaults?
  void
  AssetBrowserWindow::drawFolders(Shared<Scene> activeScene)
  {
    ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick
                               | ImGuiSelectableFlags_AllowItemOverlap;

    // Iterate over the directory and find all the folders.
    for (const auto& entry : std::filesystem::directory_iterator(this->currentDir))
    {
      if (entry.is_directory())
      {
        // Extract the folder name.
        std::string folderName = entry.path().filename().string();

        // Ignore hidden folders.
        if (folderName[0] == '.')
          continue;

        if (this->searched != "" && folderName.find(this->searched) == std::string::npos)
          continue;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::ImageButton((ImTextureID) (unsigned long) this->icons["folder"]->getID(),
                     ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleColor();

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
        {
          this->currentDir = this->currentDir + "/" + folderName;
        }

        ImGui::TextWrapped(folderName.c_str());

        ImGui::NextColumn();
      }
    }
  }

  void
  AssetBrowserWindow::drawFiles(Shared<Scene> activeScene)
  {
    ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowItemOverlap;

    // Iterate over the directory and find all the files.
    for (const auto& entry : std::filesystem::directory_iterator(this->currentDir))
    {
      if (entry.is_regular_file())
      {
        // Extract the file name.
        std::string fileName = entry.path().filename().string();
        std::string fileExt = entry.path().extension().string();

        // Ignore hidden files.
        if (fileName[0] == '.')
          continue;

        if (this->searched != "" && fileName.find(this->searched) == std::string::npos)
          continue;

        auto cursorPos = ImGui::GetCursorPos();
        ImGui::Selectable(std::string("##" + fileName).c_str(), false,
                          ImGuiSelectableFlags_AllowItemOverlap,
                          ImVec2(64.0f, 64.0f));

        // Setting up the drag and drop source for the filepath.
        if (ImGui::BeginDragDropSource())
        {
          // The items to be dragged along with the cursor.
          if (fileExt == ".srn")
          {
            ImGui::Image((ImTextureID) (unsigned long) this->icons["srfile"]->getID(),
                         ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
          }
          else if (fileExt == ".sfab")
          {
            ImGui::Image((ImTextureID) (unsigned long) this->icons["sfabfile"]->getID(),
                         ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
          }
          else
          {
            ImGui::Image((ImTextureID) (unsigned long) this->icons["file"]->getID(),
                         ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
          }
          ImGui::Text(fileName.c_str());

          std::string filepath = entry.path().string();
          char pathBuffer[256];
          memset(pathBuffer, 0, sizeof(pathBuffer));
          std::strncpy(pathBuffer, filepath.c_str(), sizeof(pathBuffer));

          // Set the payload.
          ImGui::SetDragDropPayload("ASSET_PATH", &pathBuffer, sizeof(pathBuffer));
          ImGui::EndDragDropSource();
        }

        // The items to be dragged along with the cursor.
        ImGui::SetCursorPos(cursorPos);
        if (fileExt == ".srn")
        {
          ImGui::Image((ImTextureID) (unsigned long) this->icons["srfile"]->getID(),
                       ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
        }
        else if (fileExt == ".sfab")
        {
          ImGui::Image((ImTextureID) (unsigned long) this->icons["sfabfile"]->getID(),
                       ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
        }
        else
        {
          ImGui::Image((ImTextureID) (unsigned long) this->icons["file"]->getID(),
                       ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
        }
        ImGui::TextWrapped(fileName.c_str());

        ImGui::NextColumn();
      }
    }
  }

  // Handle creation of assets.
  void
  AssetBrowserWindow::createMaterial()
  {

  }
}
