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

//---------------------------------------------------------------------------//

class GraphicsState
{
public:
  Rcpp::DataFrame db;
  GraphicsState(page* pag);

private:
  page* p;
  font* wfont;
  std::vector<std::vector<float>> gs, statehx;
  std::vector<float> xvals, yvals, fontsize, widths, Tmstate, Tdstate, R,
  left, right, bottom, size, width, fontsizestack, initstate;
  std::vector<std::string> fontname, fonts, fontstack;
  std::vector<Unicode> stringres, text;
  std::vector<int> leftmatch, rightmatch;
  int PRstate;
  float Tl, Tw, Th, Tc, currfontsize;
  std::string currentfont;
  std::vector<std::pair<std::string, Token::TState>> Instructions;
  void parser(std::vector<std::pair<std::string, Token::TState>>&, std::string);
  void Do(std::string&);
  void Q(std::vector<std::string>& );
  void q(std::vector<std::string>& );
  void TH(std::vector<std::string>& );
  void TW(std::vector<std::string>& );
  void TC(std::vector<std::string>& );
  void TL(std::vector<std::string>& );
  void Tstar(std::vector<std::string>& );
  void Tm(std::vector<std::string>& );
  void cm(std::vector<std::string>& );
  void Td(std::vector<std::string>& );
  void TD(std::vector<std::string>& );
  void BT(std::vector<std::string>& );
  void ET(std::vector<std::string>& );
  void Tf(std::vector<std::string>& );
  void TJ(std::string, std::vector<std::string>&, std::vector<Token::TState>&);
  void processRawChar(std::vector<RawChar>&, float&,
                      std::vector<float>&,   float&);
  void MakeGS();
  std::vector<float> matmul(std::vector<float>, std::vector<float>);
  std::vector<float> stringvectomat(std::vector<std::string>);
};

//---------------------------------------------------------------------------//

#endif
