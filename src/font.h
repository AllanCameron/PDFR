#ifndef PDFR_FONT
#define PDFR_FONT

#include "pdfr.h"

struct font
{
  std::string FontRef, FontName, FontID;
  std::vector<int> FontBBox;
  std::string BaseEncoding;
  GlyphMap glyphmap;

  std::map<std::string, std::string> UnicodeMap;
  bool hasUnicodeMap, hasMappings;
  std::map<uint16_t, int> Width;
  EncMap EncodingMap;
  std::map<std::string, std::string> mappings;
  void mapUnicode(dictionary& dict, document& d);
  void getEncoding(dictionary& fontref, document& d);
  void getWidthTable(dictionary& dict, document& d);
  void getCoreFont(std::string s);
  void makeGlyphTable();
  void parsewidtharray(std::string s);
  std::vector<std::pair<std::string, int>> mapString(const std::string& s);
  font(document& doc, const dictionary& fontref, const std::string& fontid);
  font(){};
};

#endif
