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
#define PDFR_GS

#include "pdfr.h"
using namespace std;

class GraphicsState
{
public:
  Rcpp::DataFrame db;
  GraphicsState(page& pag);

private:
  page p;
  font wfont;
  vector<vector<float>> gs, statehx;
  vector<float> xvals, yvals, fontsize, widths, Tmstate, Tdstate, R,
  left, right, bottom, size, width, fontsizestack, initstate;
  vector<string> fontname, stringres, text, fonts, fontstack;
  vector<int> leftmatch, rightmatch;
  int PRstate;
  float Tl, Tw, Th, Tc, currfontsize;
  string currentfont;
  vector<vector<string>> Instructions;
  void parser(vector<vector<string>>&, string);
  void Q(vector<string>& Operands);
  void q(vector<string>& Operands);
  void TH(vector<string>& Operands);
  void TW(vector<string>& Operands);
  void TC(vector<string>& Operands);
  void TL(vector<string>& Operands);
  void Tstar(vector<string>& Operands);
  void Tm(vector<string>& Operands);
  void cm(vector<string>& Operands);
  void Td(vector<string>& Operands);
  void TD(vector<string>& Operands);
  void BT(vector<string>& Operands);
  void ET(vector<string>& Operands);
  void Tf(vector<string>& Operands);
  void TJ(string Ins, vector<string>& Operands, vector<string>& OperandTypes);
  void MakeGS();
  void clump();
  void Do(string& xo);
};

#endif
