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
#include "debugtools.h"
#include "chartounicode.h"

using namespace std;

GraphicsState::GraphicsState(page& pag) : p(pag),
  PRstate(0), Tl(1), Tw(0), Th(100), Tc(0), currfontsize(0), currentfont("")
{
  Instructions  = tokenize(p.contentstring);
  initstate = {1,0,0,0,1,0,0,0,1};
  fontstack.emplace_back(currentfont);
  Tmstate = Tdstate = initstate;
  gs.push_back(initstate);
  fontsizestack.emplace_back(currfontsize);
  parser(Instructions, "");
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

/*--------------------------------------------------------------------------*/
//Converts an ASCII encoded string to a (char-based) string
string byteStringToString(const string& s)
{
  vector<string>&& sv = splitfours(s);
  vector<uint16_t> uv;
  string res = "";
  for(auto i : sv)
    uv.emplace_back((uint16_t) stoul("0x" + i, nullptr, 0));
  for(auto i : uv)
     res += UnicodeToChar(i, WINANSI);
  return res;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::q(vector<string>& Operands)
{
  gs.emplace_back(gs.back());
  fontstack.emplace_back(currentfont);
  fontsizestack.emplace_back(currfontsize);
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Do(string& a)
{
  if (p.XObjects.find(a) != p.XObjects.end())
  {
    if(IsAscii(p.XObjects[a]))
    {
      auto ins = tokenize(p.XObjects[a]);
      parser(ins, a);
    }
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Q(vector<string>& Operands)
{
  if (gs.size() > 1)
    gs.pop_back();
  if (fontstack.size() > 1)
  {
    fontstack.pop_back();
    fontsizestack.pop_back();
    currentfont = fontstack.back();
    currfontsize = fontsizestack.back();
  }
  if (p.fontmap.find(currentfont) != p.fontmap.end())
    wfont = p.fontmap[currentfont];
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Td(vector<string>& Operands)
{
  vector<float> Tds = initstate;
  vector<float> tmpvec = stringtofloat(Operands);
  Tds.at(6) = tmpvec[0]; Tds[7] = tmpvec[1];
  Tdstate = matmul(Tds, Tdstate);
  PRstate = 0;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TD(vector<string>& Operands)
{
  vector<float> Tds = initstate;
  vector<float> tmpvec = stringtofloat(Operands);
  Tds.at(6) = tmpvec[0]; Tds[7] = tmpvec[1];
  Tl = -Tds[7];
  Tdstate = matmul(Tds, Tdstate);
  PRstate = 0;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::BT(vector<string>& Operands)
{
  Tmstate = Tdstate = initstate;
  Tw = Tc = 0;
  Th = 100;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::ET(vector<string>& Operands)
{
  Tmstate = Tdstate = initstate;
  Tw = Tc = 0;
  Th = 100;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Tf(vector<string>& Operands)
{
  if(Operands.size() > 1)
  {
    currentfont = Operands[0];
    if (p.fontmap.find(currentfont) != p.fontmap.end())
      wfont = p.fontmap[currentfont];
    else
      throw runtime_error(string("Couldn't find font") + currentfont);
    currfontsize = stof(Operands[1]);
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TH(vector<string>& Operands)
{
  Th = stof(Operands.at(0));
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TC(vector<string>& Operands)
{
  Tc = stof(Operands.at(0));
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TW(vector<string>& Operands)
{
  Tw = stof(Operands.at(0));
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TL(vector<string>& Operands)
{
  Tl = stof(Operands.at(0));
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Tstar(vector<string>& Operands)
{
  Tdstate.at(7) = Tdstate.at(7) - Tl;
  PRstate = 0;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Tm(vector<string>& Operands)
{
  Tmstate = stringvectomat(Operands);
  Tdstate = initstate;
  PRstate = 0;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::cm(vector<string>& Operands)
{
  gs.back() = matmul(stringvectomat(Operands), gs.back());
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TJ(string Ins, vector<string>& Operands,
                       vector<string>& OperandTypes)
{
  if (Ins == "'") Tdstate[7] -= Tl;
  vector<float> textspace = matmul(Tmstate, gs.back());
  textspace = matmul(Tdstate, textspace);
  float txtspcinit = textspace[6];
  float scale = currfontsize * textspace[0];
  size_t otsize = OperandTypes.size();
  for (size_t z = 0; z < otsize; z++)
  {
    if (OperandTypes[z] == "number")
    {
      PRstate -= stof(Operands[z]);
      float PRscaled = PRstate * scale / 1000;
      textspace[6] = PRscaled + txtspcinit;
      continue;
    }
    float PRscaled = PRstate * scale / 1000;
    textspace[6] = PRscaled + txtspcinit;
    if (OperandTypes[z] == "hexstring")
      Operands[z] = byteStringToString(Operands[z]);
    if (Operands[z] == "")
      continue;
    vector<pair<char, int>>&& kvs = wfont.mapString(Operands[z]);
    for (auto& j : kvs)
    {
      float stw;
      statehx.emplace_back(textspace);
      if (j.first == 0x20 || j.first == 0xA0)
        stw = j.second + (Tc + Tw) * 1000;
      else stw = j.second + Tc * 1000;
      PRstate += stw;
      string tmpchar;
      if(ligatures.find(j.first) != ligatures.end())
        tmpchar = ligatures[j.first];
      else
        tmpchar = UnicodeToChar((uint16_t) j.first, WINANSI);
      float PRscaled = PRstate * scale / 1000;
      textspace[6] = PRscaled + txtspcinit;
      widths.emplace_back(scale * stw/1000 * Th/100);
      stringres.emplace_back(tmpchar);
      fontsize.emplace_back(scale);
      fontname.emplace_back(wfont.FontName);
    }
  }
}


void GraphicsState::parser(vector<vector<string>>& tokens, string inloop)
{
  vector<string>& token = tokens[0];
  vector<string>& ttype = tokens[1];
  vector<string> typevec, Operands;
  size_t tts = ttype.size();
  for (size_t i = 0; i < tts; i++)
  {
    if (ttype[i] == "identifier")
    {
      if (token[i] == "Q")  Q(Operands);
      else if (token[i] == "q" ) q(Operands);
      else if (token[i] == "Th") TH(Operands);
      else if (token[i] == "Tw") TW(Operands);
      else if (token[i] == "Tc") TC(Operands);
      else if (token[i] == "TL") TL(Operands);
      else if (token[i] == "T*") Tstar(Operands);
      else if (token[i] == "Tm") Tm(Operands);
      else if (token[i] == "cm") cm(Operands);
      else if (token[i] == "Td") Td(Operands);
      else if (token[i] == "TD") TD(Operands);
      else if (token[i] == "BT") BT(Operands);
      else if (token[i] == "ET") ET(Operands);
      else if (token[i] == "Tf") Tf(Operands);
      else if (token[i] == "Do" && inloop != Operands.at(0)) Do(Operands.at(0));
      else if (token[i] == "Tj" || token[i] == "'" || token[i] == "TJ")
        TJ(token[i], Operands, typevec);
      typevec.clear();
      Operands.clear();
    }
    else
    {
      // push operands and their types on stack, awaiting instruction
      typevec.push_back(ttype[i]);
      Operands.push_back(token[i]);
    }
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::MakeGS()
{
  for (auto i : statehx)
  {
    xvals.emplace_back(i[6]);
    yvals.emplace_back(i[7]);
  }
  size_t wsize = widths.size();
  for (size_t i = 0; i < wsize; i++)
  {
    R.emplace_back(widths[i] + xvals[i]);
    leftmatch.push_back(-1);
  }
  rightmatch = leftmatch;
  //clump();
  for (size_t i = 0; i < wsize; i++)
  {
    if (leftmatch[i] == -1 && stringres[i] != " " && stringres[i] != "  ")
    {
      trimRight(stringres[i]);
      text.emplace_back(stringres[i]);
      left.emplace_back(xvals[i]);
      bottom.emplace_back(yvals[i]);
      right.emplace_back(R[i]);
      fonts.emplace_back(fontname[i]);
      size.emplace_back(fontsize[i]);
      width.emplace_back(R[i] - xvals[i]);
    }
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::clump()
{
  map<size_t, size_t> Rjoins;
  size_t s = widths.size();
  if (s == 0) return;
  for (size_t i = 0; i < s; i++)
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
  for (size_t i = 0; i < s; i++)
    if(leftmatch[i] == -1)
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
            Rjoins[i] = Rjoins[Rjoins[i]];
          else break;
        }
        else break;
      }
}
