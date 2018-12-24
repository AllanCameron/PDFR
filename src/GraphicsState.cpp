//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR GraphicsState implementation file                                   //
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
#include "stringfunctions.h"
#include "document.h"
#include "GraphicsState.h"
#include "encodings.h"
#include "debugtools.h"
#include "tokenizer.h"

GraphicsState::GraphicsState(page& pag) : p(pag),
  PRstate(0), Tl(1), Tw(0), Th(100), Tc(0), currfontsize(0), currentfont("")
{
  Instructions  = tokenize(p.contentstring);
  initstate = {1,0,0,0,1,0,0,0,1};
  fontstack.push_back(currentfont);
  Tmstate = Tdstate = initstate;
  gs.push_back(initstate);
  fontsizestack.push_back(currfontsize);
  InstructionReader(Instructions, "");
  MakeGS();
  db =  Rcpp::DataFrame::create(
              Rcpp::Named("text") = text,
              Rcpp::Named("left") = left,
              Rcpp::Named("bottom") = bottom,
              Rcpp::Named("right") = right,
              Rcpp::Named("font") = fonts,
              Rcpp::Named("size") = size,
              Rcpp::Named("width") = width,
              Rcpp::Named("stringsAsFactors") = false
              );
}


/*---------------------------------------------------------------------------*/

