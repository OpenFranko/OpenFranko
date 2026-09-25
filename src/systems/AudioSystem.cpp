#include "AudioSystem.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace openfranko::src::systems {
namespace {

constexpr int MAX_AMOS_VOLUME = 64;
constexpr int DEFAULT_MUSIC_VOLUME = 56;
constexpr int DEFAULT_SAMPLE_VOLUME = 56;
constexpr int VOICES = 4;
constexpr uint8_t FULL_PAN = 255;

constexpr int PAL_VBL_RATE = 50;
constexpr int MODULE_CHANNELS = 2;
constexpr std::size_t RENDER_BYTES = 2048;
constexpr double LED_FILTER_HERTZ = 3275.0;
constexpr double BUTTERWORTH_Q = 0.7071067811865476;
constexpr double PI = 3.14159265358979323846;

bool isLeftVoice(int voice) { return voice == 0 || voice == 3; }

void throwError(const std::string &cause) {
  throw std::runtime_error("Audio system could not be initialised!: " + cause);
}

int16_t clampSample(double value) {
  return static_cast<int16_t>(
      std::clamp(std::lround(value),
                 static_cast<long>(std::numeric_limits<int16_t>::min()),
                 static_cast<long>(std::numeric_limits<int16_t>::max())));
}

} // namespace

AudioSystem::AudioSystem() : musicVolume(DEFAULT_MUSIC_VOLUME) {
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    throwError("SDL");
  }

  if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
    throwError("OpenAudio");
  }

  if (Mix_QuerySpec(&deviceRate, &deviceFormat, &deviceChannels) == 0) {
    throwError("QuerySpec");
  }

  player = xmp_create_context();
  if (!player) {
    throwError("xmp");
  }
  musicStream = SDL_NewAudioStream(AUDIO_S16SYS, MODULE_CHANNELS, deviceRate,
                                   deviceFormat, deviceChannels, deviceRate);
  if (!musicStream) {
    throwError("AudioStream");
  }
  renderBuffer.resize(RENDER_BYTES);
  vblRate = PAL_VBL_RATE;

  const double w0 = 2.0 * PI * LED_FILTER_HERTZ / deviceRate;
  const double alpha = std::sin(w0) / (2.0 * BUTTERWORTH_Q);
  const double cosine = std::cos(w0);
  const double a0 = 1.0 + alpha;
  lowPass = {(1.0 - cosine) / 2.0 / a0, (1.0 - cosine) / a0,
             (1.0 - cosine) / 2.0 / a0, -2.0 * cosine / a0, (1.0 - alpha) / a0};
  filterHistory.assign(static_cast<std::size_t>(std::max(deviceChannels, 1)),
                       {});

  Mix_ReserveChannels(VOICES);
  Mix_Volume(-1, DEFAULT_SAMPLE_VOLUME * MIX_MAX_VOLUME / MAX_AMOS_VOLUME);
  Mix_HookMusic(mixMusic, this);
  Mix_SetPostMix(filterOutput, this);

  applyMusicVolume();
}

AudioSystem::~AudioSystem() {
  Mix_HookMusic(nullptr, nullptr);
  Mix_SetPostMix(nullptr, nullptr);
  for (auto &pitched : pitchedChunks) {
    Mix_FreeChunk(pitched.second.chunk);
  }
  for (auto &soundEffect : soundEffects) {
    Mix_FreeChunk(soundEffect.second);
  }
  clearMusic();
  xmp_free_context(player);
  SDL_FreeAudioStream(musicStream);
  Mix_Quit();
  Mix_CloseAudio();
}

void AudioSystem::loadMusic(const std::string &path) {
  clearMusic();
  std::ifstream file(path, std::ios::binary);
  const std::vector<char> module{std::istreambuf_iterator<char>(file),
                                 std::istreambuf_iterator<char>()};
  if (module.empty()) {
    return;
  }
  std::lock_guard<std::mutex> lock(musicMutex);
  moduleLoaded =
      xmp_load_module_from_memory(player, module.data(),
                                  static_cast<long>(module.size())) == 0;
  if (moduleLoaded) {
    musicPath = path;
  }
}

void AudioSystem::clearMusic() {
  std::lock_guard<std::mutex> lock(musicMutex);
  stopPlayer();
  if (moduleLoaded) {
    xmp_release_module(player);
    moduleLoaded = false;
  }
  musicPath.clear();
}

const std::string &AudioSystem::loadedMusic() const { return musicPath; }

void AudioSystem::loadSFX(const std::string &name, const std::string &path) {
  clearSFX(name);
  auto soundEffect = Mix_LoadWAV(path.c_str());
  if (!soundEffect) {
    return;
  }
  soundEffects.emplace(name, soundEffect);
  SDL_AudioSpec spec;
  Uint8 *buffer = nullptr;
  Uint32 length = 0;
  if (SDL_LoadWAV(path.c_str(), &spec, &buffer, &length)) {
    nativeRates[name] = spec.freq;
    SDL_FreeWAV(buffer);
  }
}

