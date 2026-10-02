#include "EngineStreetHost.h"

#include "../../../systems/graphics/Bitmap.h"
#include "../../MenuTempo.h"
#include "../../assets/Assets.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace openfranko::src::engine::states::shared {
namespace {

constexpr int PANEL_RESOURCE = 0x384;
constexpr auto CREDITS_FILE = "credits.json";

street::core::Picture toPicture(systems::graphics::IndexedBitmap bitmap) {
  street::core::Picture picture;
  picture.width = bitmap.width;
  picture.height = bitmap.height;
  picture.hotX = bitmap.hotspotX;
  picture.hotY = bitmap.hotspotY;
  picture.pixels = std::move(bitmap.pixels);
  return picture;
}

constexpr int DECIMAL_BASE = 10;

std::optional<int> numberAfter(std::string_view text, std::string_view prefix,
                               std::string_view suffix) {
  if (text.size() <= prefix.size() + suffix.size() ||
      text.compare(0, prefix.size(), prefix) != 0) {
    return std::nullopt;
  }
  std::size_t end = prefix.size();
  int number = 0;
  while (end < text.size() &&
         std::isdigit(static_cast<unsigned char>(text[end]))) {
    number = number * DECIMAL_BASE + (text[end] - '0');
    ++end;
  }
  if (end == prefix.size() || text.compare(end, suffix.size(), suffix) != 0) {
    return std::nullopt;
  }
  return number;
}

std::string_view fileName(std::string_view path) {
  std::size_t start = path.size();
  while (start > 0 && path[start - 1] != '/' && path[start - 1] != '\\') {
    --start;
  }
  return path.substr(start);
}

bool endsWith(std::string_view text, std::string_view suffix) {
  return text.size() >= suffix.size() &&
         text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string readText(assets::Files &files, const std::string &what,
                     const std::string &path) {
  if (!files.exists(path)) {
    throw std::runtime_error("Failed to open " + what + ": " + path);
  }
  const std::vector<uint8_t> text = files.read(path);
  return std::string(reinterpret_cast<const char *>(text.data()), text.size());
}

} // namespace

EngineStreetHost::EngineStreetHost(systems::audio::Speaker &speaker,
                                   assets::Files &files, GameVersion version,
                                   MersenneTwister &random,
                                   std::function<void()> yield,
                                   std::string directory)
    : m_speaker(speaker), m_files(files), m_version(version),
      m_yield(std::move(yield)), m_directory(std::move(directory)),
      m_random(random) {}

EngineStreetHost::~EngineStreetHost() {
  m_speaker.setSampleLooping(false);
  for (const auto &entry : m_samples) {
    for (int sample : entry.second) {
      m_speaker.clearSample(sampleName(entry.first, sample));
    }
  }
}

class EngineStreetHost::SpriteSetSteps : public SpriteSetLoad {
public:
  SpriteSetSteps(EngineStreetHost &host, int resource, int sampleBank, int base)
      : m_host(host), m_resource(resource), m_sampleBank(sampleBank),
        m_base(base) {}

  bool step(street::core::ImageBank &images) override {
    if (!m_started) {
      m_started = true;
      m_host.listSpriteSet(m_resource, m_frames, m_samples);
      if (m_sampleBank == 0) {
        m_samples.clear();
      } else {
        m_host.clearSamples(m_sampleBank);
      }
      return false;
    }
    if (m_nextFrame < m_frames.size()) {
      const std::size_t end =
          std::min(m_nextFrame + FRAMES_PER_STEP, m_frames.size());
      for (; m_nextFrame < end; ++m_nextFrame) {
        const auto &[index, path] = m_frames[m_nextFrame];
        images.load(m_base + index, m_host.loadFrame(path));
      }
      if (m_nextFrame < m_frames.size()) {
        return false;
      }
      clearGaps(images);
      return m_samples.empty();
    }
    if (m_nextSample < m_samples.size()) {
      const auto &[sample, path] = m_samples[m_nextSample++];
      m_host.loadSample(m_sampleBank, sample, path);
    }
    return m_nextSample >= m_samples.size();
  }

private:
  static constexpr std::size_t FRAMES_PER_STEP = 2;

  void clearGaps(street::core::ImageBank &images) const {
    int count = 0;
    for (const auto &frame : m_frames) {
      count = std::max(count, frame.first + 1);
    }
    std::vector<bool> present(static_cast<std::size_t>(count), false);
    for (const auto &frame : m_frames) {
      present[static_cast<std::size_t>(frame.first)] = true;
    }
    for (int index = 0; index < count; ++index) {
      if (!present[static_cast<std::size_t>(index)]) {
        images.load(m_base + index, street::core::Picture{});
      }
    }
  }

