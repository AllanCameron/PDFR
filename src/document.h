#ifndef PDFR_DOCUMENT
#define PDFR_DOCUMENT

#include "pdfr.h"
#include "dictionary.h"

class document
{
public:
  std::ifstream *fileCon;
  bool linearized, encrypted;
  int filesize;
  std::string file, filestring;
  std::vector<dictionary> pageheaders;
  std::vector<uint8_t> filekey;
  std::map <int, object_class> objects;
  dictionary trailer;
  dictionary catalogue;
  object_class pagedir;
  xref Xref;

  // member functions
  void get_file();
  std::string subfile(int startbyte, int len);
  void getCatalogue();
  void getPageDir();
  void isLinearized();
  std::vector<page> getPages(std::vector<dictionary> pageheaders);
  page getPage(int pagenum);
  object_class getobject(int objnum);
  std::vector<uint8_t> get_cryptkey();
  document(const std::string& filename);
  document();
  std::vector<int> expandKids(std::vector<int> objnums);
  std::vector<int> expandContents(std::vector<int> objnums);
  void getPageHeaders();
};

#endif
