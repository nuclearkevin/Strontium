#include "Core/Events.h"

// Project includes.
#include "Core/Application.h"

namespace Strontium
{
  //----------------------------------------------------------------------------
  // The generic event class which all events inherit from.
  //----------------------------------------------------------------------------
  Event::Event(const EventType &type, const std::string &eventName)
    : type(type)
    , name(eventName)
  { }

  //----------------------------------------------------------------------------
  // Key pressed event.
  //----------------------------------------------------------------------------
  KeyPressedEvent::KeyPressedEvent(const int keyCode, const GLuint repeat)
    : Event(EventType::KeyPressedEvent, "Key pressed event")
    , keyCode(keyCode)
    , numRepeat(repeat)
  { }

  //----------------------------------------------------------------------------
  // Key released event.
  //----------------------------------------------------------------------------
  KeyReleasedEvent::KeyReleasedEvent(const int keyCode)
    : Event(EventType::KeyReleasedEvent, "Key released event")
    , keyCode(keyCode)
  { }

  //----------------------------------------------------------------------------
  // Key typed event.
  //----------------------------------------------------------------------------
  KeyTypedEvent::KeyTypedEvent(const GLuint keyCode)
    : Event(EventType::KeyTypedEvent, "Key typed event")
    , keyCode(keyCode)
  { }

  //----------------------------------------------------------------------------
  // Mouse button click event.
  //----------------------------------------------------------------------------
  MouseClickEvent::MouseClickEvent(const int mouseCode)
    : Event(EventType::MouseClickEvent, "Mouse clicked event")
    , mouseCode(mouseCode)
  { }

  //----------------------------------------------------------------------------
  // Mouse button release event.
  //----------------------------------------------------------------------------
  MouseReleasedEvent::MouseReleasedEvent(const int mouseCode)
    : Event(EventType::MouseReleasedEvent, "Mouse released event")
    , mouseCode(mouseCode)
  { }

  //----------------------------------------------------------------------------
  // Mouse scroll event.
  //----------------------------------------------------------------------------
  MouseScrolledEvent::MouseScrolledEvent(const GLfloat xOffset,
                                         const GLfloat yOffset)
    : Event(EventType::MouseScrolledEvent, "Mouse scrolled event")
    , xOffset(xOffset)
    , yOffset(yOffset)
  { }

  //----------------------------------------------------------------------------
  // Window close event (tells the application to stop running).
  //----------------------------------------------------------------------------
  WindowCloseEvent::WindowCloseEvent()
    : Event(EventType::WindowCloseEvent, "Window close event")
  { }

  //----------------------------------------------------------------------------
  // Window resize event.
  //----------------------------------------------------------------------------
  WindowResizeEvent::WindowResizeEvent(GLuint width, GLuint height)
    : Event(EventType::WindowResizeEvent, "Window resize event")
    , width(width)
    , height(height)
  { }

  //----------------------------------------------------------------------------
  // Open file dialogue event.
  //----------------------------------------------------------------------------
  OpenDialogueEvent::OpenDialogueEvent(DialogueEventType type,
                                       const std::string &validFiles)
    : Event(EventType::OpenDialogueEvent, "Open dialogue event")
    , validFiles(validFiles)
    , dialogueType(type)
  { }

  //----------------------------------------------------------------------------
  // Load file event.
  //----------------------------------------------------------------------------
  LoadFileEvent::LoadFileEvent(const std::string &absPath,
                               const std::string &fileName)
    : Event(EventType::LoadFileEvent, "Load file event")
    , absPath(absPath)
    , fileName(fileName)
  { }

  //----------------------------------------------------------------------------
  // Save file event.
  //----------------------------------------------------------------------------
  SaveFileEvent::SaveFileEvent(const std::string &absPath,
                               const std::string &fileName)
   : Event(EventType::SaveFileEvent, "Save file event")
   , absPath(absPath)
   , fileName(fileName)
  { }

