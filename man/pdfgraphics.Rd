% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/pdrf.R
\name{pdfgraphics}
\alias{pdfgraphics}
\title{pdfgraphics}
\usage{
pdfgraphics(file, pagenum, scale = 1)
}
\arguments{
\item{file}{a valid pdf file location}

\item{pagenum}{the page number to be plotted}

\item{scale}{Scale used for linewidth and text size. Passed to
`ggplot2::geom_text()` size parameter as scale * size/3}
}
\value{
a ggplot
}
\description{
Plots the graphical elements of a pdf page as a ggplot
}
\examples{
pdfgraphics(pdfr_paths$leeds, 1)

}
