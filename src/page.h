#ifndef PDFR_PAGE
#define PDFR_PAGE

#include "pdfr.h"

struct page
{
  int pagenumber, objectnumber, parent;
  dictionary header, resources, fonts, realfonts;
  std::string contentstring, xobjstring;
  std::map<std::string, std::string> XObjects;
  std::vector<int> resourceobjs, contents;
  std::vector<double> bleedbox, cropbox, mediabox, trimbox, artbox, minbox;
  double rotate;
  std::vector<std::string> fontnames;
  std::vector<bool> hasUnicodeMap;
  std::vector<dictionary> UnicodeMaps, fontrefs;
  std::map<std::string, std::map<uint16_t,int>> WidthTables;
  std::map<std::string, font> fontmap;
  void parseXObjStream(document& d);
  page(document& pdfdoc, int pagenum);
};

#endif
