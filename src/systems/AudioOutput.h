#ifndef SYSTEMS_AUDIOOUTPUT_H_
#define SYSTEMS_AUDIOOUTPUT_H_

#include "AudioDevice.h"
#include "AudioSystem.h"
#include "Mixer.h"
#include "Wave.h"

#include <map>
#include <memory>
#include <string>

namespace openfranko {
namespace src {
namespace systems {

struct AudioSystem::Output {
  explicit Output(int rate) : mixer(rate) {}

  Mixer mixer;
  std::map<std::string, std::unique_ptr<Sound>> sounds;
  std::string musicPath;
  double tempoScale = 1.0;
  int vblRate = 0;
  bool sampleLooping = false;
  std::unique_ptr<AudioDevice> device;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIOOUTPUT_H_
