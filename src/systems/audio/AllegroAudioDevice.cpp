#include "audio/AudioDevice.h"

#include "audio/Mixer.h"

#include <allegro.h>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace openfranko::src::systems::audio {
namespace {

constexpr int SAMPLE_BITS = 16;
constexpr int STREAM_VOLUME = 255;
constexpr int STREAM_PAN = 128;
constexpr int UNSCALED_VOICE_VOLUME = 0;
constexpr uint16_t UNSIGNED_SAMPLE_BIAS = 0x8000;

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Audio device error: " + cause + ": " +
                           allegro_error);
}

} // namespace

struct AudioDevice::Stream {
  Render render;
  int frames = 0;
  AUDIOSTREAM *stream = nullptr;
};

AudioDevice::AudioDevice(int rate, int frames, Render render)
    : m_stream(std::make_unique<Stream>()) {
  m_stream->render = std::move(render);
  m_stream->frames = frames;
  set_volume_per_voice(UNSCALED_VOICE_VOLUME);
  if (install_sound(DIGI_AUTODETECT, MIDI_NONE, nullptr) != 0) {
    throwError("Allegro sound");
  }
  m_stream->stream = play_audio_stream(frames, SAMPLE_BITS, TRUE, rate,
                                       STREAM_VOLUME, STREAM_PAN);
}

AudioDevice::~AudioDevice() {
  if (m_stream->stream) {
    stop_audio_stream(m_stream->stream);
  }
  remove_sound();
}

void AudioDevice::lock() {}

void AudioDevice::unlock() {}

void AudioDevice::update() {
  if (!m_stream->stream) {
    return;
  }
  const std::size_t samples =
      static_cast<std::size_t>(m_stream->frames) * Mixer::STEREO;
  while (void *buffer = get_audio_stream_buffer(m_stream->stream)) {
    m_stream->render(static_cast<int16_t *>(buffer), m_stream->frames);
    uint16_t *output = static_cast<uint16_t *>(buffer);
    for (std::size_t i = 0; i < samples; ++i) {
      output[i] ^= UNSIGNED_SAMPLE_BIAS;
    }
    free_audio_stream_buffer(m_stream->stream);
  }
}

} // namespace openfranko::src::systems::audio
