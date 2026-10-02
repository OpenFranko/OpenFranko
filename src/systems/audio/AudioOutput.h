#ifndef SYSTEMS_AUDIO_AUDIOOUTPUT_H_
#define SYSTEMS_AUDIO_AUDIOOUTPUT_H_

#include "audio/AudioDevice.h"
#include "audio/AudioSystem.h"
#include "audio/Mixer.h"
#include "audio/Wave.h"

#include <map>
#include <memory>
#include <string>

namespace openfranko {
namespace src {
namespace systems {
namespace audio {

struct AudioSystem::Output {
  explicit Output(int rate) : mixer(rate) {}

  Mixer mixer;
  AudioSystem::Read read;
  std::map<std::string, std::unique_ptr<Sound>> sounds;
  std::string musicPath;
  double tempoScale = 1.0;
  bool sampleLooping = false;
  std::unique_ptr<AudioDevice> device;
};

} // namespace audio
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIO_AUDIOOUTPUT_H_
