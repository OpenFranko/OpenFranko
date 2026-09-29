#include "EngineStreetHost.h"

#include "../../../systems/graphics/Bitmap.h"
#include "../../MenuTempo.h"
#include "../../assets/Assets.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
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

} // namespace

EngineStreetHost::EngineStreetHost(systems::audio::Speaker &speaker,
                                   GameVersion version, std::string directory)
    : m_speaker(speaker), m_version(version), m_directory(std::move(directory)),
      m_random(std::random_device{}()) {}

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
  return toPicture(
      systems::graphics::loadIndexedBitmap(resourcePath(resource) + ".bmp"));
}

effects::color::AmigaPalette EngineStreetHost::loadPalette(int resource) {
  return systems::graphics::loadIndexedBitmap(resourcePath(resource) + ".bmp")
      .palette;
}

std::vector<street::core::Picture> EngineStreetHost::loadScenery(int resource) {
  return loadFrames(resource);
}

street::core::LevelScript EngineStreetHost::loadLevelScript(int resource) {
  const std::string path = resourcePath(resource) + ".json";
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to open level script: " + path);
  }
  std::stringstream text;
  text << file.rdbuf();
  return street::core::LevelScript::fromJson(text.str());
}

street::core::EndingCredits EngineStreetHost::loadEndingCredits() {
  const std::string path = m_directory + "/" + CREDITS_FILE;
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to open ending credits: " + path);
  }
  std::stringstream text;
  text << file.rdbuf();
  return street::core::EndingCredits::fromJson(text.str());
}

street::core::Picture EngineStreetHost::loadPanelPicture(int part) {
  return toPicture(systems::graphics::loadIndexedBitmap(
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
  const std::filesystem::path directory = m_directory + "/" + name;
  std::vector<street::core::Picture> frames;
  std::error_code error;
  for (const auto &entry :
       std::filesystem::directory_iterator(directory, error)) {
    const auto index =
        numberAfter(entry.path().filename().string(), name + "_", ".bmp");
    if (!index) {
      continue;
    }
    if (frames.size() <= static_cast<std::size_t>(*index)) {
      frames.resize(static_cast<std::size_t>(*index) + 1);
    }
    frames[static_cast<std::size_t>(*index)] =
        toPicture(systems::graphics::loadIndexedBitmap(entry.path().string()));
  }
  if (frames.empty()) {
    throw std::runtime_error("No frames found in " + directory.string());
  }
  return frames;
}

void EngineStreetHost::loadSamples(int resource, int bank) {
  clearSamples(bank);
  const std::string name = resourceName(resource);
  const std::filesystem::path directory = m_directory + "/" + name;
  std::error_code error;
  for (const auto &entry :
       std::filesystem::directory_iterator(directory, error)) {
    const std::string file = entry.path().filename().string();
    const auto sample = numberAfter(file, name + "_sam", "_");
    if (!sample || entry.path().extension() != ".wav") {
      continue;
    }
    m_speaker.loadSample(sampleName(bank, *sample), entry.path().string());
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
