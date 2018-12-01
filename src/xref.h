#ifndef PDFR_XREF
#define PDFR_XREF

#include "pdfr.h"

struct xrefrow { int object, startbyte, stopbyte, in_object; };

/*---------------------------------------------------------------------------*/

class xref
{
private:
  std::string file;
  std::vector<int> Xreflocations;
  std::vector<std::string>  Xrefstrings;
  std::vector<bool> XrefsAreStreams;
  void locateXrefs();
  void xrefstrings();
  void xrefIsstream();
  void getTrailer();
  void xrefFromStream(int xrefloc);
  void xrefFromString(std::string& s);
  void buildXRtable();
  void findEnds();
  std::map<int, xrefrow> xreftab;
  std::vector<int> objenum;
  dictionary TrailerDictionary;

public:
  document* d;
  xref(){};
  xref(document& doc);
  dictionary trailer() {return TrailerDictionary;}
  size_t getStart(int objnum);
  size_t getEnd(int objnum);
  bool isInObject(int objnum);
  size_t inObject(int objnum);
  std::vector<int> getObjects();
  bool objectExists(int objnum);
};

#endif
