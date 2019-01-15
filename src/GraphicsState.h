//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR GraphicsState header file                                           //
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

#ifndef PDFR_GS

//---------------------------------------------------------------------------//

#define PDFR_GS

#include <Rcpp.h> // This is the first header file to require Rcpp-specific fx
#include "tokenizer.h"

using namespace Token; // Needs to use enumerator defined in Token namespace
using namespace std;

//---------------------------------------------------------------------------//

class GraphicsState
{
public:
  Rcpp::DataFrame db;
  GraphicsState(page* pag);

private:
  page* p;
  font* wfont;
  vector<vector<float>> gs, statehx;
  vector<float> xvals, yvals, fontsize, widths, Tmstate, Tdstate, R,
  left, right, bottom, size, width, fontsizestack, initstate;
  vector<string> fontname, fonts, fontstack;
  vector<Unicode> stringres, text;
  vector<int> leftmatch, rightmatch;
  int PRstate;
  float Tl, Tw, Th, Tc, currfontsize;
  string currentfont;
  vector<pair<string, TState>> Instructions;
  void parser(vector<pair<string, TState>>&, string);
  void Do(string&);
  void Q(vector<string>& );
  void q(vector<string>& );
  void TH(vector<string>& );
  void TW(vector<string>& );
  void TC(vector<string>& );
  void TL(vector<string>& );
  void Tstar(vector<string>& );
  void Tm(vector<string>& );
  void cm(vector<string>& );
  void Td(vector<string>& );
  void TD(vector<string>& );
  void BT(vector<string>& );
  void ET(vector<string>& );
  void Tf(vector<string>& );
  void TJ(string, vector<string>&, vector<TState>&);
  void processRawChar(vector<RawChar>&, float&, vector<float>&, float&);
  void MakeGS();
  std::vector<float> matmul(std::vector<float>, std::vector<float>);
  std::vector<float> stringvectomat(std::vector<std::string>);
};

//---------------------------------------------------------------------------//

#endif
