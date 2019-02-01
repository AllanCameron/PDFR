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
##---------------------------------------------------------------------------##
testfiles <- list(
  barcodes   =  system.file("extdata", "barcodes.pdf",   package = "PDFR"),
  chestpain  =  system.file("extdata", "chestpain.pdf",  package = "PDFR"),
  pdfinfo    =  system.file("extdata", "pdfinfo.pdf",    package = "PDFR"),
  adobe      =  system.file("extdata", "adobe.pdf",      package = "PDFR"),
  leeds      =  system.file("extdata", "leeds.pdf",      package = "PDFR"),
  sams       =  system.file("extdata", "sams.pdf",       package = "PDFR"),
  testreader =  system.file("extdata", "testreader.pdf", package = "PDFR"),
  tex        =  system.file("extdata", "tex.pdf",        package = "PDFR"),
  rcpp       =  system.file("extdata", "rcpp.pdf",       package = "PDFR"))

##---------------------------------------------------------------------------##
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
##---------------------------------------------------------------------------##
internetFile <- function(x, filename = NULL)
{
  xloc <- tempfile();
  if (!is.null(filename))
  {
    xloc <-  filename
  }
  writeBin(httr::GET(x)[[6]], xloc)
  res <- readBin(xloc, "raw", file.size(xloc))
  if (is.null(filename))
  {
    file.remove(xloc);
    if (!any(res == 0))
    {
      return(rawToChar(res))
    }
    else
    {
      return(res)
    }
  }
  else
  {
    cat("file saved to ", path.expand("~/"), filename, "\n", collapse = "")
  }
}

##---------------------------------------------------------------------------##
#' pdfpage
#'
#' Returns contents of a pdf page
#'
#' @param pdf a valid pdf file location
#' @param page the page number to be extracted
#'
#' @return a list containing data frames
#' @export
#'
#' @examples pdfpage(testfiles$leeds, 1)
##---------------------------------------------------------------------------##
pdfpage <- function(pdf, page)
{
  if(class(pdf) == "raw")
  {
    x <- .pdfpageraw(pdf, page)
  }
  if(class(pdf) == "character" & length(pdf) == 1 & grepl("[.]pdf$", pdf))
  {
    x <- .pdfpage(pdf, page)
  }
  if((class(pdf) != "raw"       & class(pdf) != "character") |
     (class(pdf) == "character" & length(pdf) > 1)           |
     (class(pdf) == "character" & !grepl("[.]pdf$", pdf))    )
  {
    stop("pdfpage requires a single path to a valid pdf or a raw vector.")
  }
  x$Elements$text <- unlist(lapply(x$glyphs,
    function(z)
    {
      paste(intToUtf8(z, multiple = TRUE), collapse = "")
    }
  ))
  x$glyphs <- NULL
  x$Elements <- x$Elements[order(-x$Elements$bottom, x$Elements$left),]
  x$Elements$left <- round(x$Elements$left, 1)
  x$Elements$right <- round(x$Elements$right, 1)
  x$Elements$bottom <- round(x$Elements$bottom, 1)
  x$Elements$size <- round(x$Elements$size, 1)
  rownames(x$Elements) <- seq_along(x$Elements[[1]])

  return(x)
}

##---------------------------------------------------------------------------##
#' Get a pdf's xref table as an R dataframe
#'
#' @param pdf a valid pdf file location or raw data vector
#'
#' @return a data frame showing the bytewise positions of each object in the pdf
#' @export
#'
#' @examples get_xref(testfiles$leeds)
##---------------------------------------------------------------------------##
get_xref <- function(pdf)
{
  if(class(pdf) == "raw")
  {
    return(.get_xrefraw(pdf));
  }
  else
  {
    return(.get_xref(pdf));
  }
}

##---------------------------------------------------------------------------##
#' Get the contents of a pdf object
#'
#' Returns a list consisting of a named vector representing key:value pairs
#' in a specified object. It also contains any stream data associated with
#' the object.
#'
#' @param pdf a valid pdf file location
#' @param number the object number
#'
#' @return a named vector of the dictionary and stream of the pdf object
#' @export
#'
#' @examples get_object(testfiles$leeds, 1)
##---------------------------------------------------------------------------##
get_object <- function(pdf, number)
{
  if(class(pdf) == "raw")
  {
    return(.get_objraw(pdf, number))
  }
  else
  {
    return(.get_obj(pdf, number))
  }
}

##---------------------------------------------------------------------------##
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
##---------------------------------------------------------------------------##
pdfplot <- function(pdf, page = 1, textsize = 1)
{
  x <- pdfpage(pdf, page)
  y <- x$Elements
  G <- ggplot2::ggplot(data = y, ggplot2::aes(x = y$left, y = y$bottom,
                       size = I(textsize*170 * y$size / (x$Box[4] - x$Box[2]))),
                       lims = x$Box )
  G + ggplot2::geom_rect(ggplot2::aes(xmin = x$Box[1], ymin = x$Box[2],
                                      xmax = x$Box[3], ymax = x$Box[4]),
                         fill = "white", colour = "black", size = 0.2
  ) + ggplot2::geom_text(ggplot2::aes(label = y$text), hjust = 0, vjust = 0
  ) + ggplot2::coord_equal(
  ) + ggplot2::scale_size_identity(
  )
}

