//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR tokenizer implementation file                                       //
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
#include "GraphicsState.h"
#include "stringfunctions.h"
#include "tokenizer.h"

using namespace std;

Instructionset tokenize(std::string s)
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
        case 'L': buf += m;
                  state = "identifier";
                  break;
        case 'D': buf += m;
                  state = "number";
                  break;
        case '-': buf += m;
                  state = "number";
                  break;
        case '.': buf += "0.";
                  state = "number";
                  break;
        case '_': buf += '_';
                  state = "identifier";
                  break;
        case '*': buf += '*';
                  state = "identifier";
                  break;
        case '\'': buf += '\'';
                  state = "identifier";
                  break;
        case '/': buf += '/';
                  state = "resource";
                  break;
        case '[': state = "array";
                  break;
        case '(': state = "string";
                  break;
        case '<': state = "hexstring";
                  break;
        default:  state = "newsymb";
                  buf ="";
                  break;
      }
      i++; continue;
    }

    if (state == "resource")
    {
      switch(n)
      {
        case 'L': buf += m;
                  break;
        case 'D': buf += m;
                  break;
        case '-': buf += '-';
                  break;
        case '+': buf += '+';
                  break;
        case '_': buf += '_';
                  break;
        case '*': buf += '*';
                  break;
        case '/': state = "resource";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("resource");
                  break;
        case ' ': state = "newsymb";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("resource");
                  break;
        case '[': state = "array";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("resource");
                  break;
        case '(': state = "string";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("resource");
                  break;
        case '<': state = "hexstring";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("resource");
                  break;
      }
      i++;
      continue;
    }

    if (state == "identifier")
    {
      switch(n)
      {
        case 'L': buf += m;
                  break;
        case 'D': buf += m;
                  break;
        case '-': buf += m;
                  break;
        case '_': buf += '_';
                  break;
        case '*': buf += '*';
                  break;
        case '/': state = "resource";
                  token.push_back(buf);
                  buf = "/";
                  ttype.push_back("identifier");
                  break;
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
                  ttype.push_back("identifier");
                  break;
        case '(': state = "string";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("identifier");
                  break;
        case '<': state = "hexstring";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("identifier");
                  break;
      }
      i++;
      continue;
    }

    if (state == "number")
    {
      switch(n)
      {
        case 'L': buf += m;
                  break;
        case 'D': buf += m;
                  break;
        case '.': buf += '.';
                  break;
        case '-': token.push_back(buf);
                  buf = "";
                  token.push_back("-");
                  ttype.push_back("number");
                  ttype.push_back("operator");
                  break;
        case '_': buf += '_';
                  break;
        case '*': token.push_back(buf);
                  buf = "";
                  token.push_back("*");
                  ttype.push_back("number");
                  ttype.push_back("operator");
                  break;
        case '/': token.push_back(buf);
                  buf = "";
                  token.push_back("/");
                  ttype.push_back("number");
                  ttype.push_back("operator");
                  break;
        case ' ': state = "newsymb";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("number");
                  break;
        case '[': state = "array";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("number");
                  break;
        case '(': state = "string";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("number");
                  break;
        case '<': state = "hexstring";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("identifier");
                  break;
      }
      i++;
      continue;
    }

    if (state == "array")
    {
      char n = symbol_type(m);
      switch(n)
      {
        case '\\': buf += s[i];
                   i++;
                   buf += s[i];
                   break;
        case ']':  tokenize_array(ttype, token, buf);
                   state = "newsymb";
                   buf = "";
                   break;
        default:   buf += m;
                   break;
      }
      i++;
      continue;
    }

    if (state == "string")
    {
      minibuf.clear();
      switch(s[i])
      {
        case ')':  token.push_back(buf);
                   buf = "";
                   ttype.push_back("string");
                   state = "newsymb";
                   break;
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
                    else
                      buf += s[i];
                    break;
        default:    buf += s[i];
                    break;
      }
      i++;
      continue;
    }

    if (state == "hexstring")
    {
      switch(n)
      {
        case '<':  buf = "";
                   state = "dict";
                   break;
        case '\\': buf += m + s[i+1];
                   i++;
                   break;
        case '>':  if (!buf.empty())
                   {
                     token.push_back(buf);
                     ttype.push_back("hexstring");
                   }
                   buf = "";
                   state = "newsymb";
                   break;
        default:   buf += m;
                   break;
      }
      i++;
      continue;
    }

    if (state == "dict")
    {
      switch(n)
      {
        case '\\': buf += m + s[i+1];
                   i++;
                   break;
        case '>':  token.push_back(buf);
                   buf = "";
                   ttype.push_back("dict");
                   state = "hexstring";
                   break;
        default:   buf += m;
                   break;
      }
      i++;
      continue;
    }

    if (state == "wait")
    {
      if (m == 'E')
        state = "waitE";
      i++;
      continue;
    }
    if (state == "waitE")
    {
      if (m == 'I')
        state = "waitEI";
      else
        state = "wait";
      i++;
      continue;
    }
    if (state == "waitEI")
    {
      if (n == ' ')
        state = "newsymb";
      else
        state = "wait";
      i++;
      continue;
    }
    else
    {
      i++;
      continue;
    }
  }

  return parser(token, ttype);
}

/*---------------------------------------------------------------------------*/

void tokenize_array(std::vector<std::string> &ttype,
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
        case 'D': buf += m;
                  state = "number";
                  break;
        case '.': buf += '0';
                  buf += '.';
                  state = "number";
                  break;
        case '-': buf += m;
                  state = "number";
                  break;
        case '(': state = "string";
                  break;
        case '<': state = "hexstring";
                  break;
        default:  state = "newsymb";
                  buf ="";
                  break;
      }
      i++;
      continue;
    }

    if (state == "number")
    {
      switch(n)
      {
        case 'L': buf += m;
                  break;
        case 'D': buf += m;
                  break;
        case '.': buf += '.';
                  break;
        case '-': token.push_back(buf);
                  buf = "";
                  token.push_back("-");
                  ttype.push_back("number");
                  ttype.push_back("operator");
                  break;
        case '_': buf += '_';
                  break;
        case '*': token.push_back(buf);
                  buf = "";
                  token.push_back("*");
                  ttype.push_back("number");
                  ttype.push_back("operator");
                  break;
        case '/': token.push_back(buf);
                  buf = "";
                  token.push_back("/");
                  ttype.push_back("number");
                  ttype.push_back("operator");
                  break;
        case ' ': state = "newsymb";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("number");
                  break;
        case '(': state = "string";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("number");
                  break;
        case '<': state = "hexstring";
                  token.push_back(buf);
                  buf = "";
                  ttype.push_back("number");
                  break;
      }
      i++;
      continue;
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
        case ')':   token.push_back(buf);
                    buf = "";
                    ttype.push_back("string");
                    state = "newsymb";
                    break;
        default :   buf += m;
                    break;
      }
      i++;
      continue;
    }

    if (state == "hexstring")
    {
      switch(n)
      {
        case '\\':  buf += m + s[i+1];
                    i++;
                    break;
        case '>':   token.push_back(buf);
                    buf = "";
                    ttype.push_back("hexstring");
                    state = "newsymb";
                    break;
        default:    buf += m;
                    break;
      }
      i++;
      continue;
    }
  }
  s = "";
}

/*---------------------------------------------------------------------------*/

Instructionset parser(vector<string>& token, vector<string>& ttype)
{
  std::vector<std::vector<std::string>> tmpres;
  std::vector<std::string> tmptype, tmptoken, tmpident;
  Instructionset res;
  size_t tts = ttype.size();
  for (size_t i = 0; i < tts; i++)
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
