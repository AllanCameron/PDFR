#include "pdfr.h"
#include "stringfunctions.h"
#include "dictionary.h"

/*---------------------------------------------------------------------------*/

void dictionary_parser(std::vector<std::vector<std::string>>& s)
{
  std::vector<std::string> tmpident, tmptoken;
  std::vector<std::string> &ttype = s[1];
  std::vector<std::string> &token = s[0];
  std::vector<std::vector<std::string>> tmpres, res;
  for(unsigned i = 0; i < ttype.size(); i++)
  {
    if(ttype[i] == "keyname")
    {
      tmptoken.push_back(token[i]);
      tmpident.push_back("true");
    }
    if(ttype[i] == "stream")
    {
      tmpident.push_back(token[i]);
      tmptoken.push_back("stream");
    }
    if(ttype[i] == "value" || ttype[i] == "subdictionary")
    {
      tmpident.pop_back();
      tmpident.push_back(token[i]);
    }
    if(i > 0 && i < ttype.size() && tmpident.size() > 0)
    {
      if (ttype[i] == "keyname" && ttype[i - 1] == "keyname" &&
          tmpident[tmpident.size() - 2] == "true")
      {
        tmptoken.pop_back();
        tmpident.pop_back();
        tmpident.back() = token[i];
      }
    }
  }
  res.push_back(tmptoken);
  res.push_back(tmpident);
  s = res;
}

/*---------------------------------------------------------------------------*/

std::map<std::string, std::string>
tokenize_dict(const std::string& s, unsigned pos)
{
  std::vector<std::string> token, ttype;

  if(s.length() > 0)
  {
    unsigned i = pos;
    std::string state = "preentry";
    std::string buf;
    int minibuf = 0;
    while(i < s.length() && i < (pos + 100000))
    {
      if(state == "preentry")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case '<': state = "maybe"; break;
        default: buf =""; break;
        }
        i++;
        continue;
      }
      if(state == "maybe")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case '<': state = "start"; break;
        default: buf =""; state = "preentry"; break;
        }
        i++;
        continue;
      }
      if(state == "start")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case '/': buf += '/'; state = "key"; break;
        case '>': state = "queryclose"; break;
        default : break;
        }
        i++;
        continue;
      }

      if(state == "key")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case 'L': buf += s[i]; break;
        case 'D': buf += s[i]; break;
        case '+': buf += s[i]; break;
        case '-': buf += s[i]; break;
        case '_': buf += s[i]; break;
        case '/': token.push_back(buf);  ttype.push_back("keyname");
        buf = '/'; state = "key"; break;
        case ' ': state = "prevalue";
          token.push_back(buf);
          buf = "";
          ttype.push_back("keyname"); break;
        case '[': state = "arrayval";
          token.push_back(buf);
          buf = "[";
          ttype.push_back("keyname"); break;
        case '<': state = "querydict";
          token.push_back(buf);
          buf = "";
          ttype.push_back("keyname"); break;
        case '>': state = "queryclose";
          token.push_back(buf);
          buf = "";
          ttype.push_back("keyname"); break;
        }
        i++;
        continue;
      }
      if(state == "prevalue")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case ' ': state = "prevalue"; break;
        case '<': state = "querydict"; break;
        case '>': state = "queryclose"; break;
        case '/': buf = '/'; state = "key"; break;
        case '[': buf = '['; state = "arrayval"; break;
        default : buf = s[i]; state = "value"; break;
        }
        i++;
        continue;
      }
      if(state == "value")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case '/': token.push_back(buf); buf = "/";
        ttype.push_back("value");
        state = "key"; break;
        case ' ': buf += ' '; break;
        case '<': state = "querydict";
          token.push_back(buf);
          buf = "";
          ttype.push_back("value"); break;
        case '>': state = "queryclose";
          token.push_back(buf);
          buf = "";
          ttype.push_back("value"); break;
        default : buf += s[i]; break;
        }
        i++;
        continue;
      }

      if(state == "arrayval")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case ']': buf += "]";
          token.push_back(buf);
          buf = "";
          ttype.push_back("value");
          state = "start"; break;
        default:  buf += s[i]; break;
        }
        i++;
        continue;
      }

      if(state == "querydict")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case '<': buf = "<<"; state = "subdict"; minibuf = 2; break;
        default: buf = ""; state = "start"; break;
        }
        i++;
        continue;
      }

      if(state == "queryclose")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case '>': state = "close"; break;
        default: state = "start"; break;
        }
        i++;
        continue;
      }

      if(state == "close")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case ' ': state = "close"; break;
        case 'L': if(i < s.length() - 7)
        {
          if(s[i] == 's' && s[i + 1] == 't' &&
             s[i + 2] == 'r' && s[i + 3] == 'e' &&
             s[i + 4] == 'a' && s[i + 5] == 'm')
          {
            ttype.push_back("stream");
            int ex = 7;
            while(symbol_type(s[i + ex]) == ' ') ex++;
            token.push_back(std::to_string(i + ex));
          }
        }
        state = "finished"; break;
        default: state = "finished"; break;
        }
        i++;
        continue;
      }
      if(state == "finished")
      {
        break;
      }

      if(state == "subdict")
      {
        char n = symbol_type(s[i]);
        switch(n)
        {
        case '<': buf += s[i]; minibuf ++; break;
        case '>': buf += s[i]; minibuf --; break;
        default: buf += s[i]; break;
        }
        if (minibuf == 0)
        {
          token.push_back(buf); buf = "";
          ttype.push_back("subdictionary");
          state = "start";
        }
        i++;
        continue;
      }

      else
      {
        i++;
        continue;
      }
    }
  }
  std::vector<std::vector<std::string> > res;
  res.push_back(token);
  res.push_back(ttype);
  dictionary_parser(res);
  std::map<std::string, std::string> resmap;
  if(res[0].size() > 0) for(unsigned i = 0; i < res[0].size(); i++)
  {
    resmap[res[0][i]] = res[1][i];
  }
  return resmap;
}