##---------------------------------------------------------------------------##
#' Return map of glyphs from a page
#'
#' Used mainly for debugging, this function returns an R dataframe, one row for
#' each byte that may be used as a glyph. It shows the unicode number of
#' each interpreted glyph, as well as its width in text space.
#'
#' @param pdf a valid pdf file location
#' @param page the page number from which to extract glyphs
#'
#' @return a dataframe of all entries of font encoding tables with width mapping
#' @export
#'
#' @examples getglyphmap(testfiles$leeds, 1)
##---------------------------------------------------------------------------##
getglyphmap <- function(pdf, page = 1)
{
  return(.getglyphmap(pdf, page))
}

##---------------------------------------------------------------------------##
#' Show ggplot of page with segmentation lines
#'
#' A way to assess the segmentation algorithm visually
#'
#' @param pdf a valid pdf file location
#' @param page the page number from which to extract glyphs
#' @param textsize the size of the text to be shown on the plot
#'
#' @return no return - prints a ggplot
#' @export
#'
#' @examples segplot(testfiles$leeds, 1)
##---------------------------------------------------------------------------##
segplot <- function(pdf, page = 1, textsize = 1)
{
  x       <- pdfpage(pdf, page)
  y       <- x$Elements
  y$top   <- y$bottom + y$size;
  ycounts <- numeric(1000);
  xcounts <- numeric(1000);
  ylines  <- numeric();
  xlines  <- numeric();
  xmax    <- x$Box[3]
  xmin    <- x$Box[1]
  ymax    <- x$Box[4]
  ymin    <- x$Box[2]
  ybins   <- ymin + 1:1000 * ((ymax - ymin)/1000)
  xbins   <- xmin + 1:1000 * ((xmax - xmin)/1000)

  for(i in 1:1000)
  {
    ycounts[i] <- length(which(y$bottom < ybins[i])) -
                  length(which(y$top < ybins[i]));
    xcounts[i] <- length(which(y$left < xbins[i]))   -
                  length(which(y$right < xbins[i]));
    if( i > 1)
    {
      if(abs(ycounts[i]- ycounts[i-1]) > 3)
      {
        ylines <- c(ylines, ybins[i])
      }
      if(abs(xcounts[i] - xcounts[i-1]) > 3)
      {
        xlines <- c(xlines, xbins[i])
      }
    }
  }

  G <-  ggplot2::ggplot(data = y, ggplot2::aes(x = y$left, y = y$bottom,
                  size = I(textsize*170 * y$size / (x$Box[4] - x$Box[2]))),
                  lims = x$Box )
  G + ggplot2::geom_rect(ggplot2::aes(xmin = x$Box[1], ymin = x$Box[2],
                    xmax = x$Box[3], ymax = x$Box[4]),
                fill = "white", colour="black", size=0.2
  ) + ggplot2::geom_text(ggplot2::aes(label = y$text), hjust = 0, vjust = 0
  ) + ggplot2::coord_equal(
  ) + ggplot2::scale_size_identity(
  ) + ggplot2::geom_hline(yintercept = ylines, colour = "red"
  ) + ggplot2::geom_vline(xintercept = xlines, colour = "red")
}

##---------------------------------------------------------------------------##
#' pagestring
#'
#' Returns contents of a pdf page description program
#'
#' @param pdf a valid pdf file location
#' @param page the page number to be extracted
#'
#' @return a single string containing the page description program
#' @export
#'
#' @examples getpagestring(testfiles$leeds, 1)
##---------------------------------------------------------------------------##
getpagestring <- function(pdf, page)
{
  if(class(pdf) == "raw")
  {
    x <- .pagestringraw(pdf, page)
  }
  if(class(pdf) == "character" & length(pdf) == 1 & grepl("[.]pdf$", pdf))
  {
    x <- .pagestring(pdf, page)
  }
  if((class(pdf) != "raw"       & class(pdf) != "character") |
     (class(pdf) == "character" & length(pdf) > 1)           |
     (class(pdf) == "character" & !grepl("[.]pdf$", pdf))    )
  {
    stop("pdfpage requires a single path to a valid pdf or a raw vector.")
  }
  return(x)
}


##---------------------------------------------------------------------------##
#' pdfdoc
#'
#' Returns contents of all pdf pages
#'
#' @param pdf a valid pdf file location
#'
#' @return a data frame of all text elements in a document
#' @export
#'
#' @examples pdfdoc(testfiles$leeds)
##---------------------------------------------------------------------------##
pdfdoc <- function(pdf)
{
  if(class(pdf) == "raw")
  {
    x <- .pdfdocraw(pdf)
  }
  if(class(pdf) == "character" & length(pdf) == 1 & grepl("[.]pdf$", pdf))
  {
    x <- .pdfdoc(pdf)
  }
  if((class(pdf) != "raw"       & class(pdf) != "character") |
     (class(pdf) == "character" & length(pdf) > 1)           |
     (class(pdf) == "character" & !grepl("[.]pdf$", pdf))    )
  {
    stop("pdfdoc requires a single path to a valid pdf or a raw vector.")
  }
  x$Elements$text <- unlist(lapply(x$glyphs,
    function(z)
    {
      paste(intToUtf8(z, multiple = TRUE), collapse = "")
    }
  ))
  x <- x$Elements
  x <- x[order(x$page, -x$bottom, x$left),]
  x$left <- round(x$left, 1)
  x$right <- round(x$right, 1)
  x$bottom <- round(x$bottom, 1)
  x$size <- round(x$size, 1)
  rownames(x) <- seq_along(x[[1]])

  return(x)
}

