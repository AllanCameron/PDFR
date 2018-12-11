# PDFR 

## An R package to extract usable text from portable document format (pdf) files. 
This package was created with the aim of aiding data scientists who
use R and who need the ability to extract data from files in pdf format. To
install in its current format, type 
```
devtools::install_github("AllanCameron/PDFR") 
``` 
Please note, this may take up to five minutes to complete as the source files 
have to be compiled on your computer as part of the installation. 

## Motivation 
Extracting useful data from pdf is difficult for two reasons. Firstly, the pdf 
format primarily consists of binary data, which is laid out in such a way as to 
provide quick random access to *objects* as required by a pdf reader. The text 
elements as seen on the page are usually encoded in a binary stream within the 
document. Even when the binary stream is decoded, the text items exist as 
individual elements within a page description program, which has to be parsed 
before the text can be extracted. It is therefore not a trivial matter to extract 
the "raw text" from a pdf file into a format in which it can be read by R, though 
there exist some excellent tools that can do this quickly. In particular,
[pdftools](https://ropensci.org/blog/2016/03/01/pdftools-and-jeroen/) provides
an R interface to some of Poppler's pdf tools, and can quickly and reliably
extract text wholesale from pdf. 

The second problem is that, unlike some other common file types used to exchange 
information on the internet (e.g. html, xml, csv, JSON), the raw text extracted 
from a pdf does not have a fixed structure to provide semantic information about 
the data to allow it to be processed easily by a data scientist. 

The mismatch between the fact that humans can read data from pdfs so easily yet 
the format is so difficult to convert into machine-readable data is explained by 
the fact that humans use the structure of the page layout to provide the semantic 
context to the data. When the structure is lost (as it often is with copy and 
pasting from PDF), it becomes very difficult for a human reader to interpret. 

The idea behind PDFR is to try to extract raw text, but to also use the positioning 
and formatting data from the extracted text to reconstruct some of the semantic 
content that would otherwise be lost. For example, identifying and grouping letters 
into words, words into paragraphs or into tables. 

Ultimately, to extract useful data, the user will need the option to control how and 
to what extent text elements are grouped. For example, they may need the fine 
control of having every letter's position on the page (e.g. to accurately reconstruct 
a part of the document on a plot), or may wish to extract a corpus of plain text from 
a book as a set of paragraphs or even whole pages.  

PDFR is mostly written in C++. Rather than being based on an existing library such as 
[xpdf](https://www.xpdfreader.com/) or [Poppler](https://poppler.freedesktop.org/), it 
was written from scratch with the specific goal of making text extraction easier for 
R users. Most of the design is new, an attempt to implement the text extraction elements 
of the pdf standard [ISO 32000](https://www.iso.org/standard/51502.html), though I also
learned a lot from excellent existing open-source libraries such as
[pdfjs](https://mozilla.github.io/pdf.js/). 

The only external C++ library used is [miniz](https://github.com/richgel999/miniz), which 
is used to implement the deflate decompression algorithm. It is included in the source 
code as a single pair of header / implementation source files. I chose this in preference 
to the ubiquitous zlib for the simple reason that I wrote some of the code on computers
where zlib was not installed, and this seemed the most portable option for development. 

Clearly, the package would not exist without the excellent [Rcpp](http://www.rcpp.org/) 
package. Much of the pdf parsing would take too long to do in R, but having the facility to 
write C++ extensions makes pdf parsing feasible, and even pretty quick in some cases. 
