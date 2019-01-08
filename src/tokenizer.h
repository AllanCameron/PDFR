//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR tokenizer header file                                               //
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

#ifndef PDFR_TOKEN
#define PDFR_TOKEN

#include "pdfr.h"

using namespace std;

namespace Token
{
  enum TState
  {
    NEWSYMBOL,
    IDENTIFIER,
    NUMBER,
    RESOURCE,
    STRING,
    HEXSTRING,
    ARRAY,
    DICT,
    WAIT,
    OPERATOR
  };
};

using namespace Token;

class tokenizer
{
  size_t i;
  string s, buf;
  TState state;
  vector<pair<string, TState>> output;
  void tokenize();
  void subtokenizer(string&);
  void pushbuf(TState, TState);
  void newsymbolState();
  void resourceState();
  void identifierState();
  void numberState();
  void stringState();
  void arrayState();
  void escapeState();
  void hexstringState();
  void dictState();
  void waitState();

public:
  tokenizer(string& s);
  vector<pair<string, TState>> result();
};

#endif

