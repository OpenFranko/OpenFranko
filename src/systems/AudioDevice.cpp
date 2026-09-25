#include "AudioDevice.h"

#include <SDL2/SDL.h>
#include <stdexcept>
#include <string>
#include <utility>

namespace openfranko::src::systems {
namespace {

constexpr int STEREO = 2;
constexpr int FRAME_BYTES = STEREO * static_cast<int>(sizeof(int16_t));

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
    : stream(std::make_unique<Stream>()) {
  stream->render = std::move(render);
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    throwError("SDL");
  }

  SDL_AudioSpec wanted{};
  wanted.freq = rate;
  wanted.format = AUDIO_S16SYS;
  wanted.channels = STEREO;
  wanted.samples = static_cast<Uint16>(frames);
  wanted.callback = &Stream::fill;
  wanted.userdata = stream.get();
  stream->device = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
  if (stream->device == 0) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    throwError("OpenAudioDevice");
  }
  SDL_PauseAudioDevice(stream->device, 0);
}

AudioDevice::~AudioDevice() {
  SDL_CloseAudioDevice(stream->device);
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

} // namespace openfranko::src::systems
