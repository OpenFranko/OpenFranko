#ifndef SYSTEMS_JAGUAR_OBJECTLIST_H_
#define SYSTEMS_JAGUAR_OBJECTLIST_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {

inline constexpr int PHRASE_BYTES = 8;
inline constexpr int OBJECT_ALIGNMENT = 16;
inline constexpr int SCALED_ALIGNMENT = 32;
inline constexpr int ALL_LINES = 0x7FF;
inline constexpr uint8_t SCALE_ONE = 0x20;

enum class Depth : uint8_t {
  Bits1 = 0,
  Bits2 = 1,
  Bits4 = 2,
  Bits8 = 3,
  Bits16 = 4
};

enum class Branch : uint8_t {
  Equal = 0,
  Above = 1,
  Below = 2,
  Flag = 3,
  SecondHalf = 4
};

struct BitmapObject {
  uint32_t data = 0;
  int firstPixel = 0;
  int x = 0;
  int y = 0;
  int height = 0;
  int dataWidth = 0;
  int imageWidth = 0;
  int pitch = 1;
  Depth depth = Depth::Bits8;
  int index = 0;
  bool transparent = false;
  bool reflected = false;
  bool scaled = false;
  bool released = false;
  uint8_t horizontalScale = SCALE_ONE;
  uint8_t verticalScale = SCALE_ONE;
};

class ObjectList {
public:
  explicit ObjectList(uint32_t liveAddress);

  void clear();
  void reset(uint32_t liveAddress);
  void addBranch(int halfLine, Branch condition, std::size_t target);
  void addGpuObject(int halfLine, uint32_t data);
  std::size_t addBitmap(const BitmapObject &object);
  std::size_t addStop();
  void alignTo(int bytes);

  std::size_t size() const;
  const std::vector<uint64_t> &phrases() const;
  uint32_t address(std::size_t phrase) const;

private:
  void linkPrevious(std::size_t next);

  uint32_t m_liveAddress;
  std::vector<uint64_t> m_phrases;
  std::vector<std::size_t> m_unlinked;
};

uint64_t branchPhrase(int halfLine, Branch condition, uint32_t link);
uint64_t stopPhrase();
uint64_t gpuPhrase(int halfLine, uint32_t data);
void bitmapPhrases(const BitmapObject &object, uint32_t link, uint64_t *out);
void rewriteBitmap(const BitmapObject &object, uint64_t *phrases);
void rewriteSprite(uint64_t *phrases, uint32_t data, int x, int y, int height,
                   int dataWidth, int imageWidth, int firstPixel);
bool isScaledBitmap(uint64_t phrase);

} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_OBJECTLIST_H_