void GraphicsState::q(std::vector<std::string>& Operands)
{
  gs.push_back(gs.back());
  fontstack.push_back(currentfont);
  fontsizestack.push_back(currfontsize);
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Do(std::string& a)
{
  if (p.XObjects.find(a) != p.XObjects.end())
  {
    if(IsAscii(p.XObjects[a]))
      InstructionReader(tokenize(p.XObjects[a]), a);
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Q(std::vector<std::string>& Operands)
{
  if(gs.size() > 1) gs.pop_back();
  if (fontstack.size() > 1)
  {
    fontstack.pop_back();
    fontsizestack.pop_back();
    currentfont = fontstack.back();
    currfontsize = fontsizestack.back();
  }
  if (p.fontmap.find(currentfont) != p.fontmap.end())
  {
    wfont = p.fontmap[currentfont];
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Td(std::vector<std::string>& Operands)
{
  std::vector<float> Tds = initstate;
  std::vector<float> tmpvec = stringtofloat(Operands);
  Tds.at(6) = tmpvec[0]; Tds[7] = tmpvec[1];
  Tdstate = matmul(Tds, Tdstate);
  PRstate = 0;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TD(std::vector<std::string>& Operands)
{
  std::vector<float> Tds = initstate;
  std::vector<float> tmpvec = stringtofloat(Operands);
  Tds.at(6) = tmpvec[0]; Tds[7] = tmpvec[1];
  Tl = -Tds[7];
  Tdstate = matmul(Tds, Tdstate);
  PRstate = 0;
}


/*---------------------------------------------------------------------------*/

void GraphicsState::BT(std::vector<std::string>& Operands)
{
  Tmstate = Tdstate = initstate;
  Tw = Tc = 0;
  Th = 100;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::ET(std::vector<std::string>& Operands)
{
  Tmstate = Tdstate = initstate;
  Tw = Tc = 0;
  Th = 100;
}
/*---------------------------------------------------------------------------*/

void GraphicsState::Tf(std::vector<std::string>& Operands)
{
  currentfont = Operands.at(0);
  if (p.fontmap.find(currentfont) != p.fontmap.end())
  {
    wfont = p.fontmap[currentfont];
  }
  else
  {
    std::string stopmessage = "Could not find font ";
    stopmessage += currentfont;
    throw std::runtime_error(stopmessage);
  }
  currfontsize = std::stof(Operands[1]);
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TH(std::vector<std::string>& Operands)
{
  Th = stof(Operands.at(0));
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TC(std::vector<std::string>& Operands)
{
  Tc = stof(Operands.at(0));
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TW(std::vector<std::string>& Operands)
{
  Tw = stof(Operands.at(0));
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TL(std::vector<std::string>& Operands)
{
  Tl = stof(Operands.at(0));
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Tstar(std::vector<std::string>& Operands)
{
  Tdstate.at(7) = Tdstate.at(7) - Tl;
  PRstate = 0;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Tm(std::vector<std::string>& Operands)
{
  Tmstate = stringvectomat(Operands);
  Tdstate = initstate;
  PRstate = 0;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::cm(std::vector<std::string>& Operands)
{
  gs.back() = matmul(stringvectomat(Operands), gs.back());
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TJ(std::vector<std::vector<std::string>>& i)
{
  std::string& Ins = i[0][0];
  std::vector<std::string> &OperandTypes = i[1];
  std::vector<std::string> &Operands = i[2];
  if (Ins == "'") Tdstate[7] = Tdstate[7] - Tl;
  std::vector<float> textspace = matmul(Tmstate, gs.back());
  textspace = matmul(Tdstate, textspace);
  float txtspcinit = textspace[6];
  float scale = currfontsize * textspace[0];

  for (size_t z = 0; z < OperandTypes.size(); z++)
  {
    std::vector<std::pair<std::string, int>> kvs;
    if (OperandTypes[z] == "number")
    {
      PRstate -= std::stof(Operands[z]);
      float PRscaled = PRstate * scale / 1000;
      textspace[6] = PRscaled + txtspcinit;
    }
    else
    {
      float PRscaled = PRstate * scale / 1000;
      textspace[6] = PRscaled + txtspcinit;
      if (OperandTypes[z] == "hexstring")
      {
        Operands[z] = byteStringToString(Operands[z]);
      }
      if (Operands[z] == "") continue;
      kvs = wfont.mapString(Operands[z]);
      for (auto j : kvs)
      {
        float stw;
        statehx.push_back(textspace);
        if (j.first == "/space" || j.first == "/nbspace")
        {
          stw = j.second + (Tc + Tw) * 1000;
        }
        else stw = j.second + Tc * 1000;
        PRstate += stw;
        std::string tmpchar;
        //Simplest way to deal with ligatures is to specify them here
        if(j.first == "/fi"
        || j.first == "/fl"
        || j.first == "/ffi"
        || j.first == "/ff"
        || j.first == "/ffl"){
        if(j.first == "/fi") tmpchar = "fi";
        if(j.first == "/fl") tmpchar = "fl";
        if(j.first == "/ffi") tmpchar = "ffi";
        if(j.first == "/ff") tmpchar = "ff";
        if(j.first == "/ffl") tmpchar = "ffl";}
        else tmpchar = namesToChar(j.first, "/WinAnsiEncoding");
        float PRscaled = PRstate * scale / 1000;
        textspace[6] = PRscaled + txtspcinit;
        widths.push_back(scale * stw/1000 * Th/100);
        stringres.push_back(tmpchar);
        fontsize.push_back(scale);
        fontname.push_back(wfont.FontName);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::InstructionReader(Instructionset I,
                                      const std::string& insubloop)
{
  for (auto &i : I)
  {
    std::string& Ins = i[0][0];
    std::vector<std::string> &Operands = i[2];
    if (Ins == "Q")  Q(Operands);
    if (Ins == "q" ) q(Operands);
    if (Ins == "Th") TH(Operands);
    if (Ins == "Tw") TW(Operands);
    if (Ins == "Tc") TC(Operands);
    if (Ins == "TL") TL(Operands);
    if (Ins == "T*") Tstar(Operands);
    if (Ins == "Tm") Tm(Operands);
    if (Ins == "cm") cm(Operands);
    if (Ins == "Td") Td(Operands);
    if (Ins == "TD") TD(Operands);
    if (Ins == "BT") BT(Operands);
    if (Ins == "ET") ET(Operands);
    if (Ins == "Tf") Tf(Operands);
    if (Ins == "Do" && insubloop != Operands.at(0)) Do(Operands.at(0));
    if (Ins == "Tj" || Ins == "'" || Ins == "TJ") TJ(i);
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::MakeGS()
{
  for (auto i : statehx)
  {
    xvals.push_back(i[6]);
    yvals.push_back(i[7]);
  }
  for (size_t i = 0; i < widths.size(); i++)
  {
    R.push_back(widths[i] + xvals[i]);
    leftmatch.push_back(-1);
  }
  rightmatch = leftmatch;
  clump();
  for (size_t i = 0; i < widths.size(); i++)
  {
    if (leftmatch[i] == -1 && stringres[i] != " " && stringres[i] != "  ")
    {
      trimRight(stringres[i]);
      text.push_back(stringres[i]);
      left.push_back(xvals[i]);
      bottom.push_back(yvals[i]);
      right.push_back(R[i]);
      fonts.push_back(fontname[i]);
      size.push_back(fontsize[i]);
      width.push_back(R[i] - xvals[i]);
    }
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::clump()
{
  std::map<size_t, size_t> Rjoins;
  size_t s = widths.size();
  if (s > 0)
  {
    for (size_t i = 0; i < s; i++)
    {
      for (size_t j = 0; j < s; j++)
      {
        bool isNear = fabs(R[i] - xvals[j]) < (fontsize[i]);
        bool isLeftOf = xvals[i] < xvals[j];
        bool areDifferent = (i != j);
        bool nothingCloser = true;
        bool sameY = fabs(yvals[i] - yvals[j]) < 3;
        if(Rjoins.find(i) != Rjoins.end())
          nothingCloser = (xvals[Rjoins[i]] > xvals[j]);
        if(areDifferent && isLeftOf && isNear && nothingCloser && sameY)
        {
          Rjoins[i] = j;
          leftmatch[j] = (int) i;
        }
      }
    }
    for (size_t i = 0; i < s; i++)
    {
      if(leftmatch[i] == -1)
      {
        while(true)
        {
          if(Rjoins.find(i) != Rjoins.end())
          {
            if((xvals[Rjoins[i]] - R[i]) > (0.19 * fontsize[i]))
              stringres[i] += " ";
            stringres[i] += stringres[Rjoins[i]];
            R[i] = R[Rjoins[i]];
            widths[i] = R[i] - xvals[i];
            if(Rjoins.find(Rjoins[i]) != Rjoins.end())
              Rjoins[i] = Rjoins[Rjoins[i]]; else break;
          }
          else break;
        }
      }
    }
  }
}
