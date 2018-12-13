//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Rex implementation file                                             //
//                                                                           //
//  Copyright (C) 2018 by Allan Cameron                                      //
//                                                                           //
//  Permission is hereby granted, free of charge, to any person obtaining    //
//  a copy of this software and associated documentation files               //
//  (the "Software"), to deal in the Software without restriction, including //
//  without limitation the rights to use, copy, modify, merge, publish,      //
//  distribute, sublicense, and/or sell copies of the Software, and to       //
//  permit persons to whom the Software is furnished to do so, subject to    //
//  the following conditions:                                                //
//                                                                           //
//  The above copyright notice and this permission notice shall be included  //
//  in all copies or substantial portions of the Software.                   //
//                                                                           //
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  //
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               //
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   //
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY     //
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,     //
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE        //
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                   //
//                                                                           //
//---------------------------------------------------------------------------//


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