  //----------------------------------------------------------------------------
  // Gui event.
  //----------------------------------------------------------------------------
  GuiEvent::GuiEvent(GuiEventType type, const std::string &eventText)
    : Event(EventType::GuiEvent, "Gui event")
    , guiEventType(type)
    , eventText(eventText)
  { }

  //----------------------------------------------------------------------------
  // The entity swapped event.
  //----------------------------------------------------------------------------
  EntitySwapEvent::EntitySwapEvent(GLint entityID, Scene* entityParentScene)
    : Event(EventType::EntitySwapEvent, "Entity swap event")
    , storedEntity(entityID)
    , entityParentScene(entityParentScene)
  { }

  //----------------------------------------------------------------------------
  // The entity deleted event.
  //----------------------------------------------------------------------------
  EntityDeleteEvent::EntityDeleteEvent(GLint entityID, Scene* entityParentScene)
    : Event(EventType::EntityDeleteEvent, "Entity delete event")
    , storedEntity(entityID)
    , entityParentScene(entityParentScene)
  { }

  // A utility downcast function which deletes the event.
  void
  Event::deleteEvent(Event* &event)
  {
    if (event != nullptr)
    {
      switch (event->getType())
      {
        case EventType::KeyPressedEvent:
          delete static_cast<KeyPressedEvent*>(event);
          break;
        case EventType::KeyReleasedEvent:
          delete static_cast<KeyReleasedEvent*>(event);
          break;
        case EventType::KeyTypedEvent:
          delete static_cast<KeyTypedEvent*>(event);
          break;
        case EventType::MouseClickEvent:
          delete static_cast<MouseClickEvent*>(event);
          break;
        case EventType::MouseReleasedEvent:
          delete static_cast<MouseReleasedEvent*>(event);
          break;
        case EventType::MouseScrolledEvent:
          delete static_cast<MouseScrolledEvent*>(event);
          break;
        case EventType::WindowCloseEvent:
          delete static_cast<WindowCloseEvent*>(event);
          break;
        case EventType::WindowResizeEvent:
          delete static_cast<WindowResizeEvent*>(event);
          break;
        case EventType::OpenDialogueEvent:
          delete static_cast<OpenDialogueEvent*>(event);
          break;
        case EventType::LoadFileEvent:
          delete static_cast<LoadFileEvent*>(event);
          break;
        case EventType::SaveFileEvent:
          delete static_cast<SaveFileEvent*>(event);
          break;
        case EventType::GuiEvent:
          delete static_cast<GuiEvent*>(event);
          break;
        case EventType::EntitySwapEvent:
          delete static_cast<EntitySwapEvent*>(event);
          break;
      }
    }

    event = nullptr;
  }

  //----------------------------------------------------------------------------
  // Singleton event reciever and dispatcher.
  //----------------------------------------------------------------------------
  EventDispatcher* EventDispatcher::appEvents = nullptr;
  std::mutex EventDispatcher::dispatcherMutex;

  // Destructor deletes all remaining events.
  EventDispatcher::~EventDispatcher()
  {
    while (!this->eventQueue.empty())
    {
      Event* event = this->eventQueue.front();
      Event::deleteEvent(event);
      this->eventQueue.pop();
    }
  }

  EventDispatcher*
  EventDispatcher::getInstance()
  {
    std::lock_guard<std::mutex> guard(dispatcherMutex);

    if (appEvents == nullptr)
    {
      appEvents = new EventDispatcher();
      return appEvents;
    }
    else
      return appEvents;
  }

  void
  EventDispatcher::queueEvent(Event* toAdd)
  {
    std::lock_guard<std::mutex> guard(dispatcherMutex);

    this->eventQueue.push(toAdd);
  }

  Event*
  EventDispatcher::dequeueEvent()
  {
    std::lock_guard<std::mutex> guard(dispatcherMutex);

    Event* outEvent = this->eventQueue.front();
    this->eventQueue.pop();
    return outEvent;
  }
}
