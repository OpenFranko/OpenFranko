#include "../../../../../src/engine/states/shared/StreetControls.h"

#include <catch2/catch_all.hpp>

#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states::shared;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::systems::input;

namespace {

KeyEvent keyEvent(Key key, bool pressed) {
  KeyEvent event;
  event.key = key;
  event.pressed = pressed;
  return event;
}

void tap(ControllerSystem &controller, Key key) {
  controller.receiveKey(keyEvent(key, true));
  controller.update();
}

} // namespace

SCENARIO("The joystick reaches the street as AMOS's Joy bits") {
  GIVEN("Up, right and fire held") {
    ControllerSystem controller;
    controller.states.up = true;
    controller.states.right = true;
    controller.states.button = true;

    THEN("Both versions see the same bits") {
      REQUIRE(readStreetInput(controller, GameVersion::V10).joystick ==
              (JOY_UP | JOY_RIGHT | JOY_FIRE));
      REQUIRE(readStreetInput(controller, GameVersion::V12).joystick ==
              (JOY_UP | JOY_RIGHT | JOY_FIRE));
    }
  }
}

SCENARIO("Version 1.0 reads the key pressed this frame") {
  GIVEN("The keys of the 1.0 street") {
    const std::vector<std::pair<Key, SystemKey>> keys = {
        {Key::F1, SystemKey::MusicOn},    {Key::F2, SystemKey::MusicOff},
        {Key::F3, SystemKey::Pal},        {Key::F4, SystemKey::Ntsc},
        {Key::Escape, SystemKey::Escape}, {Key::F9, SystemKey::Other},
        {Key::Delete, SystemKey::Other}};

    THEN("Each press is its system key for one frame") {
      for (const auto &[key, systemKey] : keys) {
        ControllerSystem controller;
        tap(controller, key);
        REQUIRE(readStreetInput(controller, GameVersion::V10).key == systemKey);
        controller.update();
        REQUIRE(readStreetInput(controller, GameVersion::V10).key ==
                SystemKey::None);
      }
    }
  }
}

SCENARIO("Version 1.2 reads the keys held down") {
  GIVEN("The keys of the 1.2 street") {
    const std::vector<std::pair<Key, SystemKey>> keys = {
        {Key::Escape, SystemKey::Escape}, {Key::F9, SystemKey::Lives},
        {Key::F1, SystemKey::Pal},        {Key::F2, SystemKey::Ntsc},
        {Key::F3, SystemKey::MusicOff},   {Key::F4, SystemKey::MusicOn}};

    THEN("Each key counts for as long as it is held") {
      for (const auto &[key, systemKey] : keys) {
        ControllerSystem controller;
        tap(controller, key);
        controller.update();
        REQUIRE(readStreetInput(controller, GameVersion::V12).key == systemKey);
        controller.receiveKey(keyEvent(key, false));
        controller.update();
        REQUIRE(readStreetInput(controller, GameVersion::V12).key ==
                SystemKey::None);
      }
    }

    THEN("The mouse button is passed on") {
      ControllerSystem controller;
      controller.receiveMouseButton(true);
      controller.update();
      REQUIRE(readStreetInput(controller, GameVersion::V12).mouseButton);
      REQUIRE_FALSE(readStreetInput(controller, GameVersion::V10).mouseButton);
    }
  }
}
