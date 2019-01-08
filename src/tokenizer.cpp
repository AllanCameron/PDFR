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
#include "debugtools.h"

using namespace std;
using namespace Token;

/*---------------------------------------------------------------------------*/

tokenizer::tokenizer(string& input) : i(0), s(input),  state(NEWSYMBOL)
{
  s.push_back(' ');
  tokenize();
}

/*---------------------------------------------------------------------------*/

void tokenizer::tokenize()
{
  while(i < s.length())
  {
    switch(state)
    {
      case NEWSYMBOL:   newsymbolState();   break;
      case RESOURCE:    resourceState();    break;
      case IDENTIFIER:  identifierState();  break;
      case NUMBER:      numberState();      break;
      case ARRAY:       arrayState();       break;
      case STRING:      stringState();      break;
      case HEXSTRING:   hexstringState();   break;
      case DICT:        dictState();        break;
      case WAIT:        waitState();        break;
    }
    ++i;
  }
  output.emplace_back(token);
  output.emplace_back(ttype);
}

/*---------------------------------------------------------------------------*/

vector<vector<string>> tokenizer::result()
{
  return output;
}

/*---------------------------------------------------------------------------*/

void tokenizer::pushbuf(string type, TState statename)
{
  state = statename;
  ttype.push_back(type);
  token.push_back(buf);
  buf.clear();
}

/*---------------------------------------------------------------------------*/

void tokenizer::resourceState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case 'L':   buf += m;   break;
    case 'D':   buf += m;   break;
    case '-':   buf += '-'; break;
    case '+':   buf += '+'; break;
    case '_':   buf += '_'; break;
    case '*':   buf += '*'; break;
    case '/':   pushbuf("resource", RESOURCE);  break;
    case ' ':   pushbuf("resource", NEWSYMBOL); break;
    case '[':   pushbuf("resource", ARRAY);     break;
    case '(':   pushbuf("resource", STRING);    break;
    case '<':   pushbuf("resource", HEXSTRING); break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::newsymbolState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case 'L':   buf += m;    state = IDENTIFIER; break;
    case 'D':   buf += m;    state = NUMBER;     break;
    case '-':   buf += m;    state = NUMBER;     break;
    case '_':   buf += m;    state = IDENTIFIER; break;
    case '*':   buf += m;    state = IDENTIFIER; break;
    case '\'':  buf += m;    state = IDENTIFIER; break;
    case '/':   buf += m;    state = RESOURCE;   break;
    case '.':   buf += "0."; state = NUMBER;     break;
    case '[':                state = ARRAY;      break;
    case '(':                state = STRING;     break;
    case '<':                state = HEXSTRING;  break;
    default :   buf = "";    state = NEWSYMBOL;  break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::identifierState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case ' ':   if (buf == "BI") state = WAIT; else
                pushbuf("identifier", NEWSYMBOL);           break;
    case '/':   pushbuf("identifier", RESOURCE); buf = "/"; break;
    case '[':   pushbuf("identifier", ARRAY);               break;
    case '(':   pushbuf("identifier", STRING);              break;
    case '<':   pushbuf("identifier", HEXSTRING);           break;
    case 'L':   buf += m;   break;
    case 'D':   buf += m;   break;
    case '-':   buf += m;   break;
    case '_':   buf += m;   break;
    case '*':   buf += m;   break;
    default:    break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::numberState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case 'L':   buf += m; break;
    case 'D':   buf += m; break;
    case '_':   buf += m; break;
    case '.':   buf += m; break;
    case '-':   pushbuf("number",     NUMBER);    buf = "-";
                pushbuf("operator",   NUMBER);    break;
    case '*':   pushbuf("number",     NUMBER);    buf = "*";
                pushbuf("operator",   NUMBER);    break;
    case '/':   pushbuf("number",     NUMBER);    buf = "/";
                pushbuf("operator",   NUMBER);    break;
    case ' ':   pushbuf("number",     NEWSYMBOL); break;
    case '[':   pushbuf("number",     ARRAY);     break;
    case '(':   pushbuf("number",     STRING);    break;
    case '<':   pushbuf("identifier", HEXSTRING); break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::stringState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case ')':   pushbuf("string", NEWSYMBOL); break;
    case '\\':  escapeState(); break;
    default:    buf += s[i];   break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::escapeState()
{
  char n = symbol_type(s[++i]);
  if (n == 'D')
  {
    int octcount = 0;
    pushbuf("string", STRING);
    while(n == 'D' && octcount < 3)
    {
      buf += s[i++];
      octcount++;
      n = symbol_type(s[i]);
    }
    int newint = oct2dec(stoi(buf));
    buf = intToHexstring(newint);
    pushbuf("hexstring", STRING);
    i--;
  }
  else
    buf += s[i];
}

