#ifndef SYSTEMS_GRAPHICS_KEPTRUNS_H_
#define SYSTEMS_GRAPHICS_KEPTRUNS_H_

#include "graphics/IndexedRasterizer.h"

#include <algorithm>
#include <array>
#include <utility>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

inline constexpr int GROUP_SHIFT = 2;
inline constexpr int GROUP_PIXELS = 1 << GROUP_SHIFT;
inline constexpr int GROUP_MASK = GROUP_PIXELS - 1;

struct KeptRun {
  int from = 0;
  int to = 0;
  int count = 0;
  bool backward = false;
};

inline std::array<Span, 2> groupSpans(std::array<Span, 2> spans, int width) {
  for (Span &span : spans) {
    if (span.first >= span.last) {
      span = {};
      continue;
    }
    span = {span.first & ~GROUP_MASK,
            std::min(width, (span.last + GROUP_MASK) & ~GROUP_MASK)};
  }
  if (spans[1].first < spans[1].last &&
      (spans[0].first >= spans[0].last || spans[1].first < spans[0].first)) {
    std::swap(spans[0], spans[1]);
  }
  if (spans[1].first < spans[1].last && spans[1].first <= spans[0].last) {
    spans[0].last = std::max(spans[0].last, spans[1].last);
    spans[1] = {};
  }
  return spans;
}

template <typename Copy>
void copyKeptRuns(const std::array<Span, 2> &spans, int groups, int moved,
                  bool sameRow, Copy &&copy) {
  const int first = std::max(0, moved);
  const int last = std::min(groups, groups + moved);
  const bool backward = sameRow && moved > 0;
  const auto outside = [&](const Span &span) {
    return span.first >= span.last || span.last <= first << GROUP_SHIFT ||
           span.first >= last << GROUP_SHIFT;
  };
  if (outside(spans[0]) && outside(spans[1])) {
    if (first < last) {
      copy(KeptRun{first - moved, first, last - first, backward});
    }
    return;
  }
  const auto start = [&](const Span &span) {
    return span.first < span.last ? std::min(span.first >> GROUP_SHIFT, last)
                                  : last;
  };
  const auto end = [&](const Span &span, int after) {
    return span.first < span.last
               ? std::max(after, (span.last + GROUP_MASK) >> GROUP_SHIFT)
               : std::max(after, last);
  };
  const int firstStart = start(spans[0]);
  const int firstEnd = end(spans[0], first);
  const int secondStart = start(spans[1]);
  const int secondEnd = end(spans[1], firstEnd);
  const auto keep = [&](int from, int to) {
    if (from < to) {
      copy(KeptRun{from - moved, from, to - from, backward});
    }
  };
  if (backward) {
    keep(secondEnd, last);
    keep(firstEnd, secondStart);
    keep(first, firstStart);
  } else {
    keep(first, firstStart);
    keep(firstEnd, secondStart);
    keep(secondEnd, last);
  }
}

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_KEPTRUNS_H_
