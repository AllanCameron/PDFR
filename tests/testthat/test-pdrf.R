context("test-pdrf")
pdfpage(pdfr_paths[[1]], 1, FALSE, FALSE)$Elements -> barcodes
pdfpage(pdfr_paths[[2]], 1, FALSE, FALSE)$Elements -> chestpain
pdfpage(pdfr_paths[[3]], 1, FALSE, FALSE)$Elements -> pdfinfo
pdfpage(pdfr_paths[[4]], 1, FALSE, FALSE)$Elements -> adobe
pdfpage(pdfr_paths[[5]], 1, FALSE, FALSE)$Elements -> leeds
pdfpage(pdfr_paths[[6]], 1, FALSE, FALSE)$Elements -> sams
pdfpage(pdfr_paths[[7]], 1, FALSE, FALSE)$Elements -> testreader
pdfpage(pdfr_paths[[8]], 3, FALSE, FALSE)$Elements -> tex
pdfpage(pdfr_paths[[9]], 1, FALSE, FALSE)$Elements -> rcpp

test_that("Encoding works",
{
  expect_match(chestpain$text[1], "ACUTE CARDIAC CHEST PAIN GUIDELINES")
  expect_match(paste0(adobe$text, collapse = ""), "PDF BOOKM ARK SAM P L E")
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
  expect_silent(pdfdoc(pdfr_paths[[2]]))
})

test_that("Multiple pages can be parsed",
{
  expect_silent(pdfpage(pdfr_paths[[2]], c(1:2)))
})

test_that("Errors as expected",
{
  expect_error(pdfpage(2, c(1:2)))
})
