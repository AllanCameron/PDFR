#include "pdfr.h"
#include "stringfunctions.h"
#include "document.h"
#include "encodings.h"
#include "corefonts.h"
#include "font.h"

font::font(document& d, const dictionary& Fontref, const std::string& fontid) :
FontID(fontid)
{
  dictionary fontref = Fontref;
  FontRef = "In file";
  std::string BaseFont = fontref.get("/BaseFont");
  getCoreFont(BaseFont);
  size_t BaseFont_start = 1;
  if(BaseFont.size() > 7)
    if(BaseFont[7] == '+') BaseFont_start = 8;
  FontName = BaseFont.substr(BaseFont_start, BaseFont.size() - BaseFont_start);
  getEncoding(fontref, d);
  mapUnicode(fontref, d);
  if(Width.size() == 0) getWidthTable(fontref, d);
  makeGlyphTable();
}



void parseDifferences(const std::string& enc, EncMap& symbmap)
{
  std::string state = "newsymbol";
  std::string buf = "";
  std::string minibuf = "";
  std::vector<std::string> typestring;
  std::vector<std::string> symbstring;

  for(auto i : enc)
  {
    if(state == "stop") break;
    if(state == "newsymbol")
    {
      char n = symbol_type(i);
      switch(n)
      {
      case 'D': buf = i ; state = "number"; break;
      case '/': buf = "/"; state = "name"; break;
      case ']': state = "stop"; break;
      default : buf = ""; break;
      }
      i++; continue;
    }
    if(state == "number")
    {
      char n = symbol_type(i);
      switch(n)
      {
      case 'D': buf += i ; break;
      case '/': typestring.push_back(state);
        symbstring.push_back(buf);
        buf = "/"; state = "name"; break;
      default:  typestring.push_back(state);
      symbstring.push_back(buf);
      buf = ""; state = "newsymbol"; break;
      }
      i++; continue;
    }
    if(state == "name")
    {
      char n = symbol_type(i);
      switch(n)
      {
      case 'L': buf += i;  break;
      case '.': buf += i;  break;
      case 'D': typestring.push_back(state);
        symbstring.push_back(buf);
        buf = i ; state = "number"; break;
      case '/': typestring.push_back(state);
        symbstring.push_back(buf);
        buf = i ; break;
      case ' ': typestring.push_back(state);
        symbstring.push_back(buf);
        buf = "" ; state = "newsymbol"; break;
      default: typestring.push_back(state);
      symbstring.push_back(buf);
      state = "stop"; break;
      }
      i++; continue;
    }
  }

  size_t k = 0;
  for(size_t i = 0; i < symbstring.size(); i++)
  {
    if(typestring[i] == "number") {k = std::stoi(symbstring[i]);}
    else {symbmap[k] = symbstring[i]; k++;}
  }
}

/*---------------------------------------------------------------------------*/


std::vector<std::pair<std::string, int>> font::mapString(const std::string& s)
{
  GlyphMap &G = glyphmap;
  std::vector<std::pair<std::string, int>> res;
  for(auto i : strtoint(s)) if(G.find(i) != G.end()) res.push_back(G[i]);
  return res;
}

/*---------------------------------------------------------------------------*/


void font::getWidthTable(dictionary& dict, document& d)
{
  std::map<uint16_t, int> resmap;
  std::vector<int> widtharray, chararray;
  int firstchar;
  std::string widthstrings, fcstrings;
  std::string numstring = "\\[(\\[|]| |\\d+)+";
  if (dict.has("/Widths"))
  {
    widthstrings = dict.get("/Widths");
    fcstrings = dict.get("/FirstChar");
    if (dict.hasRefs("/Widths"))
    {
      std::vector<int> os = dict.getRefs("/Widths");
      if (os.size() > 0)
      {
        object_class o = d.getobject(os[0]);
        std::string ostring = o.getStream();
        std::vector<std::string> arrstrings = Rex(ostring, numstring);
        if (arrstrings.size() > 0) widtharray = getints(arrstrings[0]);
      }
    }
    else widtharray = dict.getInts("/Widths");
    if (widtharray.size() > 0 && fcstrings.size() > 0)
    {
      std::vector<int> fcnums = getints(fcstrings);
      if (fcnums.size() > 0)
      {
        firstchar = fcnums[0];
        for (unsigned i = 0; i < widtharray.size(); i++)
        {
          resmap[(uint16_t) firstchar + i] = widtharray[i];
        }
        Width = resmap;
      }
    }
  }
  else
  {
    widthstrings = dict.get("/DescendantFonts");
    if(dict.has("/DescendantFonts"))
    {
      if (dict.hasRefs("/DescendantFonts"))
      {
        std::vector<int> os = dict.getRefs("/DescendantFonts");
        if (!os.empty())
        {
          object_class desc = d.getobject(os[0]);
          dictionary descdict = desc.getDict();
          std::string descstream = desc.getStream();
          if(!getObjRefs(descstream).empty())
            descdict = d.getobject(getObjRefs(descstream)[0]).getDict();
          if (descdict.has("/W"))
          {
            if (descdict.hasRefs("/W"))
            {
              std::vector<int> osss = descdict.getRefs("/W");
              if (osss.size() > 0) widthstrings =
                d.getobject(osss[0]).getStream();
            }
            else widthstrings = descdict.get("/W");
            std::vector<std::string> tmp = Rex(widthstrings, numstring);
            if(tmp.size() > 0)
            {
              parsewidtharray(tmp[0]);
            }
          }
        }
      }
    }
  }
  if(Width.size() == 0)
  {
    EncMap storeEnc = EncodingMap;
    getCoreFont("/Helvetica");
    EncodingMap = storeEnc;
  }
}

