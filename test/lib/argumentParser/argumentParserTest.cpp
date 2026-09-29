#include "../../../lib/argumentParser/ArgumentParser.h"

#include <catch2/catch_all.hpp>

#include <string>
#include <vector>

using namespace openfranko::lib::argumentParser;

SCENARIO("ArgumentParser gives the value after each option") {
  GIVEN("A valid command line input") {
    std::vector<std::string> arguments = {"program", "-i", "input.txt", "-o",
                                          "output.txt"};
    std::vector<char *> argv;
    for (std::string &argument : arguments) {
      argv.push_back(argument.data());
    }

    WHEN("Parsing the arguments") {
      ArgumentParser parser(static_cast<int>(argv.size()), argv.data());

      THEN("The input file is the value after -i") {
        REQUIRE(parser.option("-i").has_value());
        REQUIRE(parser.option("-i").value() == "input.txt");
      }

      THEN("The output file is the value after -o") {
        REQUIRE(parser.option("-o").has_value());
        REQUIRE(parser.option("-o").value() == "output.txt");
      }
    }
  }
}
