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
#include "chartounicode.h"

using namespace std;

vector<vector<string>> tokenize(string& s)
{
  s.push_back(' ');
  vector<string> token, ttype;
  size_t i = 0;
  string buf, minibuf;
  State state = NEWSYMBOL;
  while(i < s.length())
  {
    char m = s[i];
    char n = symbol_type(s[i]);
    switch(state)
    {
      case NEWSYMBOL:   switch(n)
                        {
                          case 'L':   buf += m;    state = IDENTIFIER; break;
                          case 'D':   buf += m;    state = NUMBER;     break;
                          case '-':   buf += m;    state = NUMBER;     break;
                          case '.':   buf += "0."; state = NUMBER;     break;
                          case '_':   buf += '_';  state = IDENTIFIER; break;
                          case '*':   buf += '*';  state = IDENTIFIER; break;
                          case '\'':  buf += '\''; state = IDENTIFIER; break;
                          case '/':   buf += '/';  state = RESOURCE;   break;
                          case '[':                state = ARRAY;      break;
                          case '(':                state = STRING;     break;
                          case '<':                state = HEXSTRING;  break;
                          default :   buf = "";    state = NEWSYMBOL;  break;
                        }
                        break;
      case RESOURCE:    switch(n)
                        {
                          case 'L':   buf += m;   break;
                          case 'D':   buf += m;   break;
                          case '-':   buf += '-'; break;
                          case '+':   buf += '+'; break;
                          case '_':   buf += '_'; break;
                          case '*':   buf += '*'; break;
                          case '/':   state = RESOURCE;
                                      ttype.push_back("resource");
                                      token.push_back(buf);
                                      buf = "";   break;
                          case ' ':   state = NEWSYMBOL;
                                      ttype.push_back("resource");
                                      token.push_back(buf);
                                      buf = "";   break;
                          case '[':   state = ARRAY;
                                      ttype.push_back("resource");
                                      token.push_back(buf);
                                      buf = "";   break;
                          case '(':   state = STRING;
                                      ttype.push_back("resource");
                                      token.push_back(buf);
                                      buf = "";   break;
                          case '<':   state = HEXSTRING;
                                      ttype.push_back("resource");
                                      token.push_back(buf);
                                      buf = ""; break;
                        }
                        break;
      case IDENTIFIER:  switch(n)
                        {
                          case 'L':   buf += m;   break;
                          case 'D':   buf += m;   break;
                          case '-':   buf += m;   break;
                          case '_':   buf += '_'; break;
                          case '*':   buf += '*'; break;
                          case '/':   state = RESOURCE;
                                      ttype.push_back("identifier");
                                      token.push_back(buf);
                                      buf = "/";  break;
                          case ' ':   if (buf == "BI")
                                      {
                                        buf = "";
                                        state = WAIT;
                                      }
                                      else
                                      {
                                        state = NEWSYMBOL;
                                        ttype.push_back("identifier");
                                        token.push_back(buf);
                                        buf = "";
                                      }
                                      break;
                          case '[':   state = ARRAY;
                                      ttype.push_back("identifier");
                                      token.push_back(buf);
                                      buf = ""; break;
                          case '(':   state = STRING;
                                      ttype.push_back("identifier");
                                      token.push_back(buf);
                                      buf = ""; break;
                          case '<':   state = HEXSTRING;
                                      ttype.push_back("identifier");
                                      token.push_back(buf);
                                      buf = ""; break;
                        }
                        break;

      case NUMBER:      switch(n)
                        {
                          case 'L':   buf += m; break;
                          case 'D':   buf += m; break;
                          case '.':   buf += '.'; break;
                          case '-':   token.push_back(buf);
                                      buf = "";
                                      token.push_back("-");
                                      ttype.push_back("number");
                                      ttype.push_back("operator"); break;
                          case '_':   buf += '_'; break;
                          case '*':   token.push_back(buf);
                                      buf = "";
                                      token.push_back("*");
                                      ttype.push_back("number");
                                      ttype.push_back("operator");
                                      break;
                          case '/':   token.push_back(buf);
                                      buf = "";
                                      token.push_back("/");
                                      ttype.push_back("number");
                                      ttype.push_back("operator");
                                      break;
                          case ' ':   state = NEWSYMBOL;
                                      token.push_back(buf);
                                      buf = "";
                                      ttype.push_back("number");
                                      break;
                          case '[':   state = ARRAY;
                                      token.push_back(buf);
                                      buf = "";
                                      ttype.push_back("number");
                                      break;
                          case '(':   state = STRING;
                                      token.push_back(buf);
                                      buf = "";
                                      ttype.push_back("number");
                                      break;
                          case '<':   state = HEXSTRING;
                                      token.push_back(buf);
                                      buf = "";
                                      ttype.push_back("identifier");
                                      break;
                        }
                        break;
      case ARRAY:       switch(n)
                        {
                          case '\\':  buf += s[i++]; buf += s[i]; break;
                          case ']':   tokenize_array(ttype, token, buf);
                                      state = NEWSYMBOL; buf = ""; break;
                          default:    buf += m; break;
                        }
                        break;
      case STRING:      minibuf.clear();
                        switch(n)
                        {
                          case ')':   token.push_back(buf);
                                      buf = "";
                                      ttype.push_back("string");
                                      state = NEWSYMBOL;
                                      break;
                          case '\\':  i++;
                                      n = symbol_type(s[i]);
                                      if (n == 'D')
                                      {
                                        int octcount = 0;
                                        token.push_back(buf); buf = "";
                                        ttype.push_back("string");
                                        while(n == 'D' && octcount < 3)
                                        {
                                          minibuf += s[i];
                                          i++; octcount++;
                                          n = symbol_type(s[i]);
                                        }
                                        int newint = oct2dec(stoi(minibuf));
                                        string tmpstr = intToHexstring(newint);
                                        token.push_back(tmpstr);
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
                        break;

      case HEXSTRING:   switch(n)
                        {
                          case '<':   buf = ""; state = DICT; break;
                          case '\\':  buf += m + s[++i]; break;
                          case '>':   if (!buf.empty())
                                      {
                                        token.push_back(buf);
                                        ttype.push_back("hexstring");
                                      }
                                      buf = ""; state = NEWSYMBOL; break;
                          default:    buf += m; break;
                        }
                        break;
      case DICT:        switch(n)
                        {
                          case '\\':  buf += m + s[++i]; break;
                          case '>':   token.push_back(buf);
                                      ttype.push_back("dict");
                                      state = HEXSTRING; buf = ""; break;
                          default:    buf += m; break;
                        }
                        break;
      case WAIT:        if (m == 'E') state = WAITE; break;
      case WAITE:       if (m == 'I') state = WAITEI; else state = WAIT; break;
      case WAITEI:      if (n == ' ') state = NEWSYMBOL;
                        else state = WAIT; break;
      default: break;
    }
    ++i;
  }
  return {token, ttype};
}

/*---------------------------------------------------------------------------*/

void tokenize_array(vector<string> &ttype, vector<string> &token, string &s)
{
  string buf, minibuf;
  s.push_back(' ');
  State state = NEWSYMBOL;
  size_t i = 0;

  while(i < s.length())
  {
    char m = s[i];
    char n = symbol_type(m);
    switch(state)
    {
      case NEWSYMBOL:
      {
        switch(n)
        {
          case 'D': buf += m;
                    state = NUMBER;
                    break;
          case '.': buf += '0';
                    buf += '.';
                    state = NUMBER;
                    break;
          case '-': buf += m;
                    state = NUMBER;
                    break;
          case '(': state = STRING;
                    break;
          case '<': state = HEXSTRING;
                    break;
          default:  state = NEWSYMBOL;
                    buf ="";
                    break;
        }
        break;
      }

      case NUMBER:
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
          case ' ': state = NEWSYMBOL;
                    token.push_back(buf);
                    buf = "";
                    ttype.push_back("number");
                    break;
          case '(': state = STRING;
                    token.push_back(buf);
                    buf = "";
                    ttype.push_back("number");
                    break;
          case '<': state = HEXSTRING;
                    token.push_back(buf);
                    buf = "";
                    ttype.push_back("number");
                    break;
        }
        break;
      }

      case STRING:
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
                        int newint = oct2dec(stoi(minibuf));
                        string tmpstr = intToHexstring(newint);
                        token.push_back(tmpstr);
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
                      state = NEWSYMBOL;
                      break;
          default :   buf += m;
                      break;
        }
        break;
      }

      case HEXSTRING:
      {
        switch(n)
        {
          case '\\':  buf += m + s[i+1];
                      i++;
                      break;
          case '>':   token.push_back(buf);
                      buf = "";
                      ttype.push_back("hexstring");
                      state = NEWSYMBOL;
                      break;
          default:    buf += m;
                      break;
        }
        break;
      }
    default: break;
    }
  ++i;
  }
  s = "";
}

/*---------------------------------------------------------------------------*/
