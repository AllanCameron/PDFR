#ifndef PDFR_STREAMS
#define PDFR

#include "pdfr.h"

bool isFlateDecode(const std::string& filestring, int startpos);

std::string FlateDecode(const std::string& s);

std::string getStreamContents(document& d, const std::string& filestring,
                              int objstart);

bool objHasStream(const std::string& filestring, int objectstart);

bool isObject(const std::string& filestring, int objectstart);

std::string objectPreStream(const std::string& filestring, int objectstart);

std::vector<std::vector<int> >
plainbytetable(std::vector<int> V, std::vector<int> ArrayWidths);

std::vector<std::vector<int>>
decodeString(document& d, const std::string& filestring, int objstart);

#endif

