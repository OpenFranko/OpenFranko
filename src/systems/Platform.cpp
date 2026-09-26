#include "Platform.h"

#include <SDL2/SDL.h>
#include <stdexcept>
#include <string>

namespace openfranko::src::systems {
namespace {

constexpr SDL_Keycode FIRST_PRINTABLE = '!';
constexpr SDL_Keycode LAST_PRINTABLE = '~';

Key toKey(SDL_Scancode scancode) {
  switch (scancode) {
  case SDL_SCANCODE_UP:
    return Key::Up;
  case SDL_SCANCODE_DOWN:
    return Key::Down;
  case SDL_SCANCODE_LEFT:
    return Key::Left;
  case SDL_SCANCODE_RIGHT:
    return Key::Right;
  case SDL_SCANCODE_SPACE:
    return Key::Space;
  case SDL_SCANCODE_W:
    return Key::W;
  case SDL_SCANCODE_A:
    return Key::A;
  case SDL_SCANCODE_S:
    return Key::S;
  case SDL_SCANCODE_D:
    return Key::D;
  case SDL_SCANCODE_DELETE:
    return Key::Delete;
  case SDL_SCANCODE_F1:
    return Key::F1;
  case SDL_SCANCODE_F2:
    return Key::F2;
  case SDL_SCANCODE_F3:
    return Key::F3;
  case SDL_SCANCODE_F4:
    return Key::F4;
  case SDL_SCANCODE_F9:
    return Key::F9;
  case SDL_SCANCODE_ESCAPE:
    return Key::Escape;
  default:
    return Key::Other;
  }
}

char typedCharacter(SDL_Keycode keycode) {
  switch (keycode) {
  case SDLK_BACKSPACE:
    return '\b';
  case SDLK_RETURN:
  case SDLK_KP_ENTER:
    return '\r';
  case SDLK_SPACE:
    return ' ';
  default:
    break;
  }
  if (keycode >= FIRST_PRINTABLE && keycode <= LAST_PRINTABLE) {
    return static_cast<char>(keycode);
  }
  return 0;
}

KeyEvent toKeyEvent(const SDL_KeyboardEvent &key) {
  KeyEvent event;
  event.key = toKey(key.keysym.scancode);
  event.code = key.keysym.scancode;
  event.character = typedCharacter(key.keysym.sym);
  event.pressed = key.type == SDL_KEYDOWN;
  event.repeat = key.repeat != 0;
  return event;
}

} // namespace

Platform::Platform() {
  if (SDL_Init(SDL_INIT_EVENTS) < 0) {
    throw std::runtime_error(std::string("Platform error: ") + SDL_GetError());
  }
}

Platform::~Platform() { SDL_Quit(); }

bool Platform::pollEvents(ControllerSystem &controller) {
  bool open = true;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      open = false;
      break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      controller.receiveKey(toKeyEvent(event.key));
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT) {
        controller.receiveMouseButton(event.type == SDL_MOUSEBUTTONDOWN);
      }
      break;
    default:
      break;
    }
  }
  return open;
}

} // namespace openfranko::src::systems
