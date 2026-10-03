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
constexpr std::size_t MAX_FRAME_DIGITS = 9;
constexpr std::size_t EXPECTED_FRAMES = 64;
constexpr std::string_view FRAME_SUFFIX = ".bmp";

struct Numbered {
  int number = 0;
  std::size_t end = 0;
};

std::optional<Numbered> numberAfter(std::string_view text,
                                    std::string_view prefix,
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
  return Numbered{number, end};
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

class EngineStreetHost::SetListing {
public:
  struct Frame {
    int index = 0;
    std::size_t digits = 0;
    std::string name;
  };

  SetListing(const EngineStreetHost &host, int resource)
      : m_name(host.resourceName(resource)),
        m_directory(host.m_directory + "/" + m_name),
        m_framePrefix(m_name + "_"), m_samplePrefix(m_name + "_sam"),
        m_walk(host.m_files.walk(m_directory)) {
    m_frames.reserve(EXPECTED_FRAMES);
  }

  bool step(std::size_t entries) {
    std::string_view file;
    for (std::size_t visited = 0; visited < entries; ++visited) {
      if (!m_walk->next(file)) {
        return true;
      }
      if (const auto frame = numberAfter(file, m_framePrefix, FRAME_SUFFIX)) {
        Frame &added = m_frames.emplace_back();
        added.index = frame->number;
        const std::size_t digits = frame->end - m_framePrefix.size();
        if (frame->end + FRAME_SUFFIX.size() == file.size() &&
            digits <= MAX_FRAME_DIGITS) {
          added.digits = digits;
        } else {
          added.name = file;
        }
      } else if (const auto sample = numberAfter(file, m_samplePrefix, "_");
                 sample && endsWith(file, ".wav")) {
        m_samples.emplace_back(sample->number, file);
      }
    }
    return false;
  }

  void finish() {
    while (!step(ENTRIES_PER_STEP)) {
    }
  }

  void requireFrames() const {
    if (m_frames.empty()) {
      throw std::runtime_error("No frames found in " + m_directory);
    }
  }

  void dropSamples() { m_samples.clear(); }

  const std::vector<Frame> &frames() const { return m_frames; }
  const NumberedFiles &samples() const { return m_samples; }

  std::size_t frameCount() const {
    int count = 0;
    for (const Frame &frame : m_frames) {
      count = std::max(count, frame.index + 1);
    }
    return static_cast<std::size_t>(count);
  }

  const std::string &path(const Frame &frame) {
    m_path.assign(m_directory);
    m_path += '/';
    if (!frame.name.empty()) {
      m_path += frame.name;
      return m_path;
    }
    m_path += m_framePrefix;
    char digits[MAX_FRAME_DIGITS];
    int value = frame.index;
    for (std::size_t at = frame.digits; at > 0; --at) {
      digits[at - 1] = static_cast<char>('0' + value % DECIMAL_BASE);
      value /= DECIMAL_BASE;
    }
    m_path.append(digits, frame.digits);
    m_path += FRAME_SUFFIX;
    return m_path;
  }

  const std::string &path(const std::string &file) {
    m_path.assign(m_directory);
    m_path += '/';
    m_path += file;
    return m_path;
  }

  static constexpr std::size_t ENTRIES_PER_STEP = 16;

private:
  std::string m_name;
  std::string m_directory;
  std::string m_framePrefix;
  std::string m_samplePrefix;
  std::unique_ptr<assets::Files::Listing> m_walk;
  std::string m_path;
  std::vector<Frame> m_frames;
  NumberedFiles m_samples;
};

class EngineStreetHost::SpriteSetSteps : public SpriteSetLoad {
public:
  SpriteSetSteps(EngineStreetHost &host, int resource, int sampleBank, int base,
                 int steps)
      : m_host(host), m_resource(resource), m_sampleBank(sampleBank),
        m_base(base), m_steps(steps) {}

  bool step(street::core::ImageBank &images) override {
    ++m_taken;
    if (!m_listing) {
      if (m_sampleBank != 0 && m_host.clearFirstSample(m_sampleBank)) {
        return false;
      }
      m_listing.emplace(m_host, m_resource);
      return false;
    }
    if (!m_listed) {
      if (!m_listing->step(SetListing::ENTRIES_PER_STEP)) {
        return false;
      }
      m_listed = true;
      m_listing->requireFrames();
      if (m_sampleBank == 0) {
        m_listing->dropSamples();
      }
      fitFrames();
      return false;
    }
    const std::vector<SetListing::Frame> &frames = m_listing->frames();
    if (m_nextFrame < frames.size()) {
      const std::size_t end =
          std::min(m_nextFrame + m_framesPerStep, frames.size());
      for (; m_nextFrame < end; ++m_nextFrame) {
        const SetListing::Frame &frame = frames[m_nextFrame];
        images.load(m_base + frame.index,
                    m_host.loadFrame(m_listing->path(frame)));
      }
      if (m_nextFrame < frames.size()) {
        return false;
      }
      clearGaps(images);
      return m_listing->samples().empty();
    }
    const NumberedFiles &samples = m_listing->samples();
    if (m_nextSample < samples.size()) {
      const auto &[sample, file] = samples[m_nextSample++];
      m_host.loadSample(m_sampleBank, sample, m_listing->path(file));
    }
    return m_nextSample >= samples.size();
  }

private:
  void fitFrames() {
    if (m_steps == FRAME_BY_FRAME) {
      return;
    }
    const std::size_t steps = static_cast<std::size_t>(m_steps);
    const std::size_t reserved = m_taken + m_listing->samples().size();
    const std::size_t left = steps > reserved ? steps - reserved : 1;
    const std::size_t frames = m_listing->frames().size();
    m_framesPerStep = std::max<std::size_t>(1, (frames + left - 1) / left);
  }

  void clearGaps(street::core::ImageBank &images) const {
    const std::size_t count = m_listing->frameCount();
    std::vector<bool> present(count, false);
    for (const SetListing::Frame &frame : m_listing->frames()) {
      present[static_cast<std::size_t>(frame.index)] = true;
    }
    for (std::size_t index = 0; index < count; ++index) {
      if (!present[index]) {
        images.load(m_base + static_cast<int>(index), street::core::Picture{});
      }
    }
  }

  EngineStreetHost &m_host;
  int m_resource;
  int m_sampleBank;
  int m_base;
  int m_steps;
  std::size_t m_taken = 0;
  std::size_t m_framesPerStep = 1;
  std::optional<SetListing> m_listing;
  std::size_t m_nextFrame = 0;
  std::size_t m_nextSample = 0;
  bool m_listed = false;
};

std::unique_ptr<street::scenes::StreetHost::SpriteSetLoad>
EngineStreetHost::beginSpriteSet(int resource, int sampleBank, int base,
                                 int steps) {
  return std::make_unique<SpriteSetSteps>(*this, resource, sampleBank, base,
                                          steps);
}

class EngineStreetHost::ScenerySteps : public FramesLoad {
public:
  ScenerySteps(const EngineStreetHost &host, int resource)
      : m_host(host), m_listing(host, resource) {}

  bool step(std::vector<street::core::Picture> &frames) override {
    if (!m_listed) {
      if (!m_listing.step(SetListing::ENTRIES_PER_STEP)) {
        return false;
      }
      m_listed = true;
      m_listing.requireFrames();
      frames.assign(m_listing.frameCount(), street::core::Picture{});
      return false;
    }
    const std::vector<SetListing::Frame> &listed = m_listing.frames();
    const std::size_t end = std::min(m_next + COLUMNS_PER_STEP, listed.size());
    for (; m_next < end; ++m_next) {
      const SetListing::Frame &frame = listed[m_next];
      frames[static_cast<std::size_t>(frame.index)] =
          m_host.loadFrame(m_listing.path(frame));
    }
    return m_next >= listed.size();
  }

private:
  static constexpr std::size_t COLUMNS_PER_STEP = 2;

  const EngineStreetHost &m_host;
  SetListing m_listing;
  std::size_t m_next = 0;
  bool m_listed = false;
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

class EngineStreetHost::LevelScriptSteps : public LevelScriptLoad {
public:
  LevelScriptSteps(EngineStreetHost &host, int resource)
      : m_host(host), m_resource(resource) {}

  bool step(street::core::LevelScript &script) override {
    if (!m_reader) {
      m_reader.emplace(readText(m_host.m_files, "level script",
                                m_host.resourcePath(m_resource) + ".json"));
      return false;
    }
    return m_reader->step(script);
  }

private:
  EngineStreetHost &m_host;
  int m_resource;
  std::optional<street::core::LevelScriptReader> m_reader;
};

std::unique_ptr<street::scenes::StreetHost::LevelScriptLoad>
EngineStreetHost::beginLevelScript(int resource) {
  return std::make_unique<LevelScriptSteps>(*this, resource);
}

class EngineStreetHost::PictureSteps : public PictureLoad {
public:
  PictureSteps(EngineStreetHost &host, int resource)
      : m_host(host), m_resource(resource) {}

  bool step(street::core::Picture &picture,
            effects::color::AmigaPalette *palette) override {
    if (!m_load) {
      m_load =
          m_host.m_files.beginBitmap(m_host.resourcePath(m_resource) + ".bmp");
    }
    if (!m_load->step(m_bitmap)) {
      return false;
    }
    if (palette) {
      *palette = m_bitmap.palette;
    }
    picture = toPicture(std::move(m_bitmap));
    return true;
  }

private:
  EngineStreetHost &m_host;
  int m_resource;
  std::unique_ptr<assets::Files::BitmapLoad> m_load;
  systems::graphics::IndexedBitmap m_bitmap;
};

std::unique_ptr<street::scenes::StreetHost::PictureLoad>
EngineStreetHost::beginPicture(int resource) {
  return std::make_unique<PictureSteps>(*this, resource);
}

class EngineStreetHost::MusicSteps : public MusicLoad {
public:
  MusicSteps(EngineStreetHost &host, int resource, int steps)
      : m_host(host), m_path(host.musicPath(resource)), m_steps(steps) {}

  bool step() override {
    ++m_taken;
    if (m_music) {
      return m_music->step();
    }
    try {
      if (!m_read) {
        m_read = m_host.m_files.beginRead(m_path);
      }
      if (!m_read->step(m_data)) {
        return false;
      }
    } catch (const std::runtime_error &) {
      m_host.m_speaker.loadMusic(m_path);
      return true;
    }
    m_music = m_host.m_speaker.beginMusic(m_path, std::move(m_data),
                                          std::max(m_steps - m_taken, 1));
    return false;
  }

private:
  EngineStreetHost &m_host;
  std::string m_path;
  int m_steps;
  int m_taken = 0;
  std::unique_ptr<assets::Files::FileLoad> m_read;
  std::vector<uint8_t> m_data;
  std::unique_ptr<systems::audio::Speaker::MusicLoad> m_music;
};

std::unique_ptr<street::scenes::StreetHost::MusicLoad>
EngineStreetHost::beginMusic(int resource, int steps) {
  return std::make_unique<MusicSteps>(*this, resource, steps);
}

street::core::LevelScript EngineStreetHost::loadLevelScript(int resource) {
  street::core::LevelScript script = street::core::LevelScript::fromJson(
      readText(m_files, "level script", resourcePath(resource) + ".json"));
  m_yield();
  return script;
}

class EngineStreetHost::CreditsSteps : public CreditsLoad {
public:
  explicit CreditsSteps(EngineStreetHost &host) : m_host(host) {}

  bool step(street::core::EndingCredits &credits) override {
    if (!m_reader) {
      m_reader.emplace(readText(m_host.m_files, "ending credits",
                                m_host.m_directory + "/" + CREDITS_FILE));
      return false;
    }
    return m_reader->step(credits);
  }

private:
  EngineStreetHost &m_host;
  std::optional<street::core::EndingCreditsReader> m_reader;
};

std::unique_ptr<street::scenes::StreetHost::CreditsLoad>
EngineStreetHost::beginEndingCredits() {
  return std::make_unique<CreditsSteps>(*this);
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

street::core::Picture
EngineStreetHost::loadFrame(const std::string &path) const {
  return toPicture(m_files.loadBitmap(path));
}

std::vector<street::core::Picture>
EngineStreetHost::loadFrames(int resource) const {
  SetListing listing(*this, resource);
  listing.finish();
  listing.requireFrames();
  std::vector<street::core::Picture> frames(listing.frameCount());
  for (const SetListing::Frame &frame : listing.frames()) {
    frames[static_cast<std::size_t>(frame.index)] =
        loadFrame(listing.path(frame));
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
  SetListing listing(*this, resource);
  listing.finish();
  for (const auto &[sample, file] : listing.samples()) {
    loadSample(bank, sample, listing.path(file));
  }
}

bool EngineStreetHost::clearFirstSample(int bank) {
  std::vector<int> &samples = m_samples[bank];
  if (samples.empty()) {
    return false;
  }
  m_speaker.clearSample(sampleName(bank, samples.front()));
  samples.erase(samples.begin());
  return true;
}

void EngineStreetHost::clearSamples(int bank) {
  for (int sample : m_samples[bank]) {
    m_speaker.clearSample(sampleName(bank, sample));
  }
  m_samples[bank].clear();
}

} // namespace openfranko::src::engine::states::shared
