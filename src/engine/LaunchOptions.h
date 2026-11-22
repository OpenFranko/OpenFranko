#ifndef ENGINE_LAUNCHOPTIONS_H_
#define ENGINE_LAUNCHOPTIONS_H_

namespace openfranko {
namespace src {
namespace engine {

struct LaunchOptions {
  bool fullscreen = false;
};

LaunchOptions parseLaunchOptions(int argc, const char *const *argv);

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_LAUNCHOPTIONS_H_
