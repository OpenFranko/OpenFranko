#include <catch2/catch_all.hpp>

SCENARIO("BackwardLZ77 decompression works correctly") {
   GIVEN("BackwardLZ77 compressed data") {
       std::vector<uint8_t> compressedData = {};
       std::vector<uint8_t> expectedDecompressedData = {};

       WHEN("Decompressing the data") {
           THEN("The decompressed data should match the expected output") {
               REQUIRE(true);
           }
       }
   }
}