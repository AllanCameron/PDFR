#include <testthat.h>
#include "pdfr.h"

using namespace std;

string test_target("I'm not a pheasant plucker, I'm a pheasant plucker's son");
string test_left = "I'm", test_right = "plucker";
vector<string> test_multicarve = vector<string> {" not a pheasant ", " a pheasant "};

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
    expect_true(MultiCarve(test_target, test_left, test_right) == test_multicarve);
  }
}
