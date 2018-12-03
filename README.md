# PDFR

##An R package to extract usable text from portable document format (pdf) files.

The pdf file format is an excellent way of sharing documents so that they
appear the same independent of the platform on which they are viewed, and
the file is structured specifically to achieve this goal. As a result,
it is an extremely common format for sharing information electronically.

However, this can be problematic for the data scientist because the pdf file 
format, while ubiquitous, does not lend itself to sharing machine-readable 
content in the way that other common file formats used on the web can, such as 
xml, json, html and csv.

Some R packages already exist to help extract information from pdfs. In
particular, [pdftools](https://ropensci.org/blog/2016/03/01/pdftools-and-jeroen/) provides an R interface to some of Poppler's pdf tools, and can quickly and 
reliably extract text wholesale from pdf. Although this preserves much of the 
text layout on a page, the precise locations, fonts and sizes of the text 
elements is lost, reducing the options for extracting semantic information. 
That was the reason for the creation of the PDFR package.

PDFR is mostly written in C++. Rather than being based on an existing library
such as [xpdf](https://www.xpdfreader.com/) or [Poppler](https://poppler.freedesktop.org/), it was written from scratch with the specific goal of making text extraction easier for R users. Most of the design is new, an attempt to implement the text extraction elements of the pdf standard 
[ISO 32000](https://www.iso.org/standard/51502.html), though I also learned a
lot from excellent existing open-source libraries such as [pdfjs](https://mozilla.github.io/pdf.js/).

The only external C++ library used is [miniz](https://github.com/richgel999/miniz), which is used to implement the deflate decompression algorithm. It is included 
in the source code as a single pair of header / implementation source files. 
I chose this in preference to the ubiquitous zlib for the simple reason that 
I wrote some of the code on computers where zlib was not installed, and this 
seemed the most portable option for development.

Clearly, the package would not exist without the excellent [Rcpp](http://www.rcpp.org/) package. Much of the pdf parsing would take too 
long to do in R, but having the facility to write C++ extensions makes pdf 
parsing feasible, and even pretty quick in some cases.