void AudioSystem::clearSFX(const std::string &name) {
  if (soundEffects.find(name) == soundEffects.end()) {
    return;
  }
  clearPitchedChunks(name);
  nativeRates.erase(name);

  auto &sfx = soundEffects.at(name);
  if (sfx == silencingSample) {
    silencingSample = nullptr;
    applyMusicVolume();
  }
  Mix_FreeChunk(sfx);
  soundEffects.erase(name);
}

void AudioSystem::playMusic() {
  {
    std::lock_guard<std::mutex> lock(musicMutex);
    stopPlayer();
    tempoScale = 1.0;
    if (moduleLoaded && xmp_start_player(player, deviceRate, 0) == 0) {
      musicPlaying = true;
      applyTempo();
    }
  }
  applyMusicVolume();
}

void AudioSystem::stopMusic() {
  std::lock_guard<std::mutex> lock(musicMutex);
  stopPlayer();
}

void AudioSystem::setMusicVolume(int volume) {
  musicVolume = volume;
  applyMusicVolume();
}

void AudioSystem::setMusicTempoScale(double scale) {
  std::lock_guard<std::mutex> lock(musicMutex);
  tempoScale = scale;
  applyTempo();
}

void AudioSystem::setVblRate(int hertz) {
  std::lock_guard<std::mutex> lock(musicMutex);
  if (hertz == vblRate) {
    return;
  }
  vblRate = hertz;
  applyTempo();
}

void AudioSystem::setLowPassFilter(bool on) { filterOn = on; }

void SDLCALL AudioSystem::mixMusic(void *system, Uint8 *stream, int length) {
  AudioSystem &audio = *static_cast<AudioSystem *>(system);
  std::lock_guard<std::mutex> lock(audio.musicMutex);
  if (!audio.musicPlaying) {
    return;
  }
  const auto rendered = static_cast<int>(audio.renderBuffer.size());
  while (SDL_AudioStreamAvailable(audio.musicStream) < length) {
    if (xmp_play_buffer(audio.player, audio.renderBuffer.data(), rendered, 0) <
            0 ||
        SDL_AudioStreamPut(audio.musicStream, audio.renderBuffer.data(),
                           rendered) < 0) {
      break;
    }
  }
  audio.mixBuffer.resize(static_cast<std::size_t>(length));
  const int got =
      SDL_AudioStreamGet(audio.musicStream, audio.mixBuffer.data(), length);
  if (got > 0) {
    SDL_MixAudioFormat(stream, audio.mixBuffer.data(), audio.deviceFormat,
                       static_cast<Uint32>(got), audio.mixerVolume.load());
  }
}

void SDLCALL AudioSystem::filterOutput(void *system, Uint8 *stream,
                                       int length) {
  AudioSystem &audio = *static_cast<AudioSystem *>(system);
  if (audio.deviceFormat != AUDIO_S16SYS) {
    return;
  }
  const bool on = audio.filterOn.load();
  const Biquad &filter = audio.lowPass;
  const std::size_t channels = audio.filterHistory.size();
  const std::size_t count = static_cast<std::size_t>(length) / sizeof(int16_t);
  for (std::size_t i = 0; i < count; ++i) {
    int16_t sample = 0;
    std::memcpy(&sample, stream + i * sizeof(int16_t), sizeof(int16_t));
    std::array<double, 4> &history = audio.filterHistory[i % channels];
    const double input = sample;
    const double output = filter.b0 * input + filter.b1 * history[0] +
                          filter.b2 * history[1] - filter.a1 * history[2] -
                          filter.a2 * history[3];
    history = {input, history[0], output, history[2]};
    if (on) {
      const int16_t filtered = clampSample(output);
      std::memcpy(stream + i * sizeof(int16_t), &filtered, sizeof(int16_t));
    }
  }
}

void AudioSystem::stopPlayer() {
  if (musicPlaying) {
    xmp_end_player(player);
    musicPlaying = false;
  }
  SDL_AudioStreamClear(musicStream);
}

void AudioSystem::applyTempo() {
  if (musicPlaying) {
    xmp_set_tempo_factor(player, static_cast<double>(PAL_VBL_RATE) /
                                     (vblRate * tempoScale));
  }
}

void AudioSystem::playSample(const std::string &name, int voiceMask) {
  auto it = soundEffects.find(name);
  if (it == soundEffects.end()) {
    return;
  }
  playChunk(it->second, voiceMask);
}

void AudioSystem::playSampleAt(const std::string &name, int voiceMask,
                               int frequency) {
  Mix_Chunk *chunk = pitchedChunk(name, frequency);
  if (chunk) {
    playChunk(chunk, voiceMask);
  }
}