/*---------------------------------------------------------------------------*/

dictionary::dictionary(const std::string& s)
{
  DictionaryMap = tokenize_dict(s, 0);
}

dictionary::dictionary(const std::string& s, const int& i)
{
  DictionaryMap = tokenize_dict(s, (unsigned) i);
}

/*---------------------------------------------------------------------------*/

dictionary::dictionary()
{
  std::map<std::string, std::string> Empty;
  DictionaryMap = Empty;
}

/*---------------------------------------------------------------------------*/

std::string dictionary::get(const std::string& Key)
{
  return DictionaryMap[Key];
}

/*---------------------------------------------------------------------------*/


bool dictionary::has(const std::string& Key)
{
  return DictionaryMap.find(Key) != DictionaryMap.end();
}

/*---------------------------------------------------------------------------*/


bool dictionary::hasRefs(const std::string& Key)
{
  if(this->has(Key)) return this->getRefs(Key).size() > 0; return false;
}

/*---------------------------------------------------------------------------*/

bool dictionary::hasInts(const std::string& Key)
{
  if(this->has(Key)) return this->getInts(Key).size() > 0; return false;
}

/*---------------------------------------------------------------------------*/


std::vector<int> dictionary::getRefs(const std::string& Key)
{
  std::vector<int> References;
  if(this->has(Key))
    for (auto i : Rex(this->get(Key), "\\d+ 0 R"))
      References.push_back(stoi(splitter(i, " ").at(0)));
  return References;
}

/*---------------------------------------------------------------------------*/

std::vector<int> dictionary::getInts(const std::string& Key)
{
  std::vector<int> blank;
  if(this->has(Key)) return getints(this->get(Key));
  return blank;
}

/*---------------------------------------------------------------------------*/

std::vector<float> dictionary::getNums(const std::string& Key)
{
  std::vector<float> blank;
  if(this->has(Key)) return getnums(this->get(Key));
  return blank;
}

/*---------------------------------------------------------------------------*/

dictionary dictionary::getDictionary(const std::string& Key)
{
  if(this->has(Key))
  {
    std::string dict = this->get(Key);
    if(isDictString(dict)) return dictionary(dict);
  }
  return dictionary();
}

/*---------------------------------------------------------------------------*/

bool dictionary::hasDictionary(const std::string& Key)
{
  if(this->has(Key)) return isDictString(this->get(Key));

  return false;
}

/*---------------------------------------------------------------------------*/

std::vector<std::string> dictionary::getDictKeys()
{
  return getKeys(this->DictionaryMap);
}
