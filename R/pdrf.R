

testfiles <- list(
  barcodes = system.file("extdata", "barcodes.pdf", package = "PDFR"),
  barrets = system.file("extdata", "barrets.pdf", package = "PDFR"),
  pdfref = system.file("extdata", "pdfref.pdf", package = "PDFR"),
  poster = system.file("extdata", "poster.pdf", package = "PDFR"),
  chestpain = system.file("extdata", "chestpain.pdf", package = "PDFR"),
  pdfinfo = system.file("extdata", "pdfinfo.pdf", package = "PDFR"),
  adobe = system.file("extdata", "adobe.pdf", package = "PDFR"),
  leeds = system.file("extdata", "leeds.pdf", package = "PDFR"),
  sams = system.file("extdata", "sams.pdf", package = "PDFR"),
  ggp = system.file("extdata", "gg.pdf", package = "PDFR"),
  testreader = system.file("extdata", "testreader.pdf", package = "PDFR"),
  tex = system.file("extdata", "tex.pdf", package = "PDFR"),
  rcpp = system.file("extdata", "rcpp.pdf", package = "PDFR"),
  paris = system.file("extdata", "paris.pdf", package = "PDFR"),
  rexts = system.file("extdata", "R-exts.pdf", package = "PDFR")
)

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
#' @param font the font to be used to show text on the plot
#' @param textsize the scale of the text to be shown
#'
#' @return a ggplot
#' @export
#'
#' @examples pdfplot(testfiles$pdfref, 1)
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

