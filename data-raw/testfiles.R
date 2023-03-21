testfiles <- list(
  barcodes   =  system.file("extdata", "barcodes.pdf",   package = "PDFR"),
  chestpain  =  system.file("extdata", "chestpain.pdf",  package = "PDFR"),
  pdfinfo    =  system.file("extdata", "pdfinfo.pdf",    package = "PDFR"),
  adobe      =  system.file("extdata", "adobe.pdf",      package = "PDFR"),
  leeds      =  system.file("extdata", "leeds.pdf",      package = "PDFR"),
  sams       =  system.file("extdata", "sams.pdf",       package = "PDFR"),
  testreader =  system.file("extdata", "testreader.pdf", package = "PDFR"),
  tex        =  system.file("extdata", "tex.pdf",        package = "PDFR"),
  rcpp       =  system.file("extdata", "rcpp.pdf",       package = "PDFR")
)

usethis::use_data(testfiles, overwrite = TRUE)
