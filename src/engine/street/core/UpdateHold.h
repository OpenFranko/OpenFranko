#ifndef ENGINE_STREET_CORE_UPDATEHOLD_H_
#define ENGINE_STREET_CORE_UPDATEHOLD_H_

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

class UpdateHold {
public:
  void start(long frame, int frames);
  bool holdsAtStart(long frame) const;
  bool holdsAtEnd(long frame) const;

private:
  long m_start = -1;
  long m_until = -1;
};

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_UPDATEHOLD_H_
