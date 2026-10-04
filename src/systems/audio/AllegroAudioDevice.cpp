#include "audio/AudioDevice.h"

#include "audio/Mixer.h"

#include <algorithm>
#include <allegro.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <dpmi.h>
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
constexpr int LEAD_FRAMES = 1024;
constexpr int RING_LEADS = 2;
constexpr int RESCUE_HERTZ = 100;
constexpr int RESCUE_PART = 2;
constexpr std::size_t FPU_STATE_BYTES = 108;
constexpr unsigned long SPARE_MEMORY = 6UL << 20;
constexpr uint16_t UNSIGNED_SAMPLE_BIAS = 0x8000;
constexpr auto NO_SOUND_CARD_MESSAGE =
    "No sound card found, the game will be silent.\n";

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Audio device error: " + cause + ": " +
                           allegro_error);
}

bool hasSpareMemory() {
  const unsigned long physical = _go32_dpmi_remaining_physical_memory();
  return physical >= SPARE_MEMORY &&
         physical < _go32_dpmi_remaining_virtual_memory();
}

} // namespace

struct AudioDevice::Stream {
  ~Stream() {
    remove_param_int(rescue, this);
    if (voice >= 0) {
      voice_stop(voice);
      deallocate_voice(voice);
    }
    if (ring) {
      destroy_sample(ring);
    }
  }

  static void rescue(void *stream);
  void refill(int least);

  Render render;
  int lead = 0;
  SAMPLE *ring = nullptr;
  int voice = -1;
  int written = 0;
  int played = 0;
  int queued = 0;
  std::atomic<int> locks{0};
};

void AudioDevice::Stream::rescue(void *stream) {
  Stream &playing = *static_cast<Stream *>(stream);
  if (playing.locks != 0) {
    return;
  }
  std::array<uint8_t, FPU_STATE_BYTES> fpu;
  asm volatile("fnsave %0" : "=m"(fpu) : : "memory");
  playing.refill(playing.lead / RESCUE_PART);
  asm volatile("frstor %0" : : "m"(fpu) : "memory");
}

void AudioDevice::Stream::refill(int least) {
  const int position = voice_get_position(voice);
  if (position < 0) {
    return;
  }
  const int length = ring->len;
  const int consumed = (position - played + length) % length;
  played = position;
  if (consumed >= queued) {
    written = position;
    queued = 0;
  } else {
    queued -= consumed;
  }
  if (queued >= least) {
    return;
  }
  int16_t *data = static_cast<int16_t *>(ring->data);
  while (queued < lead) {
    const int frames = std::min(lead - queued, length - written);
    int16_t *out = data + static_cast<std::ptrdiff_t>(written) * Mixer::STEREO;
    render(out, frames);
    uint16_t *samples = reinterpret_cast<uint16_t *>(out);
    std::for_each(samples, samples + frames * Mixer::STEREO,
                  [](uint16_t &sample) { sample ^= UNSIGNED_SAMPLE_BIAS; });
    written = (written + frames) % length;
    queued += frames;
  }
}

AudioDevice::AudioDevice(int rate, Render render)
    : m_stream(std::make_unique<Stream>()) {
  m_stream->render = std::move(render);
  m_stream->lead = LEAD_FRAMES;
  set_volume_per_voice(UNSCALED_VOICE_VOLUME);
  set_mixer_quality(PLAIN_16_BIT_MIXING);
  if (install_sound(DIGI_AUTODETECT, MIDI_NONE, nullptr) != 0) {
    throwError("Allegro sound");
  }
  if (digi_card == DIGI_NONE) {
    std::fputs(NO_SOUND_CARD_MESSAGE, stderr);
  }
  SAMPLE *ring =
      create_sample(SAMPLE_BITS, TRUE, rate, RING_LEADS * LEAD_FRAMES);
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
  update();
  if (hasSpareMemory()) {
    install_param_int_ex(Stream::rescue, m_stream.get(),
                         BPS_TO_TIMER(RESCUE_HERTZ));
  }
}

AudioDevice::~AudioDevice() {
  m_stream.reset();
  remove_sound();
}

void AudioDevice::lock() { ++m_stream->locks; }

void AudioDevice::unlock() { --m_stream->locks; }

bool AudioDevice::interpolatesMusic() { return false; }

void AudioDevice::update() {
  Stream &stream = *m_stream;
  if (stream.voice < 0) {
    return;
  }
  ++stream.locks;
  stream.refill(stream.lead);
  --stream.locks;
}

} // namespace openfranko::src::systems::audio
