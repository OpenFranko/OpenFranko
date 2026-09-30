#include "EngineStreetHost.h"

#include "../../../systems/graphics/Bitmap.h"
#include "../../MenuTempo.h"
#include "../../assets/Assets.h"

#include <cctype>
#include <optional>
#include <stdexcept>
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

std::optional<int> numberAfter(const std::string &text,
                               const std::string &prefix,
                               const std::string &suffix) {
  if (text.size() <= prefix.size() + suffix.size() ||
      text.compare(0, prefix.size(), prefix) != 0) {
    return std::nullopt;
  }
  std::size_t end = prefix.size();
  while (end < text.size() &&
         std::isdigit(static_cast<unsigned char>(text[end]))) {
    ++end;
  }
  if (end == prefix.size() || text.compare(end, suffix.size(), suffix) != 0) {
    return std::nullopt;
  }
  return std::stoi(text.substr(prefix.size(), end - prefix.size()));
}

std::string fileName(const std::string &path) {
  std::size_t start = path.size();
  while (start > 0 && path[start - 1] != '/' && path[start - 1] != '\\') {
    --start;
  }
  return path.substr(start);
}

bool endsWith(const std::string &text, const std::string &suffix) {
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
                                   std::mt19937 &random,
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
  m_speaker.playSample(sampleName(bank, sample), voices);
}

void EngineStreetHost::playSampleAt(int bank, int sample, int voices,
                                    int frequency) {
  m_speaker.playSampleAt(sampleName(bank, sample), voices, frequency);
}

void EngineStreetHost::setSampleLooping(bool loop) {
  m_speaker.setSampleLooping(loop);
}

int EngineStreetHost::random(int limit) {
  if (limit <= 0) {
    return 0;
  }
  return std::uniform_int_distribution<int>(0, limit)(m_random);
}

void EngineStreetHost::yield() { m_yield(); }

std::string EngineStreetHost::sampleName(int bank, int sample) {
  return "streetBank" + std::to_string(bank) + "Sample" +
         std::to_string(sample);
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

std::vector<street::core::Picture>
EngineStreetHost::loadFrames(int resource) const {
  const std::string name = resourceName(resource);
  const std::string directory = m_directory + "/" + name;
  const std::string prefix = name + "_";
  std::vector<street::core::Picture> frames;
  for (const std::string &path : m_files.list(directory)) {
    const auto index = numberAfter(fileName(path), prefix, ".bmp");
    if (!index) {
      continue;
    }
    if (frames.size() <= static_cast<std::size_t>(*index)) {
      frames.resize(static_cast<std::size_t>(*index) + 1);
    }
    frames[static_cast<std::size_t>(*index)] =
        toPicture(m_files.loadBitmap(path));
  }
  if (frames.empty()) {
    throw std::runtime_error("No frames found in " + directory);
  }
  return frames;
}

void EngineStreetHost::loadSamples(int resource, int bank) {
  clearSamples(bank);
  const std::string name = resourceName(resource);
  const std::string prefix = name + "_sam";
  for (const std::string &path : m_files.list(m_directory + "/" + name)) {
    const std::string file = fileName(path);
    const auto sample = numberAfter(file, prefix, "_");
    if (!sample || !endsWith(file, ".wav")) {
      continue;
    }
    m_speaker.loadSample(sampleName(bank, *sample), path);
    m_samples[bank].push_back(*sample);
  }
}

void EngineStreetHost::clearSamples(int bank) {
  for (int sample : m_samples[bank]) {
    m_speaker.clearSample(sampleName(bank, sample));
  }
  m_samples[bank].clear();
}

} // namespace openfranko::src::engine::states::shared
