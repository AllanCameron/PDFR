context("test-pdrf")
pdfpage(testfiles[[1]], 1, FALSE, FALSE)$Elements -> barcodes
pdfpage(testfiles[[2]], 1, FALSE, FALSE)$Elements -> chestpain
pdfpage(testfiles[[3]], 1, FALSE, FALSE)$Elements -> pdfinfo
pdfpage(testfiles[[4]], 1, FALSE, FALSE)$Elements -> adobe
pdfpage(testfiles[[5]], 1, FALSE, FALSE)$Elements -> leeds
pdfpage(testfiles[[6]], 1, FALSE, FALSE)$Elements -> sams
pdfpage(testfiles[[7]], 1, FALSE, FALSE)$Elements -> testreader
pdfpage(testfiles[[8]], 3, FALSE, FALSE)$Elements -> tex
pdfpage(testfiles[[9]], 1, FALSE, FALSE)$Elements -> rcpp

test_that("Encoding works",
{
  expect_match(chestpain$text[1], "ACUTE CARDIAC CHEST PAIN GUIDELINES")
  # expect_match(paste0(adobe$text, collapse = ""), "PDF BOOKMARK SAMPLE")
})

test_that("Ligatures are properly encoded",
{
  expect_match(paste(tex$text, collapse = " "), "fi")
})

test_that("Widths are non-zero",
{
  expect_gt(min(testreader$right - testreader$left), 95)
})

test_that("Whole document can be parsed",
{
  expect_silent(pdfdoc(testfiles[[2]]))
})


