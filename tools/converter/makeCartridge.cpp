#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/jaguarCartridge/jaguarCartridge.h"
#include "../../lib/converter/packedArchive/packedArchive.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

using namespace openfranko::lib;
namespace cartridge = converter::jaguarCartridge;

namespace {

constexpr int TEMPORARY_ATTEMPTS = 100;

std::vector<uint8_t> readFile(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot read " + path.string());
  }
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),
                              std::istreambuf_iterator<char>());
}

void writeFile(const std::filesystem::path &path,
               const std::vector<uint8_t> &data) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file.write(reinterpret_cast<const char *>(data.data()),
             static_cast<std::streamsize>(data.size()));
  if (!file) {
    throw std::runtime_error("Cannot write " + path.string());
  }
}

std::string quoted(const std::string &text) {
#ifdef _WIN32
  return "\"" + text + "\"";
#else
  std::string result = "'";
  for (const char character : text) {
    if (character == '\'') {
      result += "'\\''";
    } else {
      result += character;
    }
  }
  return result + "'";
#endif
}

class TemporaryDirectory {
public:
  TemporaryDirectory() {
    std::random_device entropy;
    for (int attempt = 0; attempt < TEMPORARY_ATTEMPTS; ++attempt) {
      m_path = std::filesystem::temp_directory_path() /
               ("makeCartridge-" + std::to_string(entropy()));
      if (std::filesystem::create_directory(m_path)) {
        return;
      }
    }
    throw std::runtime_error("Cannot create a temporary directory");
  }

  ~TemporaryDirectory() {
    std::error_code error;
    std::filesystem::remove_all(m_path, error);
  }

  TemporaryDirectory(const TemporaryDirectory &) = delete;
  TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;

  const std::filesystem::path &path() const { return m_path; }

private:
  std::filesystem::path m_path;
};

std::vector<uint8_t> signedImage(const std::vector<uint8_t> &payload,
                                 const std::string &jagcrypt) {
  TemporaryDirectory work;
  writeFile(work.path() / "cart.bin", payload);
#ifdef _WIN32
  const std::string changeDirectory = "cd /d ";
#else
  const std::string changeDirectory = "cd ";
#endif
  const std::string command =
      changeDirectory + quoted(work.path().string()) + " && " +
      quoted(std::filesystem::absolute(jagcrypt).string()) +
      " -u cart.bin > jagcrypt.log 2>&1";
  if (std::system(command.c_str()) != 0) {
    std::ostringstream message;
    message << "jagcrypt failed";
    const std::filesystem::path log = work.path() / "jagcrypt.log";
    if (std::filesystem::exists(log)) {
      const std::vector<uint8_t> output = readFile(log);
      if (!output.empty()) {
        message << ":\n" << std::string(output.begin(), output.end());
      }
    }
    throw std::runtime_error(message.str());
  }
  return readFile(work.path() / "cart.U1");
}

} // namespace

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);
  const auto program = parser.option("-p");
  const auto input = parser.option("-i");
  const auto output = parser.option("-o");
  const auto jagcrypt = parser.option("-j");
  if (!program || !input || !output) {
    std::cerr << "Usage: " << argv[0]
              << " -p <franko-jaguar.bin> -i <assets_dir> -o <cartridge.j64>"
                 " [-j <jagcrypt>]"
              << std::endl;
    std::cerr << "Packs the directory made by frankoExtract behind the Jaguar "
                 "program and writes a cartridge image. With -j it is signed "
                 "with jagcrypt from the Jaguar SDK so that a console starts "
                 "it; without, it runs in emulators."
              << std::endl;
    return 1;
  }
  try {
    const std::vector<uint8_t> archive =
        converter::packedArchive::packDirectory(*input, "assets");
    std::cerr << "Packed " << *input << " (" << archive.size() << " bytes)"
              << std::endl;
    const std::vector<uint8_t> payload =
        cartridge::payload(readFile(*program), archive);
    std::vector<uint8_t> image;
    if (jagcrypt &&
        cartridge::cartridgeSize(payload.size()) == cartridge::CART_SIZE) {
      std::cerr << "Signing the cartridge with " << *jagcrypt << std::endl;
      image = signedImage(payload, *jagcrypt);
    } else {
      if (jagcrypt) {
        std::cerr << "jagcrypt signs only 4 MB cartridges, so this 6 MB one "
                     "is not signed"
                  << std::endl;
      }
      image = cartridge::unsignedImage(payload);
    }
    writeFile(*output, image);
    std::cerr << "Wrote " << *output << std::endl;
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << std::endl;
    return 1;
  }
  return 0;
}
