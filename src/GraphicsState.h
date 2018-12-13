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


class GraphicsState
{
public:
  Rcpp::DataFrame db;
  GraphicsState(page& pag);
  page p;

private:
  font wfont;
  std::vector<std::vector<float>> gs, statehx;
  std::vector<float> xvals, yvals, fontsize, widths, Tmstate, Tdstate, R,
  left, right, bottom, size, width, fontsizestack, initstate;
  std::vector<std::string> fontname, stringres, text, fonts, fontstack;
  std::vector<int> leftmatch, rightmatch;
  int PRstate;
  float Tl, Tw, Th, Tc, currfontsize;
  std::string currentfont;
  Instructionset Instructions;
  Instructionset tokenize(std::string s);
  void tokenize_array(std::vector<std::string> &ttype,
                      std::vector<std::string> &token, std::string &s);
  Instructionset parser(std::vector<std::string> token,
                        std::vector<std::string> ttype);
  void InstructionReader(page& p, Instructionset I);
  void Q(page& p);
  void q();
  void Td(std::string Ins, std::vector<std::string>& Operands);
  void BT();
  void TJ(page& pag, std::vector<std::vector<std::string>>& i);
  void Tf(page& pag, std::vector<std::string>& Operands);
  void MakeGS();
  void clump();
  void Do(std::string& xo);

};

#endif
