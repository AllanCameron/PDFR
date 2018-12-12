#include "pdfr.h"
#include "Rex.h"

/*---------------------------------------------------------------------------*/
// Uses regex to pull out a vector of matching strings from a given string
Rex::Rex(const std::string& s, const std::string& r) :
  s(s), r(r), endof(std::sregex_iterator())
{
  w.assign(r);
  auto R = std::sregex_iterator(s.begin(), s.end(), w);
  for(auto i = R; i != endof; ++i)
  {
    matches.push_back(i->str());
    startpos.push_back(i->position());
    endpos.push_back(i->position() + i->length());
  }
  contains = !matches.empty();
}

/*---------------------------------------------------------------------------*/

bool Rex::has()
{
  return contains;
}

/*---------------------------------------------------------------------------*/

std::vector<int> Rex::pos()
{
  return startpos;
}

/*---------------------------------------------------------------------------*/

std::vector<int> Rex::ends()
{
  return endpos;
}

/*---------------------------------------------------------------------------*/

std::vector<std::string> Rex::get()
{
  return matches;
}

/*---------------------------------------------------------------------------*/

int Rex::n()
{
  return (int) matches.size();
}
