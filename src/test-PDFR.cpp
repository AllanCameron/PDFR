//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR unit testing file                                                   //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include<memory>
#include <testthat.h>
#include "utilities.h"
#include "dictionary.h"
#include "pdfr.h"

//---------------------------------------------------------------------------//

using namespace std;

//---------------------------------------------------------------------------//
// Most of the tests we will be running require actual pdf files. However, we
// can directly test the global utility functions and the Dictionary class
// without needing an actual pdf file. To do this, we will define some variables
// in a "TestItems" namespace. We will use these in our unit tests using the
// Catch framework.

namespace TestItems
{
string test_target("I'm not a pheasant plucker, I'm a pheasant plucker's son");
vector<string> test_multicarve1 {" not a pheasant ", " a pheasant "};
vector<string> test_multicarve2 {"not", "pheasant", "I'm", "pheasant"};

auto test_bytes = vector<uint8_t>   { 0x01, 0xAB, 0xEF, 0x2A };
auto test_rawchar = vector<RawChar> { 0x01AB, 0xEF2A };
auto test_hello_rawchar = vector<RawChar> { 0x48, 0x65, 0x6c, 0x6c, 0x6f};

string test_hexstring        = "01ABEF2A",
       test_broken_hexstring = "01ABEX F2A";

vector<int>   test_ints   = {1, 2, 31},
              test_sortby = {3, 2, 0, 4, 1},
              test_order  = {2, 4, 1, 0, 3};
vector<float> test_floats = {3.14, 2.72, 1.4};

string reference_string = "<</Refs 1 0 R 2 0 R 31 5 R>>";
string test_dict_string =
" <</A Success/Ref 1 0 R 2 0 R 31 5 R/Dict <</Subdict Success>>\
/SomeInts [1 2 31]/SplitBy /r/n(A line break)\
/SomeFloats [3.14 2.72 1.4]/Length 15>>\
stream\r\nNow in a stream\r\nendstream";
string full_pdf_string =
"%PDF-1.1\r\n%¥±ë\r\n\r\n1 0 obj\r\n  << /Type /Catalog\r\n     \
/Pages 2 0 R\r\n  >>\r\nendobj\r\n\r\n2 0 obj\r\n  << /Type /Pages\r\n     \
/Kids [3 0 R]\r\n     /Count 1\r\n     /MediaBox [0 0 300 144]\r\n  \
>>\r\nendobj\r\n\r\n3 0 obj\r\n  <<  /Type /Page\r\n      \
/Parent 2 0 R\r\n      /Resources\r\n << /Font\r\n\
<< /F1\r\n               << /Type /Font\r\n                  \
/Subtype /Type1\r\n                  /BaseFont /Times-Roman\r\n               \
>>\r\n           >>\r\n       >>\r\n      /Contents 4 0 R\r\n  \
>>\r\nendobj\r\n\r\n4 0 obj\r\n  << /Length 55 >>\r\nstream\r\n  BT\r\n    \
/F1 18 Tf\r\n    0 0 Td\r\n    (Hello World) Tj\r\n  \
ET\r\nendstream\r\nendobj\r\n\r\nxref\r\n0 5\r\n\
0000000000 65535 f \r\n\
0000000021 00000 n \r\n\
0000000086 00000 n \r\n\
0000000195 00000 n \r\n\
0000000473 00000 n \r\n\
trailer\r\n  <<  /Root 1 0 R\r\n      /Size 5\r\n  \
>>\r\nstartxref\r\n592\r\n%%EOF";
vector<uint8_t> pdf_bytes(full_pdf_string.begin(), full_pdf_string.end());
auto ptr_to_pdf = make_shared<string>(full_pdf_string);
auto dict_from_pdf = Dictionary(ptr_to_pdf, 195);
auto resources_test = dict_from_pdf.GetDictionary_("/Resources");
auto font_test = resources_test.GetDictionary_("/Font");
auto subfont_test = font_test.GetDictionary_("/F1");
auto get_font_name = subfont_test.GetString_("/BaseFont");

Dictionary test_dictionary = Dictionary(make_shared<string>(test_dict_string));

vector<char> test_chars = {'c', 'e', 'b', 'a', 'd'},
             test_alpha = {'a', 'b', 'c', 'd', 'e'};
}

//---------------------------------------------------------------------------//
// We use this namespace here to prevent polluting the global namespace

using namespace TestItems;

//---------------------------------------------------------------------------//
// The tests are laid out in groups of assertions under named "contexts".
// Within each context we use named "test_that" subgroups, each containing
// one or more assertions.

context("utilities.h")
{
  test_that("Order and SortBy work as expected.")
  {
    expect_true(Order(test_chars) == test_order);
    expect_true(test_alpha == SortBy(test_chars, test_sortby));
  }

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
    expect_true(ParseReferences(reference_string) == test_ints);
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

//---------------------------------------------------------------------------//
// Tests the public interface to Dictionary. The private methods are also
// tested by implication, since the private methods are used to build the
// Dictionary structure, and an error in Dictionary construction will lead to
// an error in retrieving data from the created Dictionary

context("dictionary.h")
{
  test_that("Dictionary can be created successfully.")
  {
    expect_true(test_dictionary.GetString_("/A") == "Success");
  }

  test_that("Dictionary entries are read correctly.")
  {
    expect_true(test_dictionary.GetReference_("/Ref") == 1);
    expect_true(test_dictionary.GetReferences_("/Ref") == test_ints);
    expect_true(test_dictionary.GetInts_("/SomeInts") == test_ints);
    expect_true(test_dictionary.GetFloats_("/SomeFloats") == test_floats);
    expect_true(
      test_dictionary.GetDictionary_("/Dict").GetString_("/Subdict") ==
      "Success");
    expect_true(get_font_name == string("/Times-Roman"));
  }
}
