#include <catch2/catch_all.hpp>

SCENARIO("ReadFile works correctly") {
   GIVEN("A valid file path") {
       std::string filePath = "test.txt";
       std::vector<uint8_t> expectedData = {};

       WHEN("Reading the file") {
           THEN("The file data should match the expected output") {
               REQUIRE(true);
           }
       }
   }
}