  EngineStreetHost &m_host;
  int m_resource;
  int m_sampleBank;
  int m_base;
  NumberedPaths m_frames;
  NumberedPaths m_samples;
  std::size_t m_nextFrame = 0;
  std::size_t m_nextSample = 0;
  bool m_started = false;
};

std::unique_ptr<street::scenes::StreetHost::SpriteSetLoad>
EngineStreetHost::beginSpriteSet(int resource, int sampleBank, int base) {
  return std::make_unique<SpriteSetSteps>(*this, resource, sampleBank, base);
}

class EngineStreetHost::ScenerySteps : public FramesLoad {
public:
  ScenerySteps(const EngineStreetHost &host, int resource)
      : m_host(host), m_frames(host.framePaths(resource)) {}

  bool step(std::vector<street::core::Picture> &frames) override {
    if (!m_started) {
      m_started = true;
      int count = 0;
      for (const auto &frame : m_frames) {
        count = std::max(count, frame.first + 1);
      }
      frames.assign(static_cast<std::size_t>(count), street::core::Picture{});
    }
    const std::size_t end =
        std::min(m_next + COLUMNS_PER_STEP, m_frames.size());
    for (; m_next < end; ++m_next) {
      const auto &[index, path] = m_frames[m_next];
      frames[static_cast<std::size_t>(index)] = m_host.loadFrame(path);
    }
    return m_next >= m_frames.size();
  }

private:
  static constexpr std::size_t COLUMNS_PER_STEP = 2;

