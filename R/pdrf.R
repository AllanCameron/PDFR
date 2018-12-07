
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
"testfiles"
testfiles <- list(
  barcodes = system.file("extdata", "barcodes.pdf", package = "PDFR"),
  chestpain = system.file("extdata", "chestpain.pdf", package = "PDFR"),
  pdfinfo = system.file("extdata", "pdfinfo.pdf", package = "PDFR"),
  adobe = system.file("extdata", "adobe.pdf", package = "PDFR"),
  leeds = system.file("extdata", "leeds.pdf", package = "PDFR"),
  sams = system.file("extdata", "sams.pdf", package = "PDFR"),
  testreader = system.file("extdata", "testreader.pdf", package = "PDFR"),
  tex = system.file("extdata", "tex.pdf", package = "PDFR"),
  rcpp = system.file("extdata", "rcpp.pdf", package = "PDFR")
)



#' get a file from the internet
#'
#' Returns the character string or raw vector (depending on content) from
#' a given url
#'
#' @param x a valid url
#' @param filename a name for the locally stored file (if required)
#'
#' @return either a character string or a raw vector
#' @export
#'
#' @examples internetFile("http://www.google.com")
internetFile <- function(x, filename = NULL)
{
  xloc <- tempfile();
  if (!is.null(filename)) xloc <-  filename
  writeBin(httr::GET(x)[[6]], xloc)
  res <- readBin (xloc, "raw", file.size(xloc))
  if (is.null(filename))
  {
    file.remove(xloc);
    if (!any(res == 0)) return(rawToChar(res)) else return(res)
  }
else cat("file saved to ", path.expand("~/"), filename, "\n", collapse = "")
}




#' pdfplot
#'
#' Plots the text elements from a page as a ggplot.
#' The aim is not a complete pdf rendering but to help identify elements of
#' interest in the data frame of text elements to convert to data points.
#'
#' @param pdf a valid pdf file location
#' @param page the page number to be plotted
#' @param clump the degree to which text is clumped together (integer)
#' @param textsize the scale of the text to be shown
#'
#' @return a ggplot
#' @export
#'
#' @examples pdfplot(testfiles$leeds, 1)
pdfplot <- function(pdf, page = 1, textsize = 1)
  {
    PDFR::pdfpage(pdf, page) -> x;
    x$Elements -> y;
    ggplot2::ggplot(data = y,
           ggplot2::aes(x = y$left, y = y$bottom, size = I(textsize*170*y$size/(x$Box[4] - x$Box[2]))),
           lims = x$Box ) -> G;
    G + ggplot2::geom_rect(ggplot2::aes(xmin = x$Box[1], ymin = x$Box[2],
                      xmax = x$Box[3], ymax = x$Box[4]),
                  fill = "white", colour="black", size=0.2
      ) + ggplot2::geom_text(ggplot2::aes(label = y$text), hjust = 0, vjust = 0,
      ) + ggplot2::coord_equal(
      ) + ggplot2::scale_size_identity();
  }

# "\\\\XGGC.SCOT.NHS.UK\\GGCData\\FolderRedirects\\GRI5\\cameral931\\My Documents\\suicide.pdf"
