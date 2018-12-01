#ifndef PDFR_COREFONTS
#define PDFR_COREFONTS

#include "pdfr.h"

font getCourier();
font getCourierBold();
font getCourierBO();
font getCourierOblique();
font getHelvetica();
font getHelveticaBold();
font getHelveticaBO();
font getHelveticaOblique();
font getSymbol();
font getTimesBold();
font getTimesBI();
font getTimesItalic();
font getTimesRoman();
font getDingbats();
std::map<int, std::string> ligatures();

#endif
