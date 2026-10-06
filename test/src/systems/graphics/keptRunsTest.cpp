#include "../../../../src/systems/graphics/KeptRuns.h"

#include <catch2/catch_all.hpp>

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

using namespace openfranko::src::systems::graphics;

namespace {

constexpr int GROUPS = 12;
constexpr int WIDTH = GROUPS * GROUP_PIXELS;
constexpr int OLD_VALUE = 100;
constexpr int OTHER_ROW_VALUE = 200;
constexpr int NEW_VALUE = 900;

using Edges = std::vector<std::pair<int, int>>;

Edges edges(const std::array<Span, 2> &spans) {
  return {{spans[0].first, spans[0].last}, {spans[1].first, spans[1].last}};
}

struct Run {
  int from = 0;
  int to = 0;
  int count = 0;
  bool backward = false;

  bool operator==(const Run &other) const {
    return from == other.from && to == other.to && count == other.count &&
           backward == other.backward;
  }
};

std::vector<Run> keptRuns(const std::array<Span, 2> &spans, int moved,
                          bool sameRow) {
  std::vector<Run> runs;
  copyKeptRuns(spans, GROUPS, moved, sameRow, [&](const KeptRun &run) {
    runs.push_back({run.from, run.to, run.count, run.backward});
  });
  return runs;
}

bool covers(const std::array<Span, 2> &spans, int group) {
  const int pixel = group * GROUP_PIXELS;
  for (const Span &span : spans) {
    if (pixel >= span.first && pixel < span.last) {
      return true;
    }
  }
  return false;
}

struct Rebuilt {
  std::vector<int> row;
  std::vector<int> writes;
};

Rebuilt rebuild(const std::array<Span, 2> &spans, int moved, bool sameRow) {
  Rebuilt rebuilt;
  std::vector<int> otherRow;
  for (int group = 0; group < GROUPS; ++group) {
    rebuilt.row.push_back(OLD_VALUE + group);
    otherRow.push_back(OTHER_ROW_VALUE + group);
  }
  rebuilt.writes.assign(GROUPS, 0);
  copyKeptRuns(spans, GROUPS, moved, sameRow, [&](const KeptRun &run) {
    for (int step = 0; step < run.count; ++step) {
      const int offset = run.backward ? run.count - 1 - step : step;
      const std::size_t from = static_cast<std::size_t>(run.from + offset);
      const std::size_t to = static_cast<std::size_t>(run.to + offset);
      rebuilt.row[to] = sameRow ? rebuilt.row[from] : otherRow[from];
      ++rebuilt.writes[to];
    }
  });
  for (int group = 0; group < GROUPS; ++group) {
    if (covers(spans, group)) {
      rebuilt.row[static_cast<std::size_t>(group)] = NEW_VALUE + group;
      ++rebuilt.writes[static_cast<std::size_t>(group)];
    }
  }
  return rebuilt;
}

int movedValue(const std::array<Span, 2> &spans, int group, int moved,
               bool sameRow) {
  if (covers(spans, group)) {
    return NEW_VALUE + group;
  }
  return (sameRow ? OLD_VALUE : OTHER_ROW_VALUE) + group - moved;
}

Span uncoveredEdge(int moved) {
  if (moved < 0) {
    return {(GROUPS + moved) * GROUP_PIXELS, WIDTH};
  }
  return {0, moved * GROUP_PIXELS};
}

} // namespace

