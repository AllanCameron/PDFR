##---------------------------------------------------------------------------##
#' Paths to test pdfs
#'
#' A list of paths to locally stored test pdfs
#'
#' @format A list of 9 pdf files
#' \describe{
#'   \item{barcodes}{a pdf constructed in Rstudio}
#'   \item{chestpain}{a flow-chart for chest pain management}
#'   \item{pdfinfo}{information about the pdf format}
#'   \item{adobe}{an official adobe document}
#'   \item{leeds}{a table-rich local government document}
#'   \item{sams}{a document based on svg}
#'   \item{testreader}{a simple pdf test}
#'   \item{tex}{a simple tex test}
#'   \item{rcpp}{a CRAN package vignette}
#' }
#' @export
##---------------------------------------------------------------------------##

pdfr_paths <- list(
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