/*---------------------------------------------------------------------------*/

void font::mapUnicode(dictionary& fontref, document& d)
{
  if (fontref.hasRefs("/ToUnicode"))
  {
    std::vector<int> unirefints = fontref.getRefs("/ToUnicode");
    if (unirefints.size() > 0)
    {
      int unirefint = unirefints[0];
      std::string x = d.getobject(unirefint).getStream();
      std::string hexfinder = "(\\d|a|b|c|d|e|f|A|B|C|D|E|F)+";
      bool hasChar =  (stringloc(x, "beginbfchar",  "end").size() > 0) &&
                      (stringloc(x, "endbfchar",  "start").size() > 0);
      bool hasRange = (stringloc(x, "beginbfrange", "end").size() > 0) &&
                      (stringloc(x, "endbfrange", "start").size() > 0);

      if (hasChar)
      {
        std::vector<std::string> sv = Rex(x, "beginbfchar(.|\\s)*?endbfchar");
        for(auto j : sv)
        {
          j = carveout(j, "beginbfchar", "endbfchar");
          std::vector<std::string> charentries = splitter(j, "(\n|\r){1,2}");
          for (auto i : charentries)
          {
            std::vector<std::string> rowentries = Rex(i, hexfinder);
            if (rowentries.size() == 2)
            {
              std::string myhex = rowentries[1];
              if(myhex.length() < 4)
                while(myhex.length() < 4) myhex = "0" + myhex;
              std::transform( myhex.begin(), myhex.end(), myhex.begin(),
                              std::ptr_fun<int, int>(std::toupper));
              std::string mykey = rowentries[0];
              std::transform( mykey.begin(), mykey.end(), mykey.begin(),
                              std::ptr_fun<int, int>(std::toupper));
              if(mykey.length() < 4)
                while(mykey.length() < 4) mykey = "0" + mykey;
              mykey = "0x" + mykey.substr(2, 2);
              uint16_t uintkey = stoul(mykey, nullptr, 0);
              if(EncodingMap.find(uintkey) != EncodingMap.end())
                EncodingMap[uintkey] = d.UCMap[myhex];
              else EncodingMap[uintkey] = d.UCMap[myhex];
            }
          }
        }
      }
      if (hasRange)
      {
        std::vector<std::string> sv = Rex(x, "beginbfrange(.|\\s)*?endbfrange");
        for(auto j : sv)
        {
          std::string bfrange = carveout(j, "beginbfrange", "endbfrange");
          std::vector<std::string> Rangenums =
            splitter(bfrange, "(\n|\r){1,2}");

          for (auto i : Rangenums)
          {
            std::vector<std::string> rowentries = Rex(i, hexfinder);
            if (rowentries.size() == 3)
            {
              std::string myhex0 = "0x" + rowentries[0];
              std::string myhex1 = "0x" + rowentries[1];
              std::string myhex2 = "0x" + rowentries[2];
              unsigned int myui0 = stoul(myhex0, nullptr, 0);
              unsigned int myui1 = stoul(myhex1, nullptr, 0);
              unsigned int myui2 = stoul(myhex2, nullptr, 0);
              unsigned int nui = (myui1 - myui0) + 1;
              for (unsigned int j = 0; j < nui; j++)
              {
                uint16_t myui = myui2 + j;
                uint16_t mykey = myui0 + j;
                std::string newHex = intToHexstring((int) myui);
                if(EncodingMap.find(mykey) != EncodingMap.end())
                {
                  EncodingMap[mykey] = d.UCMap[newHex];
                }
                else EncodingMap[mykey] = d.UCMap[newHex];
              }
            }
          }
        }
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

void font::getEncoding(dictionary& fontref, document& d)
{
  dictionary &encref = fontref;
  std::string encname = encref.get("/Encoding");
  if(fontref.hasRefs("/Encoding"))
  {
    int a = fontref.getRefs("/Encoding").at(0);
    object_class myobj = d.getobject(a);
    encref = myobj.getDict();
    encname = encref.get("/BaseEncoding");
  }
  if( encname == "/WinAnsiEncoding" ||
      encname == "/MacRomanEncoding" ||
      encname == "/PDFDocEncoding" ||
      encname == "/StandardEncoding")
  {
    BaseEncoding = encname;
    EncodingMap = getBaseEncode(encname);
  }
  else
  {
    std::vector<int> encrefs = getObjRefs(encname);
    if(!encrefs.empty())
    {
      dictionary encdict = d.getobject(encrefs[0]).getDict();
      BaseEncoding = encdict.get("/Differences");
      EncodingMap = getBaseEncode("/StandardEncoding");
      parseDifferences(BaseEncoding, EncodingMap);
    }
    else
    {
      BaseEncoding = "/StandardEncoding";
      EncodingMap = getBaseEncode(BaseEncoding);
    }
  }
}


void font::getCoreFont(std::string s)
{
  font resfont;
  if(s == "/Courier") *this = getCourier();
  if(s == "/Courier-Bold") *this = getCourierBold();
  if(s == "/Courier-BoldOblique") *this = getCourierBO();
  if(s == "/Courier-Oblique") *this = getCourierOblique();
  if(s == "/Helvetica") *this = getHelvetica();
  if(s == "/Helvetica-Bold") *this = getHelveticaBold();
  if(s == "/Helvetica-Boldoblique") *this = getHelveticaBO();
  if(s == "/Helvetica-Oblique") *this = getHelveticaOblique();
  if(s == "/Symbol") *this = getSymbol();
  if(s == "/Times-Bold") *this = getTimesBold();
  if(s == "/Times-BoldItalic") *this = getTimesBI();
  if(s == "/Times-Italic") *this = getTimesItalic();
  if(s == "/Times-Roman") *this = getTimesRoman();
  if(s == "/ZapfDingbats") *this = getDingbats();
}


void font::makeGlyphTable()
{
  std::vector<uint16_t> widthkeys = getKeys(Width);
  int defwidth = 500;
  std::vector<uint16_t> inkeys = getKeys(EncodingMap);
  for(size_t i = 0; i < inkeys.size(); i++)
  {
    int thiswidth;
    if(Width.find(inkeys[i]) == Width.end()) thiswidth = defwidth;
    else thiswidth = Width[(int) inkeys[i]];
    glyphmap[inkeys[i]] = std::make_pair(EncodingMap[inkeys[i]], thiswidth);
  }
}


void font::parsewidtharray(std::string s)
{
  s += " ";
  std::string buf, state;
  std::vector<int> vecbuf, resultint;
  std::vector<std::vector<int>> resultvec;
  state = "newsymb";
  buf = "";
  size_t i = 0;
  if(s.length() > 0)
  {
    while(i < s.length())
    {
      char a = symbol_type(s[i]);
      if(state == "newsymb")
      {
        switch(a)
        {
          case '[': state = "inarray"; break;
          default : break;
        }
        i++; continue;
      }
      if(state == "inarray")
      {
        switch(a)
        {
        case 'D' : buf += s[i]; break;
        case '[': state = "insubarray";
                  if(buf.size() > 0)
                  {
                    vecbuf.push_back(stoi(buf));
                    if(vecbuf.size() == 1) resultint.push_back(vecbuf[0]);
                    else resultvec.push_back(vecbuf);
                  }
                  buf = "";
                  vecbuf.clear();
                  break;
        case ' ': if(buf.size() > 0)
                  {
                    vecbuf.push_back(stoi(buf));
                  }
                  buf = ""; break;
        case ']': state = "end";
                  if(buf.size() > 0)
                  {
                    vecbuf.push_back(stoi(buf));
                    if(vecbuf.size() == 1) resultint.push_back(vecbuf[0]);
                    else resultvec.push_back(vecbuf);
                  }
                  buf = "";
                  vecbuf.clear();
                  break;
        default: Rcpp::stop("Error parsing string " + s);

        }
        i++; continue;
      }
      if(state == "insubarray")
      {
        switch(a)
        {
        case ' ': if(buf.size() > 0) vecbuf.push_back(stoi(buf));
                  buf = ""; break;
        case ']': state = "inarray";
                  if(buf.size() > 0) vecbuf.push_back(stoi(buf));
                  resultvec.push_back(vecbuf);
                  vecbuf.clear();
                  buf = ""; break;
        case 'D': buf += s[i]; break;
        default: Rcpp::stop("Error parsing string " + s);
        }
        i++; continue;
      }
      if(state == "end")
      {
        break;
      }
    }
  }
  std::map<uint16_t, int> resultmap;

  if((resultint.size() == resultvec.size()) && (resultint.size() > 0))
  {
    for(size_t i = 0; i < resultint.size(); i++)
    {
      if(resultvec[i].size() > 0)
      {
        for(size_t j = 0; j < resultvec[i].size(); j++)
        {
          resultmap[(uint16_t) resultint[i] + j] = resultvec[i][j];
        }
      }
    }
  }
  Width = resultmap;
}
