##---------------------------------------------------------------------------##
##                                                                           ##
##  PDFR main R file                                                         ##
##                                                                           ##
##  Copyright (C) 2018 by Allan Cameron                                      ##
##                                                                           ##
##  Permission is hereby granted, free of charge, to any person obtaining    ##
##  a copy of this software and associated documentation files               ##
##  (the "Software"), to deal in the Software without restriction, including ##
##  without limitation the rights to use, copy, modify, merge, publish,      ##
##  distribute, sublicense, and/or sell copies of the Software, and to       ##
##  permit persons to whom the Software is furnished to do so, subject to    ##
##  the following conditions:                                                ##
##                                                                           ##
##  The above copyright notice and this permission notice shall be included  ##
##  in all copies or substantial portions of the Software.                   ##
##                                                                           ##
##  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  ##
##  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               ##
##  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   ##
##  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY     ##
##  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,     ##
##  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE        ##
##  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                   ##
##                                                                           ##
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

#' pdfpage
#'
#' Returns contents of a pdf page
#'
#' @param pdf a valid pdf file location
#' @param page the page number to be plotted
#'
#' @return a list containing data frames
#' @export
#'
#' @examples pdfpage(testfiles$leeds, 1)
pdfpage <- function(pdf, page){
  if(class(pdf) == "raw") {
    return(.pdfpageraw(pdf, page));
    } else {
    return(.pdfpage(pdf, page));
    }
}


#' Get a pdf's xref table as an R dataframe
#'
#' @param pdf a valid pdf file location or raw data vector
#'
#' @return a data frame showing the positions of each object
#' @export
#'
#' @examples get_xref(testfiles$leeds)
get_xref <- function(pdf){
  if(class(pdf) == "raw") {
    return(.get_xrefraw(pdf));
    } else {
    return(.get_xref(pdf));
    }
}


#' Get the contents of a pdf object
#'
#' @param pdf a valid pdf file location
#' @param number the object number
#'
#' @return a named vector of the dictionary and stream of the pdf object
#' @export
#'
#' @examples get_object(testfiles$leeds, 1)
get_object <- function(pdf, number){
  if(class(pdf) == "raw") {
    return(.get_objraw(pdf, number));
    } else {
    return(.get_obj(pdf, number));
    }
}


#' pdfplot
#'
#' Plots the text elements from a page as a ggplot.
#' The aim is not a complete pdf rendering but to help identify elements of
#' interest in the data frame of text elements to convert to data points.
#'
#' @param pdf a valid pdf file location
#' @param page the page number to be plotted
#' @param textsize the scale of the text to be shown
#'
#' @return a ggplot
#' @export
#'
#' @examples pdfplot(testfiles$leeds, 1)
pdfplot <- function(pdf, page = 1, textsize = 1)
  {
  pdfpage(pdf, page) -> x;
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

