#ifndef PDFR_REX
#define PDFR_REX

#include "pdfr.h"


class Rex
{
public:
  Rex(const std::string& s, const std::string& r);
  bool has();
  std::vector<int> pos();
  std::vector<int> ends();
  std::vector<std::string> get();
  int n();

private:
  std::string s;
  std::string r;
  std::sregex_iterator endof;
  std::regex w;
  std::vector<std::string> matches;
  std::vector<int> startpos;
  std::vector<int> endpos;
  bool contains;
};


#endif
