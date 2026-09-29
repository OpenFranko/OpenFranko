#include "audio/AudioDevice.h"

#include "audio/Mixer.h"

#include <SDL2/SDL.h>
#include <stdexcept>
#include <string>
#include <utility>

namespace openfranko::src::systems::audio {
namespace {

constexpr int FRAME_BYTES = Mixer::STEREO * static_cast<int>(sizeof(int16_t));

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Audio device error: " + cause + ": " +
                           SDL_GetError());
}

} // namespace

struct AudioDevice::Stream {
  static void SDLCALL fill(void *stream, Uint8 *bytes, int length) {
    static_cast<Stream *>(stream)->render(reinterpret_cast<int16_t *>(bytes),
                                          length / FRAME_BYTES);
  }

  Render render;
  SDL_AudioDeviceID device = 0;
};

AudioDevice::AudioDevice(int rate, int frames, Render render)
    : m_stream(std::make_unique<Stream>()) {
  m_stream->render = std::move(render);
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    throwError("SDL");
  }

  SDL_AudioSpec wanted{};
  wanted.freq = rate;
  wanted.format = AUDIO_S16SYS;
  wanted.channels = Mixer::STEREO;
  wanted.samples = static_cast<Uint16>(frames);
  wanted.callback = &Stream::fill;
  wanted.userdata = m_stream.get();
  m_stream->device = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
  if (m_stream->device == 0) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    throwError("OpenAudioDevice");
  }
  SDL_PauseAudioDevice(m_stream->device, 0);
}

AudioDevice::~AudioDevice() {
  SDL_CloseAudioDevice(m_stream->device);
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

} // namespace openfranko::src::systems::audio
