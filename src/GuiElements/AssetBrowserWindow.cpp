#include "GuiElements/AssetBrowserWindow.h"

// Project includes.
#include "Core/AssetManager.h"
#include "GuiElements/Styles.h"
#include "Scenes/Entity.h"

// STL includes.
#include <filesystem>

namespace SciRenderer
{
  AssetBrowserWindow::AssetBrowserWindow()
    : GuiWindow()
    , currentDir("./assets")
    , drawCursor(0.0f, 0.0f)
    , loadingAsset(false)
    , loadingAssetText("")
  {
    // Load in the icons.
    Texture2D* tex;
    tex = Texture2D::loadTexture2D("./assets/.icons/folder.png", Texture2DParams(), false);
    this->icons.insert({ "folder", tex });
    tex = Texture2D::loadTexture2D("./assets/.icons/backfolder.png", Texture2DParams(), false);
    this->icons.insert({ "backfolder", tex });
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

    ImGui::BeginChild("DirTree", ImVec2(256.0f, 0.0f));
    this->drawDirectoryTree();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("Content");

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

      ImGui::Selectable("##goback", false, flags, ImVec2(64.0f, 64.0f));

      if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
        this->currentDir = this->currentDir.substr(0, this->currentDir.find_last_of('/'));

      ImGui::SetCursorPos(cursorPos);
      ImGui::Image((ImTextureID) (unsigned long) this->icons["backfolder"]->getID(),
                   ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
    }
    else
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      ImGui::Selectable("##goback", false, flags, ImVec2(64.0f, 64.0f));
      ImGui::SetCursorPos(cursorPos);
      ImGui::Image((ImTextureID) (unsigned long) this->icons["backfolder"]->getID(),
                   ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
    ImGui::SetCursorPos(ImVec2(cursorPos.x + 64.0f + 8.0f, cursorPos.y));

    float maxCursorYPos = 0;
    this->drawFolders(activeScene, maxCursorYPos);
    this->drawFiles(activeScene, maxCursorYPos);

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

          // The things I'll do to get a proper payload...
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

  void
  AssetBrowserWindow::drawFolders(Shared<Scene> activeScene, float &maxCursorYPos)
  {
    float fontSize = ImGui::GetFontSize();

    ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick
                               | ImGuiSelectableFlags_AllowItemOverlap;

    ImVec2 textSize, cursorPos;

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

        textSize = ImGui::CalcTextSize(folderName.c_str());
        cursorPos = ImGui::GetCursorPos();

        ImGui::Selectable((std::string("##") + folderName).c_str(), false, flags, ImVec2(64.0f, 64.0f));

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
          this->currentDir = this->currentDir + "/" + folderName;

        ImGui::SetCursorPos(cursorPos);
        ImGui::Image((ImTextureID) (unsigned long) this->icons["folder"]->getID(),
                     ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0));

        // Align the folder text so the icons are evenly spaced out.
        unsigned int numChars = (unsigned int) (2 * 64.0f / fontSize);
        unsigned int stringPos = 0;
        float yPos = cursorPos.y + 64.0f + fontSize;
        unsigned int stride;
        for (unsigned int i = 0; i < (unsigned int) (textSize.x / 64.0f) + 1; i++)
        {
          ImGui::SetCursorPos(ImVec2(cursorPos.x, yPos));

          stride = ((stringPos + numChars) < folderName.size()) ? numChars : folderName.size() - stringPos;

          ImGui::Text(folderName.substr(stringPos, stride).c_str());
          stringPos += numChars;
          yPos += fontSize;
        }

        // Handle the content potentially being offscreen for small
        // content browsers. Move offscreen content to a new line.
        maxCursorYPos = std::max(std::ceil((textSize.x / 64.0f) + 1.0f) * fontSize + cursorPos.y + 64.0f + fontSize, maxCursorYPos);
        ImVec2 windowSize = ImGui::GetWindowSize();
        if (windowSize.x - (64.0f + 8.0f) > cursorPos.x + 64.0f + 8.0f)
          ImGui::SetCursorPos(ImVec2(cursorPos.x + 64.0f + 8.0f, cursorPos.y));
        else
          ImGui::SetCursorPos(ImVec2(0.0f, maxCursorYPos));
      }
    }
  }

  void
  AssetBrowserWindow::drawFiles(Shared<Scene> activeScene, float &maxCursorYPos)
  {
    float fontSize = ImGui::GetFontSize();
    ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowItemOverlap;

    ImVec2 textSize, cursorPos;

    // Iterate over the directory and find all the files.
    for (const auto& entry : std::filesystem::directory_iterator(this->currentDir))
    {
      if (entry.is_regular_file())
      {
        // Extract the file name.
        std::string filename = entry.path().filename().string();
        std::string fileExt = entry.path().extension().string();

        // Ignore hidden files.
        if (filename[0] == '.')
          continue;

        textSize = ImGui::CalcTextSize(filename.c_str());
        cursorPos = ImGui::GetCursorPos();

        ImGui::Selectable((std::string("##") + filename).c_str(), false, flags, ImVec2(64.0f, 64.0f));

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
          ImGui::Text(filename.c_str());

          // The things I'll do to get a proper payload...
          std::string filepath = entry.path().string();
          char pathBuffer[256];
          memset(pathBuffer, 0, sizeof(pathBuffer));
          std::strncpy(pathBuffer, filepath.c_str(), sizeof(pathBuffer));

          // Set the payload.
          ImGui::SetDragDropPayload("ASSET_PATH", &pathBuffer, sizeof(pathBuffer));
          ImGui::EndDragDropSource();
        }

        ImGui::SetCursorPos(cursorPos);
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

        // Align the file text so the icons are evenly spaced out.
        unsigned int numChars = (unsigned int) (2 * 64.0f / fontSize);
        unsigned int stringPos = 0;
        float yPos = cursorPos.y + 64.0f + fontSize;
        unsigned int stride;
        for (unsigned int i = 0; i < (unsigned int) (textSize.x / 64.0f) + 1; i++)
        {
          ImGui::SetCursorPos(ImVec2(cursorPos.x, yPos));

          stride = ((stringPos + numChars) < filename.size()) ? numChars : filename.size() - stringPos;

          ImGui::Text(filename.substr(stringPos, stride).c_str());
          stringPos += numChars;
          yPos += fontSize;
        }

        // Handle the content potentially being offscreen for small
        // content browsers. Move offscreen content to a new line.
        maxCursorYPos = std::max(std::ceil((textSize.x / 64.0f) + 1.0f) * fontSize + cursorPos.y + 64.0f + fontSize, maxCursorYPos);
        ImVec2 windowSize = ImGui::GetWindowSize();
        if (windowSize.x - (64.0f + 8.0f) > cursorPos.x + 64.0f + 8.0f)
          ImGui::SetCursorPos(ImVec2(cursorPos.x + 64.0f + 8.0f, cursorPos.y));
        else
          ImGui::SetCursorPos(ImVec2(0.0f, maxCursorYPos));
      }
    }
  }
}