  const EngineStreetHost &m_host;
  NumberedPaths m_frames;
  std::size_t m_next = 0;
  bool m_started = false;
};

std::unique_ptr<street::scenes::StreetHost::FramesLoad>
EngineStreetHost::beginScenery(int resource) {
  return std::make_unique<ScenerySteps>(*this, resource);
}

std::vector<street::core::Picture>
EngineStreetHost::loadSpriteSet(int resource, int sampleBank) {
  auto frames = loadFrames(resource);
  if (sampleBank != 0) {
    loadSamples(resource, sampleBank);
  }
  return frames;
}

street::core::Picture EngineStreetHost::loadPicture(int resource) {
  return toPicture(m_files.loadBitmap(resourcePath(resource) + ".bmp"));
}

effects::color::AmigaPalette EngineStreetHost::loadPalette(int resource) {
  return m_files.loadBitmap(resourcePath(resource) + ".bmp").palette;
}

std::vector<street::core::Picture> EngineStreetHost::loadScenery(int resource) {
  return loadFrames(resource);
}

street::core::LevelScript EngineStreetHost::loadLevelScript(int resource) {
  street::core::LevelScript script = street::core::LevelScript::fromJson(
      readText(m_files, "level script", resourcePath(resource) + ".json"));
  m_yield();
  return script;
}

street::core::EndingCredits EngineStreetHost::loadEndingCredits() {
  return street::core::EndingCredits::fromJson(
      readText(m_files, "ending credits", m_directory + "/" + CREDITS_FILE));
}

street::core::Picture EngineStreetHost::loadPanelPicture(int part) {
  return toPicture(m_files.loadBitmap(
      assets::partPath(resourceName(PANEL_RESOURCE), part, m_directory)));
}

void EngineStreetHost::loadMusic(int resource) {
  m_speaker.loadMusic(musicPath(resource));
}

bool EngineStreetHost::isMusicLoaded(int resource) const {
  return m_speaker.loadedMusic() == musicPath(resource);
}

void EngineStreetHost::playMusic() { m_speaker.playMusic(); }

void EngineStreetHost::stopMusic() { m_speaker.stopMusic(); }

void EngineStreetHost::setMusicVolume(int volume) {
  m_speaker.setMusicVolume(volume);
}

void EngineStreetHost::setMusicTempo(int tempo) {
  m_speaker.setMusicTempoScale(menuTuneScale(tempo));
}

void EngineStreetHost::playSample(int bank, int sample, int voices) {
  m_speaker.playSample(cachedSampleName(bank, sample), voices);
}

void EngineStreetHost::playSampleAt(int bank, int sample, int voices,
                                    int frequency) {
  m_speaker.playSampleAt(cachedSampleName(bank, sample), voices, frequency);
}

void EngineStreetHost::setSampleLooping(bool loop) {
  m_speaker.setSampleLooping(loop);
}

int EngineStreetHost::random(int limit) {
  if (limit <= 0) {
    return 0;
  }
  return static_cast<int>(m_random.upTo(static_cast<uint32_t>(limit)));
}

void EngineStreetHost::yield() { m_yield(); }

std::string EngineStreetHost::sampleName(int bank, int sample) {
  return "streetBank" + std::to_string(bank) + "Sample" +
         std::to_string(sample);
}

const std::string &EngineStreetHost::cachedSampleName(int bank, int sample) {
  if (bank < 0 || sample < 0) {
    static std::string unusual;
    unusual = sampleName(bank, sample);
    return unusual;
  }
  const std::size_t bankIndex = static_cast<std::size_t>(bank);
  const std::size_t sampleIndex = static_cast<std::size_t>(sample);
  if (m_sampleNames.size() <= bankIndex) {
    m_sampleNames.resize(bankIndex + 1);
  }
  std::vector<std::string> &names = m_sampleNames[bankIndex];
  if (names.size() <= sampleIndex) {
    names.resize(sampleIndex + 1);
  }
  std::string &name = names[sampleIndex];
  if (name.empty()) {
    name = sampleName(bank, sample);
  }
  return name;
}

GameVersion EngineStreetHost::version() const { return m_version; }

std::string EngineStreetHost::resourceName(int resource) const {
  return assets::resourceName(resource, m_version);
}

std::string EngineStreetHost::resourcePath(int resource) const {
  return m_directory + "/" + resourceName(resource);
}

std::string EngineStreetHost::musicPath(int resource) const {
  return resourcePath(resource) + ".s3m";
}

void EngineStreetHost::listSpriteSet(int resource, NumberedPaths &frames,
                                     NumberedPaths &samples) const {
  const std::string name = resourceName(resource);
  const std::string directory = m_directory + "/" + name;
  const std::string framePrefix = name + "_";
  const std::string samplePrefix = name + "_sam";
  for (std::string &path : m_files.list(directory)) {
    const std::string_view file = fileName(path);
    if (const auto index = numberAfter(file, framePrefix, ".bmp")) {
      frames.emplace_back(*index, std::move(path));
    } else if (const auto sample = numberAfter(file, samplePrefix, "_");
               sample && endsWith(file, ".wav")) {
      samples.emplace_back(*sample, std::move(path));
    }
  }
  if (frames.empty()) {
    throw std::runtime_error("No frames found in " + directory);
  }
}

EngineStreetHost::NumberedPaths
EngineStreetHost::framePaths(int resource) const {
  NumberedPaths frames;
  NumberedPaths samples;
  listSpriteSet(resource, frames, samples);
  return frames;
}

EngineStreetHost::NumberedPaths
EngineStreetHost::samplePaths(int resource) const {
  const std::string name = resourceName(resource);
  const std::string prefix = name + "_sam";
  NumberedPaths samples;
  for (const std::string &path : m_files.list(m_directory + "/" + name)) {
    const std::string_view file = fileName(path);
    const auto sample = numberAfter(file, prefix, "_");
    if (sample && endsWith(file, ".wav")) {
      samples.emplace_back(*sample, path);
    }
  }
  return samples;
}

street::core::Picture
EngineStreetHost::loadFrame(const std::string &path) const {
  return toPicture(m_files.loadBitmap(path));
}

std::vector<street::core::Picture>
EngineStreetHost::loadFrames(int resource) const {
  std::vector<street::core::Picture> frames;
  for (const auto &[index, path] : framePaths(resource)) {
    if (frames.size() <= static_cast<std::size_t>(index)) {
      frames.resize(static_cast<std::size_t>(index) + 1);
    }
    frames[static_cast<std::size_t>(index)] = loadFrame(path);
  }
  return frames;
}

void EngineStreetHost::loadSample(int bank, int sample,
                                  const std::string &path) {
  m_speaker.loadSample(sampleName(bank, sample), path);
  m_samples[bank].push_back(sample);
}

void EngineStreetHost::loadSamples(int resource, int bank) {
  clearSamples(bank);
  for (const auto &[sample, path] : samplePaths(resource)) {
    loadSample(bank, sample, path);
  }
}

void EngineStreetHost::clearSamples(int bank) {
  for (int sample : m_samples[bank]) {
    m_speaker.clearSample(sampleName(bank, sample));
  }
  m_samples[bank].clear();
}

} // namespace openfranko::src::engine::states::shared
