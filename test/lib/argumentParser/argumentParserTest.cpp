#include "../../../lib/argumentParser/ArgumentParser.h"
#include <catch2/catch_all.hpp>

SCENARIO("ArgumentParser works correctly") {
  GIVEN("A valid command line input") {
    char *argv[] = {"program", "-i", "input.txt", "-o", "output.txt"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    WHEN("Parsing the arguments") {
      openfranko::lib::argumentParser::ArgumentParser parser(argc, argv);

      THEN("The input file should be correctly parsed") {
        REQUIRE(parser.getCmdOption("-i").has_value());
        REQUIRE(parser.getCmdOption("-i").value() == "input.txt");
      }

      THEN("The output file should be correctly parsed") {
        REQUIRE(parser.getCmdOption("-o").has_value());
        REQUIRE(parser.getCmdOption("-o").value() == "output.txt");
      }
    }
  }
}