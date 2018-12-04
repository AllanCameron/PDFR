library(devtools)
library(Rcpp)
library(Rcpp11)
library(ggplot2)
library(httr)
library(magrittr)
library(extrafont)
library(grid)

Sys.setenv("PKG_CXXFLAGS" = "-std=c++11")

assignInNamespace("version_info",
                  c(devtools:::version_info,
                    list("3.5" = list(version_min = "3.3.0",
                                      version_max = "99.99.99",
                                      path = "bin"))), "devtools")


barcodes   <- "./inst/barcodes.pdf"
barrets    <- "./inst/barrets.pdf"
pdfref     <- "./inst/pdfref.pdf"
poster     <- "./inst/poster.pdf"
chestpain  <- "./inst/chestpain.pdf"
pdfinfo    <- "./inst/pdfinfo.pdf"
adobe      <- "./inst/adobe.pdf"
leeds      <- "./inst/leeds.pdf"
sams       <- "./inst/sams.pdf"
ggp        <- "./inst/gg.pdf"
testreader <- "./inst/testreader.pdf"
tex        <- "./inst/tex.pdf"
rcpp       <- "./inst/rcpp.pdf"
paris      <- "./inst/paris.pdf"
rexts      <- "./inst/R-exts.pdf"

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
#' @examples pdfplot(pdfref, 1)
pdfplot <- function(pdf, page = 1, font = "Arial", textsize = 1)
  {
    PDFR::pdfpage(pdf, page) -> x;
    x$Elements -> y;
    ggplot2::ggplot(data = y,
           ggplot2::aes(x = left, y = bottom, size = I(textsize*170*size/(x$Box[4] - x$Box[2]))),
           lims = x$Box ) -> G;
    G + ggplot2::geom_rect(ggplot2::aes(xmin = x$Box[1], ymin = x$Box[2],
                      xmax = x$Box[3], ymax = x$Box[4]),
                  fill = "white", colour="black", size=0.2
      ) + ggplot2::geom_text(ggplot2::aes(label = text), hjust = 0, vjust = 0, family = font
      ) + ggplot2::coord_equal(
      ) + ggplot2::scale_size_identity();
  }

pdfpageplot <- function(x, font = "Arial", fontsize = 1){
x$Elements -> y;
ggplot2::ggplot(data = y,
       ggplot2::aes(x = left, y = bottom, size = I(fontsize*100*size/(x$Box[4] - x$Box[2]))),
       lims = x$Box ) -> G;
G + ggplot2::geom_rect(ggplot2::aes(xmin = x$Box[1], ymin = x$Box[2],
                  xmax = x$Box[3], ymax = x$Box[4]),
              fill = "white", colour="black", size=0.2
) + geom_text(aes(label = Text), hjust = 0, vjust = 0, family = font
) + coord_equal(
) + scale_size_identity();
}




