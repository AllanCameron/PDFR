//---------------------------------------------------------------------------//
//                                                                           //
//                         PDFR C++ header file                              //
//                        (C) 2018 Allan Cameron                             //
//                                                                           //
//        This file declares generic functions, classes and templates        //
//        that are used in creating the library's other *.cpp files          //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_H
#define PDFR_H

#include<string>
#include<map>
#include<vector>
#include<algorithm>
#include<regex>
#include<chrono>
#include<cassert>
#include<iostream>
#include<fstream>
#include<thread>
#include<ratio>
#include<Rcpp.h>

typedef std::map<uint16_t, std::string> EncMap;
typedef std::vector<std::vector<int>> XRtab;
typedef std::vector<std::vector<std::vector<std::string>>> Instructionset;
typedef std::map<uint16_t, std::pair<std::string, int>> GlyphMap;

class object_class;
class page;
class xref;
class document;

#include "fileio.h"
#include "stringfunctions.h"
#include "streams.h"
#include "dictionary.h"
#include "font.h"
#include "object_class.h"
#include "xref.h"
#include "document.h"
#include "page.h"
#include "GraphicsState.h"

/*---------------------------------------------------------------------------*/

template< typename Mt, typename T >
std::vector<Mt> getKeys(std::map<Mt, T> Map)
{
  std::vector<Mt> keyvec;
  keyvec.reserve(Map.size());
  for(typename std::map<Mt, T>::iterator i = Map.begin(); i != Map.end(); i++)
  {
    keyvec.push_back(i->first);
  }
  return keyvec;
}


#endif
