#include <testthat.h>
#include "pdfr.h"

using namespace std;

namespace TestItems
{
string test_target("I'm not a pheasant plucker, I'm a pheasant plucker's son");
vector<string> test_multicarve1 = vector<string> {" not a pheasant ",
                                                  " a pheasant "};
vector<string> test_multicarve2 = vector<string> {"not", "pheasant", "I'm",
                                                  "pheasant"};
vector<uint8_t> test_bytes = vector<uint8_t>   { 0x01, 0xAB, 0xEF, 0x2A };
vector<RawChar> test_rawchar = vector<RawChar> { 0x01AB, 0xEF2A };
vector<RawChar> test_hello_rawchar = vector<RawChar> { 0x0048, 0x0065, 0x006c,
                                                       0x006c, 0x006f};
string test_hexstring = "01ABEF2A";
string test_broken_hexstring = "01ABEX F2A";
vector<int> test_ints {1, 2, 31};
vector<float> test_floats {3.14, 2.72, 1.4};
string test_dictionary = " <</A Success>>";
vector<string> test_keys = {"/Here", "/Are", "/The", "/Keys"};
vector<string> test_values = {"/These", "/Strings", "/Are", "/Values"};
}

using namespace TestItems;

context("Utilities")
{
  test_that("CarveOut correctly splits a string between two delimiters.")
  {
    expect_true(CarveOut("Hello there world!", "Hello", "world") == " there ");
    expect_true(CarveOut("Hello world!", "cat", "dog") == "Hello world!");
    expect_true(CarveOut("Hello world!", "Hello", "dog") == " world!");
    expect_true(CarveOut("Hello world!", "cat", " world") == "Hello");
  }

  test_that("Multicarve correctly splits strings.")
  {
    expect_true(MultiCarve(test_target, "I'm", "plucker") == test_multicarve1);
    expect_true(MultiCarve(test_target, " ", " ") == test_multicarve2);
  }

  test_that("IsAscii correctly identifies string types.")
  {
    expect_true(IsAscii("Hello World!"));
    expect_false(IsAscii("Hélló Wórld!"));
  }

  test_that("HexStrings are converted to bytes appropriately.")
  {
    expect_true(ConvertHexToBytes(test_hexstring) == test_bytes);
    expect_true(ConvertHexToBytes(test_broken_hexstring) == test_bytes);
  }

  test_that("Ints are converted to hex appropriately.")
  {
    expect_true(ConvertIntToHex(161) == string("00A1"));
    expect_true(ConvertIntToHex(100000) == string("FFFF"));
  }

  test_that("Characters are correctly identified in lexers.")
  {
    expect_true(GetSymbolType('a') == 'L');
    expect_true(GetSymbolType('7') == 'D');
    expect_true(GetSymbolType('!') == '!');
    expect_true(GetSymbolType('\t') == ' ');
  }

  test_that("Hex is converted to RawChar correctly.")
  {
    expect_true(ConvertHexToRawChar(test_hexstring) == test_rawchar);
  }

  test_that("Strings are converted to rawchar correctly.")
  {
    expect_true(test_hello_rawchar == ConvertStringToRawChar("Hello"));
  }

  test_that("References are parsed in strings.")
  {
    expect_true(ParseReferences("<</Refs 1 0 R 2 0 R 31 5 R>>") == test_ints);
  }

  test_that("Ints are parsed as expected.")
  {
    expect_true(ParseInts("01.04 2.1 A 31") == test_ints);
  }

  test_that("Floats are parsed as expected.")
  {
    expect_true(ParseFloats("vector<float> test_floats {3.14, 2.72, 1.4};") ==
      test_floats);
  }

  test_that("Read file throws if file not found.")
  {
    expect_error(GetFile("not_a_real_file.nrf"));
  }
}

context("Dictionary")
{
  test_that("Dictionary can be created successfully.")
  {
    expect_true(Dictionary(make_shared<string>(test_dictionary)).GetString("/A")
        == "Success");
  }
}
