#ifndef ENGINE_STREET_SCENES_STREETHOST_H_
#define ENGINE_STREET_SCENES_STREETHOST_H_

#include "../../effects/color/AmigaPalette.h"
#include "../core/Bobs.h"
#include "../core/EndingCredits.h"
#include "../core/IndexedSurface.h"
#include "../core/LevelScript.h"
#include "../session/GameSession.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class StreetHost {
public:
  static constexpr int LOADING_STRIP = 0;
  static constexpr int PANEL_ARTWORK = 1;

  class SpriteSetLoad {
  public:
    virtual ~SpriteSetLoad() = default;
    virtual bool step(core::ImageBank &images) = 0;
  };

  class FramesLoad {
  public:
    virtual ~FramesLoad() = default;
    virtual bool step(std::vector<core::Picture> &frames) = 0;
  };

  virtual ~StreetHost() = default;

  virtual std::unique_ptr<SpriteSetLoad>
  beginSpriteSet(int resource, int sampleBank, int base) {
    return std::make_unique<WholeSpriteSet>(*this, resource, sampleBank, base);
  }
  virtual std::unique_ptr<FramesLoad> beginScenery(int resource) {
    return std::make_unique<WholeScenery>(*this, resource);
  }

  virtual std::vector<core::Picture> loadSpriteSet(int resource,
                                                   int sampleBank) = 0;
  virtual core::Picture loadPicture(int resource) = 0;
  virtual effects::color::AmigaPalette loadPalette(int resource) = 0;
  virtual std::vector<core::Picture> loadScenery(int resource) = 0;
  virtual core::LevelScript loadLevelScript(int resource) = 0;
  virtual core::EndingCredits loadEndingCredits() = 0;
  virtual core::Picture loadPanelPicture(int part) = 0;
  virtual void loadMusic(int resource) = 0;
  virtual bool isMusicLoaded(int resource) const = 0;
  virtual void playMusic() = 0;
  virtual void stopMusic() = 0;
  virtual void setMusicVolume(int volume) = 0;
  virtual void setMusicTempo(int tempo) = 0;
  virtual void playSample(int bank, int sample, int voices) = 0;
  virtual void playSampleAt(int bank, int sample, int voices,
                            int frequency) = 0;
  virtual void setSampleLooping(bool loop) = 0;
  virtual int random(int limit) = 0;
  virtual void yield() = 0;

private:
  class WholeSpriteSet : public SpriteSetLoad {
  public:
    WholeSpriteSet(StreetHost &host, int resource, int sampleBank, int base)
        : m_host(host), m_resource(resource), m_sampleBank(sampleBank),
          m_base(base) {}

    bool step(core::ImageBank &images) override {
      images.load(m_base, m_host.loadSpriteSet(m_resource, m_sampleBank));
      return true;
    }

  private:
    StreetHost &m_host;
    int m_resource;
    int m_sampleBank;
    int m_base;
  };

  class WholeScenery : public FramesLoad {
  public:
    WholeScenery(StreetHost &host, int resource)
        : m_host(host), m_resource(resource) {}

    bool step(std::vector<core::Picture> &frames) override {
      frames = m_host.loadScenery(m_resource);
      return true;
    }

  private:
    StreetHost &m_host;
    int m_resource;
  };
};

struct StreetInput {
  int16_t joystick = 0;
  session::SystemKey key = session::SystemKey::None;
  bool mouseButton = false;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_STREETHOST_H_
