#ifndef PDFR_OC
#define PDFR_OC

#include "pdfr.h"

class object_class
{
private:
  int number, startpos;
  dictionary header;
  std::string stream;
  std::vector<int> Kids, Contents;
  bool has_stream, has_kids, has_contents;
  void objectHasKids();
  void objectHasContents();
  void findKids();
  void findContents();

public:
  bool hasStream();
  std::string getStream();
  bool hasKids();
  std::vector<int> getKids();
  bool hasContents();
  std::vector<int> getContents();

  dictionary getDict();
  object_class(document& d, int objnum);
  object_class(document& d, std::string str, int objnum);
  object_class(){};
};

#endif
