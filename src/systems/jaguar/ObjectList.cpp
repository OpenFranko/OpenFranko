#include "ObjectList.h"

namespace openfranko::src::systems::jaguar {
namespace {

constexpr uint64_t BITMAP_TYPE = 0;
constexpr uint64_t SCALED_TYPE = 1;
constexpr uint64_t TYPE_MASK = 7;
constexpr uint64_t GPU_TYPE = 2;
constexpr uint64_t BRANCH_TYPE = 3;
constexpr uint64_t STOP_TYPE = 4;
constexpr int NEVER_LINE = 0x7FE;
constexpr int LINK_SHIFT = 24;
constexpr uint64_t LINK_MASK = uint64_t(0x7FFFF) << LINK_SHIFT;

uint64_t field(uint64_t value, int bits, int shift) {
  return (value & ((uint64_t(1) << bits) - 1)) << shift;
}

uint64_t linkField(uint32_t link) { return field(link >> 3, 19, LINK_SHIFT); }

uint64_t phrase(uint32_t high, uint32_t low) {
  return static_cast<uint64_t>(high) << 32 | low;
}

uint32_t bits(int value, int count, int shift) {
  return (static_cast<uint32_t>(value) & ((1u << count) - 1)) << shift;
}

} // namespace

uint64_t branchPhrase(int halfLine, Branch condition, uint32_t link) {
  return BRANCH_TYPE | field(static_cast<uint64_t>(halfLine), 11, 3) |
         field(static_cast<uint64_t>(condition), 3, 14) | linkField(link);
}

uint64_t stopPhrase() { return STOP_TYPE; }

uint64_t gpuPhrase(int halfLine, uint32_t data) {
  return GPU_TYPE | field(static_cast<uint64_t>(halfLine), 11, 3) |
         field(data, 32, 14);
}

void bitmapPhrases(const BitmapObject &object, uint32_t link, uint64_t *out) {
  const uint32_t next = (link >> 3) & 0x7FFFF;
  const uint32_t type = object.scaled ? SCALED_TYPE : BITMAP_TYPE;
  out[0] = phrase((object.data >> 3) << 11 | next >> 8,
                  type | bits(object.y, 11, 3) | bits(object.height, 10, 14) |
                      (next & 0xFF) << 24);
  const int imageWidth = object.imageWidth & 0x3FF;
  out[1] = phrase(
      bits(imageWidth >> 4, 6, 0) | bits(object.index, 7, 6) |
          bits(object.reflected ? 1 : 0, 1, 13) |
          bits(object.transparent ? 1 : 0, 1, 15) |
          bits(object.released ? 1 : 0, 1, 16) | bits(object.firstPixel, 6, 17),
      bits(object.x, 12, 0) | bits(static_cast<int>(object.depth), 3, 12) |
          bits(object.pitch, 3, 15) | bits(object.dataWidth, 10, 18) |
          bits(imageWidth, 4, 28));
  if (object.scaled) {
    out[2] = phrase(0, bits(object.horizontalScale, 8, 0) |
                           bits(object.verticalScale, 8, 8) |
                           bits(object.verticalScale, 8, 16));
  }
}

ObjectList::ObjectList(uint32_t liveAddress) : m_liveAddress(liveAddress) {}

void ObjectList::clear() {
  m_phrases.clear();
  m_unlinked.clear();
}

void ObjectList::reset(uint32_t liveAddress) {
  m_liveAddress = liveAddress;
  clear();
}

void ObjectList::addBranch(int halfLine, Branch condition, std::size_t target) {
  linkPrevious(m_phrases.size());
  m_phrases.push_back(branchPhrase(halfLine, condition, address(target)));
}

void ObjectList::addGpuObject(int halfLine, uint32_t data) {
  linkPrevious(m_phrases.size());
  m_phrases.push_back(gpuPhrase(halfLine, data));
}

std::size_t ObjectList::addBitmap(const BitmapObject &object) {
  alignTo(object.scaled ? SCALED_ALIGNMENT : OBJECT_ALIGNMENT);
  linkPrevious(m_phrases.size());
  const std::size_t at = m_phrases.size();
  uint64_t phrases[3] = {};
  bitmapPhrases(object, 0, phrases);
  m_phrases.insert(m_phrases.end(), phrases, phrases + (object.scaled ? 3 : 2));
  m_unlinked.push_back(at);
  return at;
}

std::size_t ObjectList::addStop() {
  linkPrevious(m_phrases.size());
  m_phrases.push_back(stopPhrase());
  return m_phrases.size() - 1;
}

void ObjectList::alignTo(int bytes) {
  while ((address(m_phrases.size()) % static_cast<uint32_t>(bytes)) != 0) {
    m_phrases.push_back(
        branchPhrase(NEVER_LINE, Branch::Equal, address(m_phrases.size())));
  }
}

std::size_t ObjectList::size() const { return m_phrases.size(); }

const std::vector<uint64_t> &ObjectList::phrases() const { return m_phrases; }

uint32_t ObjectList::address(std::size_t phrase) const {
  return m_liveAddress + static_cast<uint32_t>(phrase) * PHRASE_BYTES;
}

void ObjectList::linkPrevious(std::size_t next) {
  for (const std::size_t at : m_unlinked) {
    m_phrases[at] = (m_phrases[at] & ~LINK_MASK) | linkField(address(next));
  }
  m_unlinked.clear();
}

void rewriteBitmap(const BitmapObject &object, uint64_t *phrases) {
  uint64_t fresh[3] = {};
  bitmapPhrases(object, 0, fresh);
  phrases[0] = (fresh[0] & ~LINK_MASK) | (phrases[0] & LINK_MASK);
  phrases[1] = fresh[1];
  if (object.scaled) {
    phrases[2] = fresh[2];
  }
}

void rewriteSprite(uint64_t *phrases, uint32_t data, int x, int y, int height,
                   int dataWidth, int imageWidth, int firstPixel) {
  const int width = imageWidth & 0x3FF;
  phrases[0] = (phrases[0] & LINK_MASK) |
               phrase((data >> 3) << 11,
                      BITMAP_TYPE | bits(y, 11, 3) | bits(height, 10, 14));
  phrases[1] =
      phrase(bits(width >> 4, 6, 0) | bits(1, 1, 15) | bits(firstPixel, 6, 17),
             bits(x, 12, 0) | bits(static_cast<int>(Depth::Bits8), 3, 12) |
                 bits(1, 3, 15) | bits(dataWidth, 10, 18) | bits(width, 4, 28));
}

bool isScaledBitmap(uint64_t phrase) {
  return (phrase & TYPE_MASK) == SCALED_TYPE;
}

} // namespace openfranko::src::systems::jaguar