SCENARIO("groupSpans widens the spans to whole pixel groups") {
  GIVEN("A span that starts and ends inside groups") {
    const std::array<Span, 2> spans = {Span{5, 9}, Span{}};

    THEN("It grows to the edges of those groups") {
      REQUIRE(edges(groupSpans(spans, WIDTH)) == Edges{{4, 12}, {0, 0}});
    }
  }

  GIVEN("A span that ends past the row") {
    const std::array<Span, 2> spans = {Span{10, WIDTH}, Span{}};

    THEN("It ends at the row width") {
      REQUIRE(edges(groupSpans(spans, WIDTH - 2)) ==
              Edges{{8, WIDTH - 2}, {0, 0}});
    }
  }

  GIVEN("Two spans that overlap once they are widened") {
    const std::array<Span, 2> spans = {Span{1, 5}, Span{6, 10}};

    THEN("They merge into the first span") {
      REQUIRE(edges(groupSpans(spans, WIDTH)) == Edges{{0, 12}, {0, 0}});
    }
  }

  GIVEN("Two spans that touch") {
    const std::array<Span, 2> spans = {Span{0, 4}, Span{4, 8}};

    THEN("They merge into one span") {
      REQUIRE(edges(groupSpans(spans, WIDTH)) == Edges{{0, 8}, {0, 0}});
    }
  }

  GIVEN("Two spans in reverse order") {
    const std::array<Span, 2> spans = {Span{20, 24}, Span{2, 3}};

    THEN("The leftmost span comes first") {
      REQUIRE(edges(groupSpans(spans, WIDTH)) == Edges{{0, 4}, {20, 24}});
    }
  }

  GIVEN("Only the second span is set") {
    const std::array<Span, 2> spans = {Span{}, Span{13, 14}};

    THEN("It becomes the first span") {
      REQUIRE(edges(groupSpans(spans, WIDTH)) == Edges{{12, 16}, {0, 0}});
    }
  }
}

SCENARIO("copyKeptRuns copies the groups outside the spans") {
  GIVEN("A row scrolled two groups to the left") {
    const std::array<Span, 2> spans = {Span{8, 16}, uncoveredEdge(-2)};

    THEN("Each run reads two groups further right, from left to right") {
      REQUIRE(keptRuns(spans, -2, true) ==
              std::vector<Run>{{2, 0, 2, false}, {6, 4, 6, false}});
    }
  }

  GIVEN("A row scrolled two groups to the right") {
    const std::array<Span, 2> spans = {uncoveredEdge(2), Span{24, 32}};

    THEN("Each run copies backwards, from right to left") {
      REQUIRE(keptRuns(spans, 2, true) ==
              std::vector<Run>{{6, 8, 4, true}, {0, 2, 4, true}});
    }
  }

  GIVEN("A row taken from another row and shifted right") {
    const std::array<Span, 2> spans = {uncoveredEdge(2), Span{24, 32}};

    THEN("The runs copy forwards, from left to right") {
      REQUIRE(keptRuns(spans, 2, false) ==
              std::vector<Run>{{0, 2, 4, false}, {6, 8, 4, false}});
    }
  }

  GIVEN("A row taken from another row without spans") {
    const std::array<Span, 2> spans{};

    THEN("One run copies the whole row") {
      REQUIRE(keptRuns(spans, 0, false) ==
              std::vector<Run>{{0, 0, GROUPS, false}});
    }
  }

  GIVEN("A span that covers the whole row") {
    const std::array<Span, 2> spans = {Span{0, WIDTH}, Span{}};

    THEN("Nothing is copied") { REQUIRE(keptRuns(spans, -2, true).empty()); }
  }
}

SCENARIO("The kept runs and the spans rebuild a moved row") {
  GIVEN("Moved rows with a changed span anywhere and the uncovered edge") {
    THEN("Each group is written once and ends with its value in the new row") {
      for (const int moved : {-2, 0, 2}) {
        for (const bool sameRow : {true, false}) {
          if (moved == 0 && sameRow) {
            continue;
          }
          for (int first = 0; first <= WIDTH; ++first) {
            for (int length = 0; length <= 3 * GROUP_PIXELS; ++length) {
              const std::array<Span, 2> spans = groupSpans(
                  {Span{first, first + length}, uncoveredEdge(moved)}, WIDTH);
              const Rebuilt rebuilt = rebuild(spans, moved, sameRow);
              for (int group = 0; group < GROUPS; ++group) {
                const std::size_t at = static_cast<std::size_t>(group);
                REQUIRE(rebuilt.writes[at] == 1);
                REQUIRE(rebuilt.row[at] ==
                        movedValue(spans, group, moved, sameRow));
              }
            }
          }
        }
      }
    }
  }
}
