//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR dictionary implementation file                                      //
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
#include "Rex.h"
#include "stringfunctions.h"
#include "dictionary.h"

using namespace std;

/*---------------------------------------------------------------------------*/

void dictionary::tokenize_dict(const string& s, unsigned pos)
{
  if(s.length() > 0)
  {
    unsigned i = pos;
    string state = "preentry";
    string buf, pendingKey;
    bool keyPending = false;
    int minibuf = 0;
    char n = symbol_type(s[i]);
    while(i < s.length() && i < (pos + 100000))
    {
      if(state == "preentry")
      {
        switch(n)
        {
        case '<': state = "maybe"; break;
        default: buf =""; break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "maybe")
      {
        switch(n)
        {
        case '<': state = "start"; break;
        default: buf =""; state = "preentry"; break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "start")
      {
        switch(n)
        {
        case '/': buf += '/'; state = "key"; break;
        case '>': state = "queryclose"; break;
        default : break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "key")
      {
        switch(n)
        {
        case 'L': buf += s[i]; break;
        case 'D': buf += s[i]; break;
        case '+': buf += s[i]; break;
        case '-': buf += s[i]; break;
        case '_': buf += s[i]; break;
        case '/': if(!keyPending)
                  {
                    pendingKey = buf;  keyPending = true;
                  }
                  else
                  {
                    DictionaryMap[pendingKey] = buf; keyPending = false;
                  }
                  buf = '/'; state = "key"; break;
        case ' ': if(!keyPending)
                  {
                    pendingKey = buf;  keyPending = true;
                  }
                  else
                  {
                    DictionaryMap[pendingKey] = buf; keyPending = false;
                  }
                  state = "prevalue";
                  buf = ""; break;
        case '(': if(!keyPending)
                  {
                    pendingKey = buf;  keyPending = true;
                  }
                  else
                  {
                    DictionaryMap[pendingKey] = buf; keyPending = false;
                  }
                  state = "string";
                  buf = "("; break;
        case '[': if(!keyPending)
                  {
                    pendingKey = buf;  keyPending = true;
                  }
                  else
                  {
                    DictionaryMap[pendingKey] = buf; keyPending = false;
                  }
                  state = "arrayval";
                  buf = "["; break;
        case '<': if(!keyPending)
                  {
                    pendingKey = buf;  keyPending = true;
                  }
                  else
                  {
                    DictionaryMap[pendingKey] = buf; keyPending = false;
                  }
                  state = "querydict";
                  buf = ""; break;
        case '>': if(!keyPending)
                  {
                    pendingKey = buf;  keyPending = true;
                  }
                  else
                  {
                    DictionaryMap[pendingKey] = buf; keyPending = false;
                  }
                  state = "queryclose";
                  buf = ""; break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "prevalue")
      {
        switch(n)
        {
          case ' ': state = "prevalue"; break;
          case '<': state = "querydict"; break;
          case '>': state = "queryclose"; break;
          case '/': buf = '/'; state = "key"; break;
          case '[': buf = '['; state = "arrayval"; break;
          default : buf = s[i]; state = "value"; break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "value")
      {
        switch(n)
        {
          case '/': DictionaryMap[pendingKey] = buf; keyPending = false; buf = "/";
                    state = "key"; break;
          case ' ': buf += ' '; break;
          case '<': DictionaryMap[pendingKey] = buf; keyPending = false;
                    state = "querydict";
                    buf = ""; break;
          case '>': DictionaryMap[pendingKey] = buf; keyPending = false;
                    state = "queryclose";
                    buf = ""; break;
          default : buf += s[i]; break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "arrayval")
      {
        switch(n)
        {
          case ']': buf += "]";
                    DictionaryMap[pendingKey] = buf; keyPending = false;
                    buf = "";
                    state = "start"; break;
          default:  buf += s[i]; break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "string")
      {
        switch(n)
        {
          case ')': buf += ")";
                    DictionaryMap[pendingKey] = buf; keyPending = false;
                    buf = "";
                    state = "start"; break;
          default:  buf += s[i]; break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "querydict")
      {
        switch(n)
        {
          case '<': buf = "<<"; state = "subdict"; minibuf = 2; break;
          default: buf = ""; state = "start"; break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "queryclose")
      {
        switch(n)
        {
          case '>': state = "close"; break;
          default: state = "start"; break;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "close")
      {
        switch(n)
        {
        case ' ': state = "close"; break;
        case 'L': if(i < s.length() - 7)
                  {
                    if(s[i] == 's' && s[i + 1] == 't' &&
                       s[i + 2] == 'r' && s[i + 3] == 'e' &&
                       s[i + 4] == 'a' && s[i + 5] == 'm')
                    {
                      int ex = 7;
                      while(symbol_type(s[i + ex]) == ' ') ex++;
                      DictionaryMap["stream"] = to_string(i + ex);
                    }
                  }
                  return;
        default: return;
        }
        i++; n = symbol_type(s[i]); continue;
      }

      if (state == "subdict")
      {
        switch(n)
        {
          case '<': buf += s[i]; minibuf ++; break;
          case '>': buf += s[i]; minibuf --; break;
          default: buf += s[i]; break;
        }
        if (minibuf == 0)
        {
          DictionaryMap[pendingKey] = buf; keyPending = false; buf = "";
          state = "start";
        }
        i++; n = symbol_type(s[i]); continue;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

dictionary::dictionary(const string& s)
{
  tokenize_dict(s, 0);
}

/*---------------------------------------------------------------------------*/

dictionary::dictionary(const string& s, const int& i)
{
  tokenize_dict(s, (unsigned) i);
}

/*---------------------------------------------------------------------------*/

dictionary::dictionary()
{
  map<string, string> Empty;
  DictionaryMap = Empty;
}

/*---------------------------------------------------------------------------*/

string dictionary::get(const string& Key)
{
  return DictionaryMap[Key];
}

/*---------------------------------------------------------------------------*/

bool dictionary::has(const string& Key)
{
  return DictionaryMap.find(Key) != DictionaryMap.end();
}

/*---------------------------------------------------------------------------*/

bool dictionary::hasRefs(const string& Key)
{
  if(this->has(Key)) return this->getRefs(Key).size() > 0; return false;
}

/*---------------------------------------------------------------------------*/

bool dictionary::hasInts(const string& Key)
{
  if(this->has(Key)) return this->getInts(Key).size() > 0; return false;
}

/*---------------------------------------------------------------------------*/

vector<int> dictionary::getRefs(const string& Key)
{
  vector<int> References;
  if(this->has(Key))
  {
    string keyval = this->get(Key);
    string refmatch = "(0|1|2|3|4|5|6|7|8|9)+ (0|1|2|3|4|5|6|7|8|9) R";
    Rex refrex = Rex(keyval, refmatch);
    vector<string> refs = refrex.get();
    for (auto i : refs)
      References.push_back(stoi(i));
  }
  return References;
}

/*---------------------------------------------------------------------------*/

vector<int> dictionary::getInts(const string& Key)
{
  vector<int> blank;
  if(this->has(Key)) return getints(this->get(Key));
  return blank;
}

/*---------------------------------------------------------------------------*/

vector<float> dictionary::getNums(const string& Key)
{
  vector<float> blank;
  if(this->has(Key)) return getnums(this->get(Key));
  return blank;
}

/*---------------------------------------------------------------------------*/

dictionary dictionary::getDictionary(const string& Key)
{
  if(this->has(Key))
  {
    string dict = this->get(Key);
    if(isDictString(dict)) return dictionary(dict);
  }
  return dictionary();
}

/*---------------------------------------------------------------------------*/

bool dictionary::hasDictionary(const string& Key)
{
  if(this->has(Key)) return isDictString(this->get(Key));

  return false;
}

/*---------------------------------------------------------------------------*/

vector<string> dictionary::getDictKeys()
{
  return getKeys(this->DictionaryMap);
}
