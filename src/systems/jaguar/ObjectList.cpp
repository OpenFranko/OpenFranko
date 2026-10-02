#include "ObjectList.h"

namespace openfranko::src::systems::jaguar {
namespace {

constexpr uint64_t BITMAP_TYPE = 0;
constexpr uint64_t SCALED_TYPE = 1;
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
  out[0] = (object.scaled ? SCALED_TYPE : BITMAP_TYPE) |
           field(static_cast<uint64_t>(object.y), 11, 3) |
           field(static_cast<uint64_t>(object.height), 10, 14) |
           linkField(link) | field(object.data >> 3, 21, 43);
  out[1] = field(static_cast<uint64_t>(object.x), 12, 0) |
           field(static_cast<uint64_t>(object.depth), 3, 12) |
           field(static_cast<uint64_t>(object.pitch), 3, 15) |
           field(static_cast<uint64_t>(object.dataWidth), 10, 18) |
           field(static_cast<uint64_t>(object.imageWidth), 10, 28) |
           field(static_cast<uint64_t>(object.index), 7, 38) |
           field(object.reflected ? 1 : 0, 1, 45) |
           field(object.transparent ? 1 : 0, 1, 47) |
           field(static_cast<uint64_t>(object.firstPixel), 6, 49);
  if (object.scaled) {
    out[2] = field(object.horizontalScale, 8, 0) |
             field(object.verticalScale, 8, 8) |
             field(object.verticalScale, 8, 16);
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

void ObjectList::addBitmap(const BitmapObject &object) {
  alignTo(object.scaled ? SCALED_ALIGNMENT : OBJECT_ALIGNMENT);
  linkPrevious(m_phrases.size());
  const std::size_t at = m_phrases.size();
  uint64_t phrases[3] = {};
  bitmapPhrases(object, 0, phrases);
  m_phrases.insert(m_phrases.end(), phrases, phrases + (object.scaled ? 3 : 2));
  m_unlinked.push_back(at);
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

} // namespace openfranko::src::systems::jaguar