/*---------------------------------------------------------------------------*/

void tokenizer::arrayState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case '\\':  buf += s[i++]; buf += s[i];  break;
    case ']':   tokenize_array(buf); state = NEWSYMBOL; buf = ""; break;
    default:    buf += m; break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::hexstringState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case '>':   if (!buf.empty()) pushbuf("hexstring", NEWSYMBOL);
                state = NEWSYMBOL;      break;
    case '<':   buf = ""; state = DICT; break;
    case '\\':  buf += m + s[++i];      break;
    default:    buf += m;               break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::dictState()
{
  char m = s[i];
  char n = symbol_type(m);
  switch(n)
  {
    case '\\':  buf += m + s[++i];          break;
    case '>':   pushbuf("dict", HEXSTRING); break;
    default:    buf += m;                   break;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::waitState()
{
  buf =  s[i] + s[i + 1];
  buf += symbol_type(s[i + 2]);
  if(buf == "EI ")
  {
    buf.clear();
    state = NEWSYMBOL;
  }
}

/*---------------------------------------------------------------------------*/

void tokenizer::tokenize_array(string &str)
{
  string buffer, minibuffer;
  str.push_back(' ');
  TState Arrstate = NEWSYMBOL;
  size_t i = 0;

  while(i < str.length())
  {
    char m = str[i];
    char n = symbol_type(m);
    switch(Arrstate)
    {
      case NEWSYMBOL:
      {
        switch(n)
        {
          case 'D': buffer += m;
                    Arrstate = NUMBER;
                    break;
          case '.': buffer += '0';
                    buffer += '.';
                    Arrstate = NUMBER;
                    break;
          case '-': buffer += m;
                    Arrstate = NUMBER;
                    break;
          case '(': Arrstate = STRING;
                    break;
          case '<': Arrstate = HEXSTRING;
                    break;
          default:  Arrstate = NEWSYMBOL;
                    buffer ="";
                    break;
        }
        break;
      }

      case NUMBER:
      {
        switch(n)
        {
          case 'L': buffer += m;
                    break;
          case 'D': buffer += m;
                    break;
          case '.': buffer += '.';
                    break;
          case '-': token.push_back(buffer);
                    buffer = "";
                    token.push_back("-");
                    ttype.push_back("number");
                    ttype.push_back("operator");
                    break;
          case '_': buffer += '_';
                    break;
          case '*': token.push_back(buffer);
                    buffer = "";
                    token.push_back("*");
                    ttype.push_back("number");
                    ttype.push_back("operator");
                    break;
          case '/': token.push_back(buffer);
                    buffer = "";
                    token.push_back("/");
                    ttype.push_back("number");
                    ttype.push_back("operator");
                    break;
          case ' ': Arrstate = NEWSYMBOL;
                    token.push_back(buffer);
                    buffer = "";
                    ttype.push_back("number");
                    break;
          case '(': Arrstate = STRING;
                    token.push_back(buffer);
                    buffer = "";
                    ttype.push_back("number");
                    break;
          case '<': Arrstate = HEXSTRING;
                    token.push_back(buffer);
                    buffer = "";
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
                      if (symbol_type(str[i]) == 'D')
                      {
                        token.push_back(buffer); buffer = "";
                        ttype.push_back("string");
                        int octcount = 0;
                        while(symbol_type(str[i]) == 'D' && octcount < 3)
                        {
                          minibuffer += str[i]; i++; octcount++;
                        }
                        int newint = oct2dec(stoi(minibuffer));
                        string tmpstr = intToHexstring(newint);
                        token.push_back(tmpstr);
                        ttype.push_back("hexstring");
                        minibuffer = "";
                        i--;
                      }
                      else
                      {
                        buffer += str[i];
                      }
                      break;
          case ')':   token.push_back(buffer);
                      buffer = "";
                      ttype.push_back("string");
                      Arrstate = NEWSYMBOL;
                      break;
          default :   buffer += m;
                      break;
        }
        break;
      }

      case HEXSTRING:
      {
        switch(n)
        {
          case '\\':  buffer += m + str[i+1];
                      i++;
                      break;
          case '>':   token.push_back(buffer);
                      buffer = "";
                      ttype.push_back("hexstring");
                      Arrstate = NEWSYMBOL;
                      break;
          default:    buffer += m;
                      break;
        }
        break;
      }
    default: break;
    }
  ++i;
  }
  str = "";
}

/*---------------------------------------------------------------------------*/
