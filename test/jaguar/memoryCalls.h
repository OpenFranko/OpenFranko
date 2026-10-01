#ifndef TEST_JAGUAR_MEMORYCALLS_H_
#define TEST_JAGUAR_MEMORYCALLS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace memory_calls {

struct CallSite {
  uint32_t caller = 0;
  uint32_t calls = 0;
  uint32_t bytes = 0;
  uint8_t kind = 0;
};

void start();
void stop();
std::vector<CallSite> heaviest(std::size_t count);

} // namespace memory_calls

#endif // TEST_JAGUAR_MEMORYCALLS_H_