void AudioSystem::playChunk(Mix_Chunk *chunk, int voiceMask) {
  for (int voice = 0; voice < VOICES; ++voice) {
    if (voiceMask & (1 << voice)) {
      Mix_HaltChannel(voice);
      Mix_SetPanning(voice, isLeftVoice(voice) ? FULL_PAN : 0,
                     isLeftVoice(voice) ? 0 : FULL_PAN);
      Mix_PlayChannel(voice, chunk, sampleLooping ? -1 : 0);
      voiceStarted[static_cast<std::size_t>(voice)] = SDL_GetTicks();
      loopingVoices[static_cast<std::size_t>(voice)] =
          sampleLooping ? chunk : nullptr;
    }
  }
  if ((voiceMask & ALL_VOICES) == ALL_VOICES) {
    silencingSample = chunk;
    applyMusicVolume();
  }
}

Mix_Chunk *AudioSystem::pitchedChunk(const std::string &name, int frequency) {
  const auto key = std::make_pair(name, frequency);
  auto found = pitchedChunks.find(key);
  if (found != pitchedChunks.end()) {
    return found->second.chunk;
  }
  auto source = soundEffects.find(name);
  auto rate = nativeRates.find(name);
  if (source == soundEffects.end() || rate == nativeRates.end() ||
      frequency <= 0 || rate->second <= 0) {
    return nullptr;
  }
  int deviceRate = 0;
  Uint16 format = 0;
  int channels = 0;
  if (Mix_QuerySpec(&deviceRate, &format, &channels) == 0) {
    return nullptr;
  }
  const std::size_t frameBytes =
      static_cast<std::size_t>(SDL_AUDIO_BITSIZE(format) / 8 * channels);
  const std::size_t frames = source->second->alen / frameBytes;
  const auto native = static_cast<std::size_t>(rate->second);
  const auto wanted = static_cast<std::size_t>(frequency);
  const std::size_t pitchedFrames = frames * native / wanted;
  if (pitchedFrames == 0) {
    return nullptr;
  }
  PitchedChunk pitched;
  pitched.data.resize(pitchedFrames * frameBytes);
  for (std::size_t frame = 0; frame < pitchedFrames; ++frame) {
    const std::size_t from = frame * wanted / native;
    std::memcpy(pitched.data.data() + frame * frameBytes,
                source->second->abuf + from * frameBytes, frameBytes);
  }
  pitched.chunk = Mix_QuickLoad_RAW(pitched.data.data(),
                                    static_cast<Uint32>(pitched.data.size()));
  if (!pitched.chunk) {
    return nullptr;
  }
  Mix_Chunk *chunk = pitched.chunk;
  pitchedChunks.emplace(key, std::move(pitched));
  return chunk;
}

void AudioSystem::clearPitchedChunks(const std::string &name) {
  for (auto it = pitchedChunks.begin(); it != pitchedChunks.end();) {
    if (it->first.first == name) {
      if (it->second.chunk == silencingSample) {
        silencingSample = nullptr;
        applyMusicVolume();
      }
      Mix_FreeChunk(it->second.chunk);
      it = pitchedChunks.erase(it);
    } else {
      ++it;
    }
  }
}

void AudioSystem::stopSFX() { Mix_HaltChannel(-1); }

void AudioSystem::setSampleLooping(bool looping) {
  sampleLooping = looping;
  if (looping) {
    return;
  }
  for (int voice = 0; voice < VOICES; ++voice) {
    const auto index = static_cast<std::size_t>(voice);
    const Mix_Chunk *chunk = loopingVoices[index];
    loopingVoices[index] = nullptr;
    if (!chunk || !Mix_Playing(voice) || Mix_GetChunk(voice) != chunk) {
      continue;
    }
    const uint32_t length = chunkMilliseconds(chunk);
    if (length == 0) {
      Mix_HaltChannel(voice);
      continue;
    }
    const uint32_t played = (SDL_GetTicks() - voiceStarted[index]) % length;
    Mix_ExpireChannel(voice, static_cast<int>(length - played));
  }
}

void AudioSystem::update() {
  if (silencingSample && !isVoicePlaying(silencingSample)) {
    silencingSample = nullptr;
    applyMusicVolume();
  }
}

void AudioSystem::applyMusicVolume() {
  mixerVolume =
      silencingSample ? 0 : musicVolume * MIX_MAX_VOLUME / MAX_AMOS_VOLUME;
}

uint32_t AudioSystem::chunkMilliseconds(const Mix_Chunk *chunk) const {
  int frequency = 0;
  Uint16 format = 0;
  int channels = 0;
  if (Mix_QuerySpec(&frequency, &format, &channels) == 0) {
    return 0;
  }
  const uint64_t bytesPerSecond = static_cast<uint64_t>(frequency) *
                                  static_cast<uint64_t>(channels) *
                                  (SDL_AUDIO_BITSIZE(format) / 8);
  if (bytesPerSecond == 0) {
    return 0;
  }
  return static_cast<uint32_t>(static_cast<uint64_t>(chunk->alen) * 1000 /
                               bytesPerSecond);
}

bool AudioSystem::isVoicePlaying(const Mix_Chunk *chunk) const {
  for (int voice = 0; voice < VOICES; ++voice) {
    if (Mix_Playing(voice) && Mix_GetChunk(voice) == chunk) {
      return true;
    }
  }
  return false;
}

} // namespace openfranko::src::systems