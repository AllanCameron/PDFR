#include <testthat.h>
#include "pdfr.h"

int twoPlusTwo() {
  return 2 + 2;
}

// Initialize a unit test context. This is similar to how you
// might begin an R test file with 'context()', expect the
// associated context should be wrapped in braced.
context("Sample unit tests")
{
  test_that("two plus two equals four")
  {
    expect_true(twoPlusTwo() == 4);
  }
}
