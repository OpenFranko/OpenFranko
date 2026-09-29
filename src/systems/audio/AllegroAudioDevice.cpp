#include "audio/AudioDevice.h"

#include "audio/Mixer.h"

#include <algorithm>
#include <allegro.h>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace openfranko::src::systems::audio {
namespace {

constexpr int SAMPLE_BITS = 16;
constexpr int VOICE_VOLUME = 255;
constexpr int VOICE_PAN = 128;
constexpr int UNSCALED_VOICE_VOLUME = 0;
constexpr int PLAIN_16_BIT_MIXING = 1;
constexpr int RING_LEADS = 2;
constexpr uint16_t UNSIGNED_SAMPLE_BIAS = 0x8000;

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Audio device error: " + cause + ": " +
                           allegro_error);
}

} // namespace

struct AudioDevice::Stream {
  ~Stream() {
    if (voice >= 0) {
      voice_stop(voice);
      deallocate_voice(voice);
    }
    if (ring) {
      destroy_sample(ring);
    }
  }

  Render render;
  int lead = 0;
  SAMPLE *ring = nullptr;
  int voice = -1;
  int written = 0;
  int played = 0;
  int queued = 0;
};

AudioDevice::AudioDevice(int rate, int frames, Render render)
    : m_stream(std::make_unique<Stream>()) {
  m_stream->render = std::move(render);
  m_stream->lead = frames;
  set_volume_per_voice(UNSCALED_VOICE_VOLUME);
  set_mixer_quality(PLAIN_16_BIT_MIXING);
  if (install_sound(DIGI_AUTODETECT, MIDI_NONE, nullptr) != 0) {
    throwError("Allegro sound");
  }
  SAMPLE *ring = create_sample(SAMPLE_BITS, TRUE, rate, RING_LEADS * frames);
  if (!ring) {
    return;
  }
  m_stream->ring = ring;
  uint16_t *data = static_cast<uint16_t *>(ring->data);
  std::fill(data, data + ring->len * Mixer::STEREO, UNSIGNED_SAMPLE_BIAS);
  m_stream->voice = allocate_voice(ring);
  if (m_stream->voice < 0) {
    return;
  }
  voice_set_playmode(m_stream->voice, PLAYMODE_LOOP);
  voice_set_volume(m_stream->voice, VOICE_VOLUME);
  voice_set_pan(m_stream->voice, VOICE_PAN);
  voice_start(m_stream->voice);
}

AudioDevice::~AudioDevice() {
  m_stream.reset();
  remove_sound();
}

void AudioDevice::lock() {}

void AudioDevice::unlock() {}

void AudioDevice::update() {
  Stream &stream = *m_stream;
  if (stream.voice < 0) {
    return;
  }
  const int position = voice_get_position(stream.voice);
  if (position < 0) {
    return;
  }
  const int length = stream.ring->len;
  const int consumed = (position - stream.played + length) % length;
  stream.played = position;
  if (consumed >= stream.queued) {
    stream.written = position;
    stream.queued = 0;
  } else {
    stream.queued -= consumed;
  }
  int16_t *data = static_cast<int16_t *>(stream.ring->data);
  while (stream.queued < stream.lead) {
    const int frames =
        std::min(stream.lead - stream.queued, length - stream.written);
    int16_t *out =
        data + static_cast<std::ptrdiff_t>(stream.written) * Mixer::STEREO;
    stream.render(out, frames);
    uint16_t *samples = reinterpret_cast<uint16_t *>(out);
    std::for_each(samples, samples + frames * Mixer::STEREO,
                  [](uint16_t &sample) { sample ^= UNSIGNED_SAMPLE_BIAS; });
    stream.written = (stream.written + frames) % length;
    stream.queued += frames;
  }
}

} // namespace openfranko::src::systems::audio
