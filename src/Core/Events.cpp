#include "Core/Events.h"

// Project includes.
#include "Core/Application.h"

namespace SciRenderer
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
      }
    }
    event = nullptr;
  }

  //----------------------------------------------------------------------------
  // Singleton event reciever and dispatcher.
  //----------------------------------------------------------------------------
  EventDispatcher* EventDispatcher::appEvents = nullptr;

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
    this->eventQueue.push(toAdd);
  }

  Event*
  EventDispatcher::dequeueEvent()
  {
    Event* outEvent = this->eventQueue.front();
    this->eventQueue.pop();
    return outEvent;
  }
}
