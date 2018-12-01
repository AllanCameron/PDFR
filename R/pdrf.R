library(trak)
library(devtools)
library(Rcpp)
library(Rcpp11)
library(ggplot2)
library(extrafont)
library(grid)

Sys.setenv("PKG_CXXFLAGS" = "-std=c++11")

assignInNamespace("version_info",
                  c(devtools:::version_info,
                    list("3.5" = list(version_min = "3.3.0",
                                      version_max = "99.99.99",
                                      path = "bin"))), "devtools")

pdfpath    <- "\\\\XGGC.SCOT.NHS.UK\\GGCData\\FolderRedirects\\" %>%
              paste0("GRI5\\cameral931\\My Documents\\")

gricurrent <- pdfpath %>% paste0("gricurrent.pdf")
cppref     <- pdfpath %>% paste0("cppref.pdf")
w46        <- pdfpath %>% paste0("griwd46.pdf")
barcodes   <- pdfpath %>% paste0("barcodes.pdf")
fortuna    <- pdfpath %>% paste0("fortuna.pdf")
barrets    <- pdfpath %>% paste0("barrets.pdf")
pdfref     <- pdfpath %>% paste0("pdfref.pdf")
poster     <- pdfpath %>% paste0("Ellie poster1.pdf")
Novel      <- pdfpath %>% paste0("Novel.pdf")
chestpain  <- pdfpath %>% paste0("chest pain.pdf")
pdfinfo    <- pdfpath %>% paste0("pdfinfo.pdf")
adobe      <- pdfpath %>% paste0("adobe.pdf")
yukon      <- pdfpath %>% paste0("yukon.pdf")
dummy      <- pdfpath %>% paste0("dummy.pdf")
leeds      <- pdfpath %>% paste0("leeds.pdf")
sams       <- pdfpath %>% paste0("sams.pdf")
audition   <- pdfpath %>% paste0("audition.pdf")
sam        <- pdfpath %>% paste0("SAM Scotland 2018-5.pdf")
ggp        <- pdfpath %>% paste0("gg.pdf")
testreader <- pdfpath %>% paste0("testreader.pdf")
sample     <- pdfpath %>% paste0("sample.pdf")
tex        <- pdfpath %>% paste0("tex.pdf")
rcpp       <- pdfpath %>% paste0("rcpp.pdf")
paris      <- pdfpath %>% paste0("paris.pdf")

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
#' @examples pdfplot(apdf, 1)
pdfplot <- function(pdf, page = 1, font = "Arial", textsize = 1)
  {
    PDFR::pdfpage(pdf, page) -> x;
    x$Elements -> y;
    ggplot(data = y,
           aes(x = left, y = bottom, size = I(textsize*170*size/(x$Box[4] - x$Box[2]))),
           lims = x$Box ) -> G;
    G + geom_rect(aes(xmin = x$Box[1], ymin = x$Box[2],
                      xmax = x$Box[3], ymax = x$Box[4]),
                  fill = "white", colour="black", size=0.2
      ) + geom_text(aes(label = text), hjust = 0, vjust = 0, family = font
      ) + coord_equal(
      ) + scale_size_identity();
  }

pdfpageplot <- function(x, font = "Arial", fontsize = 1){
x$Elements -> y;
ggplot(data = y,
       aes(x = left, y = bottom, size = I(fontsize*100*size/(x$Box[4] - x$Box[2]))),
       lims = x$Box ) -> G;
G + geom_rect(aes(xmin = x$Box[1], ymin = x$Box[2],
                  xmax = x$Box[3], ymax = x$Box[4]),
              fill = "white", colour="black", size=0.2
) + geom_text(aes(label = Text), hjust = 0, vjust = 0, family = font
) + coord_equal(
) + scale_size_identity();
}




