##---------------------------------------------------------------------------##
##                                                                           ##
##  PDFR main R file                                                         ##
##                                                                           ##
##  Copyright (C) 2018 - 2019 by Allan Cameron                               ##
##                                                                           ##
##  Licensed under the MIT license - see https://mit-license.org             ##
##  or the LICENSE file in the project root directory                        ##
##                                                                           ##
##---------------------------------------------------------------------------##

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
  if (!is.null(filename)) { xloc <-  filename }
  writeBin(httr::GET(x)[[6]], xloc)
  res <- readBin(xloc, "raw", file.size(xloc))
  if (is.null(filename))
  {
    file.remove(xloc);
    if (!any(res == 0)) return(rawToChar(res)) else return(res)
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
#' @param atomic a boolean - should each letter treated individually?
#' @param table_only a boolean - return data frame alone, as opposed to list
#'
#' @return a list containing data frames
#' @export
#'
#' @examples pdfpage(testfiles$leeds, 1, FALSE)
##---------------------------------------------------------------------------##
pdfpage <- function(pdf, page, atomic = FALSE, table_only = TRUE)
{
  if(class(pdf) == "raw")
  {
    x <- .pdfpageraw(pdf, page, atomic)
  }
  if(class(pdf) == "character" & length(pdf) == 1 & grepl("[.]pdf$", pdf[1]) &
     !grepl("/", pdf[1]))
  {
    x <- .pdfpage(paste0(path.expand("~/"), pdf), page, atomic)
  }
  if(class(pdf) == "character" & length(pdf) == 1 & grepl("[.]pdf$", pdf[1]) &
     grepl("/", pdf[1]))
  {
    x <- .pdfpage(pdf, page, atomic)
  }
  if((class(pdf) != "raw"       & class(pdf) != "character") |
     (class(pdf) == "character" & length(pdf) > 1)           |
     (class(pdf) == "character" & !grepl("[.]pdf$", pdf[1])) )
  {
    stop("pdfpage requires a single path to a valid pdf or a raw vector.")
  }
  Encoding(x$Elements$text) <- "UTF-8"
  x$Elements <- x$Elements[order(-x$Elements$bottom, x$Elements$left),]
  x$Elements$left <- round(x$Elements$left, 1)
  x$Elements$right <- round(x$Elements$right, 1)
  x$Elements$bottom <- round(x$Elements$bottom, 1)
  x$Elements$size <- round(x$Elements$size, 1)
  rownames(x$Elements) <- seq_along(x$Elements[[1]])
  .stopCpp()
  if(table_only == FALSE) return(x) else return(x$Elements)
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
#' @param atomic a boolean - should each letter treated individually?
#' @param boxes Show the calculated text bounding boxes
#' @param textsize the scale of the text to be shown
#'
#' @return a ggplot
#' @export
#'
#' @examples pdfplot(testfiles$leeds, 1)
##---------------------------------------------------------------------------##
pdfplot <- function(pdf, page = 1, atomic = FALSE, boxes = FALSE, textsize = 1)
{
  x <- pdfpage(pdf, page, atomic, FALSE)
  y <- x$Elements
  y$midx <- (y$right + y$left) / 2
  y$midy <- (y$top + y$bottom) / 2
  G <- ggplot2::ggplot(data = y, ggplot2::aes(x = y$midx, y = y$midy,
                       size = I(textsize*170 * y$size / (x$Box[4] - x$Box[2]))),
                       lims = x$Box
  ) + ggplot2::geom_rect(ggplot2::aes(xmin = x$Box[1], ymin = x$Box[2],
                                      xmax = x$Box[3], ymax = x$Box[4]),
                         fill = "white", colour = "black", size = 0.2
  ) + ggplot2::coord_equal(
  ) + ggplot2::scale_size_identity()
  if(boxes == TRUE)
  {
    G <- G + ggplot2::geom_rect(ggplot2::aes(xmin = y$left, ymin = y$bottom,
                                             xmax = y$right , ymax = y$top),
                                fill = "grey", colour = "grey",
                                size = 0.2, alpha = 0.2)
  }
  if(atomic == FALSE)
  {
    G + ggplot2::geom_text(ggplot2::aes(label = y$text),
                           hjust = 0.5, vjust = 0.5)
  }
  else
  {
    G + ggplot2::geom_text(ggplot2::aes(label = y$text),
                           hjust = 0.5, vjust = 0.5)
  }
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
  if(class(pdf) == "character" & length(pdf) == 1 & grepl("[.]pdf$", pdf[1]))
  {
    x <- .pagestring(pdf, page)
  }
  if((class(pdf) != "raw"       & class(pdf) != "character") |
     (class(pdf) == "character" & length(pdf) > 1)           |
     (class(pdf) == "character" & !grepl("[.]pdf$", pdf[1])) )
  {
    stop("pdfpage requires a single path to a valid pdf or a raw vector.")
  }
  .stopCpp()
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
  if((class(pdf) != "raw"       & class(pdf) != "character") |
     (class(pdf) == "character" & length(pdf) > 1)           |
     (class(pdf) == "character" & !grepl("[.]pdf$", pdf[1])) )
  {
    stop("pdfdoc requires a single path to a valid pdf or a raw vector.")
  }
  is_pdf <- grepl("[.]pdf$", pdf[1])
  valid_pdf_name <- (is.character(pdf) & length(pdf) == 1 & is_pdf)
  if (is.raw(pdf)) x <- .pdfdocraw(pdf)
  if (valid_pdf_name & !grepl("/", pdf[1])) pdf <- paste0(path.expand("~/"), pdf)
  x <- .pdfdoc(pdf)
  x <- x[order(x$page, -x$bottom, x$left),]
  x$left <- round(x$left, 1)
  x$right <- round(x$right, 1)
  x$bottom <- round(x$bottom, 1)
  x$size <- round(x$size, 1)
  rownames(x) <- seq_along(x[[1]])
  Encoding(x$text) <- "UTF-8"
  .stopCpp()
  return(x)
}

##---------------------------------------------------------------------------##
#' pdfboxes
#'
#' Plots the bounding boxes of text elements from a page as a ggplot.
#'
#' @param pdf a valid pdf file location
#' @param pagenum the page number to be plotted
#'
#' @return a ggplot
#' @export
#'
#' @examples pdfboxes(testfiles$leeds, 1)
##---------------------------------------------------------------------------##
pdfboxes <- function(pdf, pagenum)
{
  if(class(pdf) == "raw")
  {
    x <- .pdfboxesRaw(pdf, pagenum)
  }
  if(class(pdf) == "character" & length(pdf) == 1 & grepl("[.]pdf$", pdf[1]) &
     !grepl("/", pdf[1]))
  {
    x <- .pdfboxesString(paste0(path.expand("~/"), pdf), pagenum)
  }
  if(class(pdf) == "character" & length(pdf) == 1 & grepl("[.]pdf$", pdf[1]) &
     grepl("/", pdf[1]))
  {
    x <- .pdfboxesString(pdf, pagenum)
  }
  if((class(pdf) != "raw"       & class(pdf) != "character") |
     (class(pdf) == "character" & length(pdf) > 1)           |
     (class(pdf) == "character" & !grepl("[.]pdf$", pdf[1])) )
  {
    stop("pdfboxes requires a single path to a valid pdf or a raw vector.")
  }
  ggplot(data = x, aes(xmin = x$xmin, ymin = x$ymin, xmax = x$xmax, ymax = x$ymax,
                       fill = factor(x$box))) -> D
  print(D + geom_rect(alpha = 0.5))
  .stopCpp()
  return(x)
}
