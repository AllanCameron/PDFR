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

GraphicsState::GraphicsState(page& pag) : p(pag),
  PRstate(0), Tl(1), Tw(0), Th(100), Tc(0), currfontsize(0), currentfont("")
{
  Instructions  = tokenize(pag.contentstring);
  initstate = {1,0,0,0,1,0,0,0,1};
  fontstack.push_back(currentfont);
  Tmstate = Tdstate = initstate;
  gs.push_back(initstate);
  fontsizestack.push_back(currfontsize);
  InstructionReader(pag, Instructions);
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

Instructionset GraphicsState::tokenize(std::string s)
{
  s.push_back(' ');
  std::vector<std::string> token, ttype;
  size_t i = 0;
  std::string buf, minibuf, state;
  state = "newsymb";
  while(i < s.length())
  {
    char m = s[i];
    char n = symbol_type(s[i]);

    if (state == "newsymb")
    {
      switch(n)
      {
      case 'L': buf += m; state = "identifier"; break;
      case 'D': buf += m; state = "number"; break;
      case '-': buf += m; state = "number"; break;
      case '.': buf += "0."; state = "number"; break;
      case '_': buf += '_';  state = "identifier"; break;
      case '*': buf += '*';  state = "identifier"; break;
      case '\'': buf += '\''; state = "identifier"; break;
      case '/': buf += '/'; state = "resource"; break;
      case '[': state = "array"; break;
      case '(': state = "string"; break;
      case '<': state = "hexstring"; break;
      default: state = "newsymb"; buf =""; break;
      }
      i++; continue;
    }

    if (state == "resource")
    {
      switch(n)
      {
      case 'L': buf += m; break;
      case 'D': buf += m; break;
      case '-': buf += '-'; break;
      case '+': buf += '+'; break;
      case '_': buf += '_'; break;
      case '*': buf += '*'; break;
      case '/': state = "resource";
        token.push_back(buf);
        buf = "";
        ttype.push_back("resource"); break;
      case ' ': state = "newsymb";
        token.push_back(buf);
        buf = "";
        ttype.push_back("resource");
        break;
      case '[': state = "array";
        token.push_back(buf);
        buf = "";
        ttype.push_back("resource"); break;
      case '(': state = "string";
        token.push_back(buf);
        buf = "";
        ttype.push_back("resource"); break;
      case '<': state = "hexstring";
        token.push_back(buf);
        buf = "";
        ttype.push_back("resource"); break;
      }
      i++; continue;
    }

    if (state == "identifier")
    {
      switch(n)
      {
      case 'L': buf += m; break;
      case 'D': buf += m; break;
      case '-': buf += m; break;
      case '_': buf += '_'; break;
      case '*': buf += '*'; break;
      case '/': state = "resource";
        token.push_back(buf);
        buf = "/";
        ttype.push_back("identifier"); break;
      case ' ': if (buf == "BI")
      {
        buf = "";
        state = "wait";
      }
      else
      {
        state = "newsymb";
        token.push_back(buf);
        buf = "";
        ttype.push_back("identifier");
      }
      break;
      case '[': state = "array";
        token.push_back(buf);
        buf = "";
        ttype.push_back("identifier"); break;
      case '(': state = "string";
        token.push_back(buf);
        buf = "";
        ttype.push_back("identifier"); break;
      case '<': state = "hexstring";
        token.push_back(buf);
        buf = "";
        ttype.push_back("identifier"); break;
      }
      i++; continue;
    }

    if (state == "number")
    {
      switch(n)
      {
      case 'L': buf += m; break;
      case 'D': buf += m; break;
      case '.': buf += '.'; break;
      case '-': token.push_back(buf); buf = "";
      token.push_back("-");
      ttype.push_back("number");
      ttype.push_back("operator"); break;
      case '_': buf += '_'; break;
      case '*': token.push_back(buf); buf = "";
      token.push_back("*");
      ttype.push_back("number");
      ttype.push_back("operator"); break;
      case '/': token.push_back(buf); buf = "";
      token.push_back("/");
      ttype.push_back("number");
      ttype.push_back("operator"); break;
      case ' ': state = "newsymb";
        token.push_back(buf);
        buf = "";
        ttype.push_back("number"); break;
      case '[': state = "array";
        token.push_back(buf);
        buf = "";
        ttype.push_back("number"); break;
      case '(': state = "string";
        token.push_back(buf);
        buf = "";
        ttype.push_back("number"); break;
      case '<': state = "hexstring";
        token.push_back(buf);
        buf = "";
        ttype.push_back("identifier"); break;
      }
      i++; continue;
    }

    if (state == "array")
    {
      char n = symbol_type(m);
      switch(n)
      {
      case '\\': buf += s[i]; i++; buf += s[i]; break;
      case ']': tokenize_array(ttype, token, buf);
        state = "newsymb"; buf = ""; break;
      default: buf += m; break;
      }
      i++; continue;
    }

    if (state == "string")
    {
      minibuf.clear();
      switch(s[i])
      {
      case ')': token.push_back(buf); buf = "";
      ttype.push_back("string");
      state = "newsymb"; break;
      case '\\': i++;
        n = symbol_type(s[i]);
        if (n == 'D')
        {
          int octcount = 0;
          token.push_back(buf); buf = "";
          ttype.push_back("string");
          while(n == 'D' && octcount < 3)
          {
            minibuf += s[i];
            i++; octcount ++;
            n = symbol_type(s[i]);
          }
          int newint = oct2dec(std::stoi(minibuf));
          token.push_back(intToHexstring(newint));
          ttype.push_back("hexstring");
          minibuf = "";
          i--;
        }
        else buf += s[i];
        break;
      default: buf += s[i]; break;
      }
      i++; continue;
    }

    if (state == "hexstring")
    {
      switch(n)
      {
      case '<': buf = ""; state = "dict"; break;
      case '\\': buf += m + s[i+1]; i++; break;
      case '>': if (buf.length() > 0) token.push_back(buf);
      if (buf.length() > 0) ttype.push_back("hexstring");
      buf = ""; state = "newsymb"; break;
      default: buf += m; break;
      }
      i++; continue;
    }

    if (state == "dict")
    {
      switch(n)
      {
      case '\\': buf += m + s[i+1]; i++; break;
      case '>': token.push_back(buf); buf = "";
      ttype.push_back("dict");
      state = "hexstring"; break;
      default:  buf += m; break;
      }
      i++; continue;
    }

    if (state == "wait"){ if (m == 'E') state = "waitE"; i++; continue;}
    if (state == "waitE")
    {
      if (m == 'I') state = "waitEI"; else state = "wait";
      i++; continue;
    }
    if (state == "waitEI")
    {
      if (n == ' ') state = "newsymb"; else state = "wait";
      i++; continue;
    }

    else { i++; continue; }
  }

  return parser(token, ttype);
}

/*---------------------------------------------------------------------------*/

void GraphicsState::tokenize_array(std::vector<std::string> &ttype,
                                   std::vector<std::string> &token,
                                   std::string &s)
{
  std::string buf, minibuf;
  s.push_back(' ');
  std::string state = "newsymb";
  size_t i = 0;

  while(i < s.length())
  {
    char m = s[i];
    char n = symbol_type(m);
    if (state == "newsymb")
    {
      switch(n)
      {
      case 'D': buf += m; state = "number"; break;
      case '.': buf += '0'; buf += '.'; state = "number"; break;
      case '-': buf += m; state = "number"; break;
      case '(': state = "string"; break;
      case '<': state = "hexstring"; break;
      default: state = "newsymb"; buf =""; break;
      }
      i++; continue;
    }

    if (state == "number")
    {
      switch(n)
      {
      case 'L': buf += m; break;
      case 'D': buf += m; break;
      case '.': buf += '.'; break;
      case '-': token.push_back(buf); buf = "";
      token.push_back("-");
      ttype.push_back("number");
      ttype.push_back("operator"); break;
      case '_': buf += '_'; break;
      case '*': token.push_back(buf); buf = "";
      token.push_back("*");
      ttype.push_back("number");
      ttype.push_back("operator"); break;
      case '/': token.push_back(buf); buf = "";
      token.push_back("/");
      ttype.push_back("number");
      ttype.push_back("operator"); break;
      case ' ': state = "newsymb";
        token.push_back(buf);
        buf = "";
        ttype.push_back("number"); break;
      case '(': state = "string";
        token.push_back(buf);
        buf = "";
        ttype.push_back("number"); break;
      case '<': state = "hexstring";
        token.push_back(buf);
        buf = "";
        ttype.push_back("number"); break;
      }
      i++; continue;
    }

    if (state == "string")
    {
      switch(n)
      {
      case '\\':  i++;
        if (symbol_type(s[i]) == 'D')
        {
          token.push_back(buf); buf = "";
          ttype.push_back("string");
          int octcount = 0;
          while(symbol_type(s[i]) == 'D' && octcount < 3)
          {
            minibuf += s[i]; i++; octcount++;
          }
          int newint = oct2dec(std::stoi(minibuf));
          token.push_back(intToHexstring(newint));
          ttype.push_back("hexstring");
          minibuf = "";
          i--;
        }
        else
        {
          buf += s[i];
        }
        break;
      case ')':   token.push_back(buf); buf = "";
      ttype.push_back("string");
      state = "newsymb"; break;
      default :   buf += m; break;
      }
      i++; continue;
    }

    if (state == "hexstring")
    {
      switch(n)
      {
      case '\\': buf += m + s[i+1]; i++; break;
      case '>':   token.push_back(buf); buf = "";
      ttype.push_back("hexstring");
      state = "newsymb"; break;
      default:  buf += m; break;
      }
      i++;
      continue;
    }
  }
  s = "";
}

/*---------------------------------------------------------------------------*/

Instructionset GraphicsState::parser(std::vector<std::string> token,
                                     std::vector<std::string> ttype)
{
  std::vector<std::vector<std::string>> tmpres;
  std::vector<std::string> tmptype, tmptoken, tmpident;
  std::vector<std::vector<std::vector<std::string>>> res;
  for (size_t i = 0; i < ttype.size(); i++)
  {
    tmptype.push_back(ttype[i]);
    tmptoken.push_back(token[i]);
    if (ttype[i] == "identifier")
    {
      if (token[i] == "Q"  || token[i] == "q"  ||  token[i] == "BT" ||
          token[i] == "ET" || token[i] == "TJ" ||  token[i] == "Tj" ||
          token[i] == "TD" || token[i] == "Td" ||  token[i] == "T*" ||
          token[i] == "Tc" || token[i] == "Tw" ||  token[i] == "Tm" ||
          token[i] == "Tf" || token[i] == "TL" ||  token[i] == "Tr" ||
          token[i] == "\"" || token[i] == "'"  ||  token[i] == "cm" ||
          token[i] == "Tz" || token[i] == "Th" ||  token[i] == "Do"
      )
      {
        tmptype.pop_back();
        tmptoken.pop_back();
        tmpident.push_back(token[i]);
        tmpres.push_back(tmpident);
        tmpres.push_back(tmptype);
        tmpres.push_back(tmptoken);
        res.push_back(tmpres);
      }
      tmptype.clear();
      tmptoken.clear();
      tmpident.clear();
      tmpres.clear();
    }
  }
  return res;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::q()
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
      InstructionReader(p, tokenize(p.XObjects[a]));
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Q(page& pag)
{
  if(gs.size() > 1) gs.pop_back();
  if (fontstack.size() > 1)
  {
    fontstack.pop_back();
    fontsizestack.pop_back();
    currentfont = fontstack.back();
    currfontsize = fontsizestack.back();
  }
  if (pag.fontmap.find(currentfont) != pag.fontmap.end())
  {
    wfont = pag.fontmap[currentfont];
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Td(std::string Ins, std::vector<std::string>& Operands)
{
  std::vector<float> Tds = initstate;
  std::vector<float> tmpvec = stringtofloat(Operands);
  Tds.at(6) = tmpvec[0]; Tds[7] = tmpvec[1];
  if (Ins == "TD") Tl = -Tds[7];
  Tdstate = matmul(Tds, Tdstate);
  PRstate = 0;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::BT()
{
  Tmstate = Tdstate = initstate;
  Tw = Tc = 0;
  Th = 100;
}

/*---------------------------------------------------------------------------*/

void GraphicsState::Tf(page& pag, std::vector<std::string>& Operands)
{
  currentfont = Operands.at(0);
  if (pag.fontmap.find(currentfont) != pag.fontmap.end())
  {
    wfont = pag.fontmap[currentfont];
  }
  else
  {
    std::string stopmessage = "Could not find font ";
    stopmessage += currentfont;
    throw stopmessage;
  }
  currfontsize = std::stof(Operands[1]);
}

/*---------------------------------------------------------------------------*/

void GraphicsState::TJ(page& pag, std::vector<std::vector<std::string>>& i)
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
        || j.first == "/ffl")
        {
          if(j.first == "/fi") tmpchar = "fi";
          if(j.first == "/fl") tmpchar = "fl";
          if(j.first == "/ffi") tmpchar = "ffi";
          if(j.first == "/ff") tmpchar = "ff";
          if(j.first == "/ffl") tmpchar = "ffl";
        }
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

void GraphicsState::InstructionReader(page& pag, Instructionset I)
{
  for (auto &i : I)
  {
    std::string& Ins = i.at(0).at(0);
    std::vector<std::string> &Operands = i.at(2);
    if (Ins == "Q" && gs.size() > 0) Q(pag);
    if (Ins == "q") q();
    if (Ins == "Th") Th = stof(Operands.at(0));
    if (Ins == "Tw") Tw = stof(Operands.at(0));
    if (Ins == "Tc") Tc = stof(Operands.at(0));
    if (Ins == "TL") Tl = stof(Operands.at(0));
    if (Ins == "T*") { Tdstate.at(7) = Tdstate.at(7) - Tl; PRstate = 0;}
    if (Ins == "Tm") { Tmstate = stringvectomat(Operands);
                      Tdstate = initstate; PRstate = 0;}
    if (Ins == "cm") gs.back() = matmul(stringvectomat(Operands), gs.back());
    if (Ins == "Td" || Ins == "TD") Td(Ins, Operands);
    if (Ins == "ET" || Ins == "BT") BT();
    if (Ins == "Tf") Tf(pag, Operands);
    if (Ins == "Do") Do(Operands.at(0));
    if (Ins == "Tj" || Ins == "'" || Ins == "TJ") TJ(pag, i);
  }
}

/*---------------------------------------------------------------------------*/

void GraphicsState::MakeGS()
{
  for (auto i : statehx) { xvals.push_back(i[6]); yvals.push_back(i[7]);}
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
