#include "pdfr.h"
#include "Rex.h"
#include "stringfunctions.h"
#include "debugtools.h"

/*---------------------------------------------------------------------------*/
// Finds matches m in string s, returning a vector of (matches + 1) substrings
std::vector<std::string> splitter(const std::string& s, const std::string& m)
{
  Rex sl = Rex(s, m);
  std::vector<std::string> RC, res;
  if(!sl.has())
  {
    return sl.get();
  }
  size_t n = (size_t) sl.n();
  RC.push_back(s.substr(0, sl.pos().at(0)));
  if(n > 1)
    for(size_t i = 1; i < n; i++)
      RC.push_back(s.substr(sl.ends().at(i - 1),
                            sl.pos().at(i) - sl.ends().at(i - 1)));
  RC.push_back(s.substr(sl.ends().at(n - 1), s.size() - sl.ends().at(n - 1)));
  for(auto i : RC)
    if(!i.empty())
      res.push_back(i);
  return res;
}

/*---------------------------------------------------------------------------*/
// Return the first substring of s that lies between two regexes
std::string carveout(const std::string& s,
                const std::string& precarve,
                const std::string& postcarve)
{
  int firstpos  = 0;
  int secondpos = s.length();
  std::vector<int> FPV = Rex(s, precarve).ends();
  std::vector<int> SPV = Rex(s, postcarve).pos();
  int fpvs = FPV.size();
  int spvs = SPV.size();
  // Ensure the match lies between the first balanced matches
  if((fpvs == 0) && (spvs > 0)) secondpos = SPV.at(spvs - 1);
  if((fpvs > 0) && (spvs == 0)) firstpos = FPV.at(0);
  if((fpvs > 0) && (spvs > 0))
  {
    firstpos = FPV.at(0);
    for(auto i : SPV)
      if(i > firstpos)
      {
        secondpos = i;
        break;
      }
  }
  std::string res(s.begin() + firstpos, s.begin() + secondpos);
  return res;
}

/*---------------------------------------------------------------------------*/
// Decent approximation of whether a string contains binary data or not
bool IsAscii(const std::string& tempint)
{
  int mymin = *std::min_element(tempint.begin(), tempint.end());
  int mymax = *std::max_element(tempint.begin(), tempint.end());
  return (mymin > 7) && (mymax < 126);
}

/*---------------------------------------------------------------------------*/
//Casts a string to vector of unsigned ints
std::vector<uint16_t> strtoint(std::string x)
{
  std::vector<uint16_t> res;
  for(auto i : x)
  {
    uint16_t a = i;
    res.push_back(a % 255);
  }
  return res;
}

/*---------------------------------------------------------------------------*/
// Casts a single unsigned int to a length 1 string
std::string intToString(uint16_t a)
{
  std::string b;
  if (a > 0x00ff) return "*";
  uint8_t d = (uint8_t) a;
  b += (char) d;
  return b;
}

/*---------------------------------------------------------------------------*/
// Generalizes stof to allow multiple floats from a single string
std::vector<float> getnums(const std::string& s)
{
  std::vector<float> res;
  std::string numstring = "(-)?(\\.)?\\d+(\\.)?\\d*"; // float regex
  std::vector<std::string> strs = Rex(s, numstring).get();
  for(auto i : strs)
    res.push_back(stof(i));
  return res;
}

/*---------------------------------------------------------------------------*/
// Generalizes stoi to allow multiple ints to be derived from a string
std::vector<int> getints(const std::string& s)
{
  std::vector<int> res;
  std::string numstring = "(-)?\\d+"; // int regex
  std::vector<std::string> strs = Rex(s, numstring).get();
  for(auto i : strs)
    res.push_back(stoi(i));
  return res;
}

/*---------------------------------------------------------------------------*/
// converts an int to a pseudo - octal
int dec2oct(int x)
{
  int a = (x / 64);
  int b = (x - a * 64) / 8;
  int c = (x - a * 64 - 8 * b);
  std::string res = std::to_string(a);
  res += std::to_string(b);
  res += std::to_string(c);
  return stoi(res);
}

/*---------------------------------------------------------------------------*/
// converts an octal (as captured by stoi) to intended decimal value
int oct2dec(int x)
{
  int res = 0;
  std::string str = std::to_string(x);
  int l = str.length();
  if(l == 0) return res;
  for (int i = 0; i < l; i++)
  {
    int e = std::stoi(str.substr(i,1));
    if(e > 7) Rcpp::stop("Invalid octal");
    res += (e * pow(8, l - i - 1));
  }
  return res;
}

/*---------------------------------------------------------------------------*/
//Takes a string of bytes represented in ASCII and converts to actual bytes

std::vector<unsigned char> bytesFromArray(const std::string& s)
{
  if(s.empty())
    Rcpp::stop("Zero-length string passed to bytesFromArray");
  std::vector<int> tmpvec, res;
  std::vector<unsigned char> resvec;
  for(auto a : s)
  {
    if(a > 47 && a < 58)  tmpvec.push_back(a - 48); //Digits 0-9
    if(a > 64 && a < 71)  tmpvec.push_back(a - 55); //Uppercase A-F
    if(a > 96 && a < 103) tmpvec.push_back(a - 87); //Lowercase a-f
  }
  size_t ts = tmpvec.size();
  if(ts == 0)
    Rcpp::stop("arrayFromBytes not given a byte string");
  for(size_t i = 0; i < ts; i++)
    if(i % 2 == 0)
      tmpvec[i] = 16 * tmpvec[i];
  for(size_t i = 0; i < (ts - 1); i++)
    if(i % 2 == 0)
      res.push_back(tmpvec.at(i) + tmpvec.at(i + 1));
  if((ts - 1) % 2 == 0)
    res.push_back(tmpvec.at(ts - 1));
  for(auto i : res)
    resvec.push_back(i);
  return resvec;
}

/*---------------------------------------------------------------------------*/
// reinterprets string as vector of bytes
std::vector<uint8_t> stringtobytes(const std::string& s)
{
  std::vector<uint8_t> res;
  res.reserve(s.size());
  for(auto i : s)
    res.push_back(i);
  return res;
}

/*---------------------------------------------------------------------------*/
// reinterprets vector of bytes as a std::string
std::string bytestostring(const std::vector<uint8_t>& v)
{
  std::string res;
  res.reserve(v.size());
  for(auto i : v)
    res += i;
  return res;
}

/*---------------------------------------------------------------------------*/
// Matrix mulitplication on two 3 x 3 matrices
std::vector<float> matmul(std::vector<float> b, std::vector<float> a)
{
  if(a.size() != b.size())
    Rcpp::stop("Vectors must have same size.");
  if(a.size() != 9)
    Rcpp::stop("Vectors must have size 9.");
  std::vector<float> newmat;
  for(size_t i = 0; i < 9; i++) //clever use of indices to allow fill by loop
    newmat.push_back(a[i % 3 + 0] * b[3 * (i / 3) + 0] +
                     a[i % 3 + 3] * b[3 * (i / 3) + 1] +
                     a[i % 3 + 6] * b[3 * (i / 3) + 2] );
  return newmat;
}

/*---------------------------------------------------------------------------*/
// Converts a pdf style transformation sequence of 6 numbers to a 3x3 matrix
// simply by adding the "missing" invariant final column
std::vector<float> six2nine(std::vector<float> a)
{
  if(a.size() != 6)
    Rcpp::stop("Vector must have size 6.");
  std::vector<float> newmat {a[0], a[1], 0, a[2], a[3], 0, a[4], a[5], 1};
  return newmat;
}

/*---------------------------------------------------------------------------*/
// Allows a length-6 vector of number strings to be converted to 3x3 matrix
std::vector<float> stringvectomat(std::vector<std::string> b)
{
  if(b.size() != 6)
    Rcpp::stop("Vector must have size 6.");
  std::vector<float> a;
  for(auto i : b) a.push_back(std::stof(i));
  return six2nine(a);
}

/*---------------------------------------------------------------------------*/
// generalizes stof to vectors
std::vector<float> stringtofloat(std::vector<std::string> b)
{
  std::vector<float> r;
  for(auto i : b)
    r.push_back(std::stof(i));
  return r;
}

/*---------------------------------------------------------------------------*/
//Converts an int to the relevant 2-byte ASCII hex (4 characters long)
std::string intToHexstring(int i)
{
  std::string hex = "0123456789ABCDEF";
  std::string res;
  int firstnum = i / (16 * 16 * 16);
  i -= firstnum * (16 * 16 * 16);
  int secondnum = i / (16 * 16);
  i -= secondnum * (16 * 16);
  int thirdnum = i / 16;
  i -= thirdnum * 16;
  res += hex[firstnum];
  res += hex[secondnum];
  res += hex[thirdnum];
  res += hex[i];
  while(res.length() < 4)
    res = "0" + res;
  std::transform(res.begin(), res.end(), res.begin(),
                 std::ptr_fun<int, int>(std::toupper));
  return res;
}

/*---------------------------------------------------------------------------*/
//Split a string into length-4 elements
std::vector<std::string> splitfours(std::string s)
{
  std::vector<std::string> res;
  if(s.empty())
    return res;
  while(s.size() % 4 != 0)
    s += '0';
  for(unsigned i = 0; i < s.length()/4; i++)
    res.push_back(s.substr(i * 4, 4));
  return res;
}

/*--------------------------------------------------------------------------*/
//Split a string into length-2 elements
std::vector<std::string> splittwos(std::string s)
{
  std::vector<std::string> res;
  if(s.empty())
    return res;
  while(s.size() % 2 != 0)
    s += '0';
  for(unsigned i = 0; i < s.length()/2; i++)
    res.push_back(s.substr(i * 2, 2));
  return res;
}

/*--------------------------------------------------------------------------*/
//Converts an ASCII encoded string to a (char-based) string
std::string byteStringToString(const std::string& s)
{
  std::vector<std::string> sv = splitfours(s);
  std::vector<unsigned int> uv;
  std::string res;
  for(auto i : sv)
    uv.push_back((unsigned) stoul("0x" + i, nullptr, 0));
  for(auto i : uv)
  {
    if(i > 255)
      i = 255;
    res += (char) i;
  }
  return res;
}

/*--------------------------------------------------------------------------*/
// Extracts pdf object references from string
std::vector<int> getObjRefs(std::string ds)
{
  std::vector<int> res;
  for (auto i : Rex(ds, "\\d+ 0 R").get())
    res.push_back(stoi(splitter(i, " ")[0]));
  return res;
}

/*--------------------------------------------------------------------------*/
//test of whether string s contains a dictionary
bool isDictString(const std::string& s)
{
  return Rex(s, "<<").has();
}

/*---------------------------------------------------------------------------*/
//helper function to classify tokens in lexers
char symbol_type(const char c)
{
  if(c > 0x40 && c < 0x5b) return 'L'; //UPPERCASE
  if(c > 0x60 && c < 0x7b) return 'L'; //lowercase
  if(c > 0x2f && c < 0x3a) return 'D'; //digits
  if(c == ' ' || c == 0x0d || c == 0x0a ) return ' '; //whitespace
  return c;
}

/*---------------------------------------------------------------------------*/
// This probably should be in encodings rather than stringfunctions
std::string namesToChar(std::string s, const std::string& encoding)
{
  std::map<std::string, char> Rev;
  if(encoding == "/StandardEncoding")
  {
    Rev["/A"] = 0x41;
    Rev["/AE"] = 0xe1;
    Rev["/B"] = 0x42;
    Rev["/C"] = 0x43;
    Rev["/D"] = 0x44;
    Rev["/E"] = 0x45;
    Rev["/F"] = 0x46;
    Rev["/G"] = 0x47;
    Rev["/H"] = 0x48;
    Rev["/I"] = 0x49;
    Rev["/J"] = 0x4a;
    Rev["/K"] = 0x4b;
    Rev["/L"] = 0x4c;
    Rev["/Lslash"] = 0xe8;
    Rev["/M"] = 0x4d;
    Rev["/N"] = 0x4e;
    Rev["/O"] = 0x4f;
    Rev["/OE"] = 0xea;
    Rev["/Oslash"] = 0xe9;
    Rev["/P"] = 0x50;
    Rev["/Q"] = 0x51;
    Rev["/R"] = 0x52;
    Rev["/S"] = 0x53;
    Rev["/T"] = 0x54;
    Rev["/U"] = 0x55;
    Rev["/V"] = 0x56;
    Rev["/W"] = 0x57;
    Rev["/X"] = 0x58;
    Rev["/Y"] = 0x59;
    Rev["/Z"] = 0x5a;
    Rev["/a"] = 0x61;
    Rev["/acute"] = 0xc2;
    Rev["/ae"] = 0xf1;
    Rev["/ampersand"] = 0x26;
    Rev["/asciicircum"] = 0x5e;
    Rev["/asciitilde"] = 0x7e;
    Rev["/asterisk"] = 0x2a;
    Rev["/at"] = 0x40;
    Rev["/b"] = 0x62;
    Rev["/backslash"] = 0x5c;
    Rev["/bar"] = 0x7c;
    Rev["/braceleft"] = 0x7b;
    Rev["/braceright"] = 0x7d;
    Rev["/bracketleft"] = 0x5b;
    Rev["/bracketright"] = 0x5d;
    Rev["/breve"] = 0xc6;
    Rev["/bullet"] = 0xb7;
    Rev["/c"] = 0x63;
    Rev["/caron"] = 0xcf;
    Rev["/cedilla"] = 0xcb;
    Rev["/cent"] = 0xa2;
    Rev["/circumflex"] = 0xc3;
    Rev["/colon"] = 0x3a;
    Rev["/comma"] = 0x2c;
    Rev["/currency"] = 0xa8;
    Rev["/d"] = 0x64;
    Rev["/dagger"] = 0xb2;
    Rev["/daggerdbl"] = 0xb3;
    Rev["/dieresis"] = 0xc8;
    Rev["/dollar"] = 0x24;
    Rev["/dotaccent"] = 0xc7;
    Rev["/dotlessi"] = 0xf5;
    Rev["/e"] = 0x65;
    Rev["/eight"] = 0x38;
    Rev["/ellipsis"] = 0xbc;
    Rev["/emdash"] = 0xd0;
    Rev["/endash"] = 0xb1;
    Rev["/equal"] = 0x3d;
    Rev["/exclam"] = 0x21;
    Rev["/exclamdown"] = 0xa1;
    Rev["/f"] = 0x66;
    Rev["/fi"] = 0xae;
    Rev["/five"] = 0x35;
    Rev["/fl"] = 0xaf;
    Rev["/florin"] = 0xa6;
    Rev["/four"] = 0x34;
    Rev["/fraction"] = 0xa4;
    Rev["/g"] = 0x67;
    Rev["/germandbls"] = 0xfb;
    Rev["/grave"] = 0xc1;
    Rev["/greater"] = 0x3e;
    Rev["/guillemotleft"] = 0xab;
    Rev["/guillemotright"] = 0xbb;
    Rev["/guilsinglleft"] = 0xac;
    Rev["/guilsinglright"] = 0xad;
    Rev["/h"] = 0x68;
    Rev["/hungarumlaut"] = 0xcd;
    Rev["/hyphen"] = 0x2d;
    Rev["/i"] = 0x69;
    Rev["/j"] = 0x6a;
    Rev["/k"] = 0x6b;
    Rev["/l"] = 0x6c;
    Rev["/less"] = 0x3c;
    Rev["/lslash"] = 0xf8;
    Rev["/m"] = 0x6d;
    Rev["/macron"] = 0xc5;
    Rev["/n"] = 0x6e;
    Rev["/nine"] = 0x39;
    Rev["/numbersign"] = 0x23;
    Rev["/o"] = 0x6f;
    Rev["/oe"] = 0xfa;
    Rev["/ogonek"] = 0xce;
    Rev["/one"] = 0x31;
    Rev["/ordfeminine"] = 0xe3;
    Rev["/ordmasculine"] = 0xeb;
    Rev["/oslash"] = 0xf9;
    Rev["/p"] = 0x70;
    Rev["/paragraph"] = 0xb6;
    Rev["/parenleft"] = 0x28;
    Rev["/parenright"] = 0x29;
    Rev["/percent"] = 0x25;
    Rev["/period"] = 0x2e;
    Rev["/periodcentered"] = 0xb4;
    Rev["/perthousand"] = 0xbd;
    Rev["/plus"] = 0x2b;
    Rev["/q"] = 0x71;
    Rev["/question"] = 0x3f;
    Rev["/questiondown"] = 0xbf;
    Rev["/quotedbl"] = 0x22;
    Rev["/quotedblbase"] = 0xb9;
    Rev["/quotedblleft"] = 0xaa;
    Rev["/quotedblright"] = 0xba;
    Rev["/quoteleft"] = 0x60;
    Rev["/quoteright"] = 0x27;
    Rev["/quotesinglbase"] = 0xb8;
    Rev["/quotesingle"] = 0xa9;
    Rev["/r"] = 0x72;
    Rev["/ring"] = 0xca;
    Rev["/s"] = 0x73;
    Rev["/section"] = 0xa7;
    Rev["/semicolon"] = 0x3b;
    Rev["/seven"] = 0x37;
    Rev["/six"] = 0x36;
    Rev["/slash"] = 0x2f;
    Rev["/space"] = 0x20;
    Rev["/sterling"] = 0xa3;
    Rev["/t"] = 0x74;
    Rev["/three"] = 0x33;
    Rev["/tilde"] = 0xc4;
    Rev["/two"] = 0x32;
    Rev["/u"] = 0x75;
    Rev["/underscore"] = 0x5f;
    Rev["/v"] = 0x76;
    Rev["/w"] = 0x77;
    Rev["/x"] = 0x78;
    Rev["/y"] = 0x79;
    Rev["/yen"] = 0xa5;
    Rev["/z"] = 0x7a;
    Rev["/zero"] = 0x30;
  }

  if(encoding == "/MacRomanEncoding")
  {
    Rev["/A"] = 0x41;
    Rev["/AE"] = 0xae;
    Rev["/Aacute"] = 0xe7;
    Rev["/Acircumflex"] = 0xe5;
    Rev["/Adieresis"] = 0x80;
    Rev["/Agrave"] = 0xcb;
    Rev["/Aring"] = 0x41;
    Rev["/Atilde"] = 0xcc;
    Rev["/B"] = 0x42;
    Rev["/C"] = 0x43;
    Rev["/Ccedilla"] = 0x82;
    Rev["/D"] = 0x44;
    Rev["/E"] = 0x45;
    Rev["/Eacute"] = 0x83;
    Rev["/Ecircumflex"] = 0xe6;
    Rev["/Edieresis"] = 0xe8;
    Rev["/Egrave"] = 0xe9;
    Rev["/F"] = 0x46;
    Rev["/G"] = 0x47;
    Rev["/H"] = 0x48;
    Rev["/I"] = 0x49;
    Rev["/Iacute"] = 0xea;
    Rev["/Icircumflex"] = 0xeb;
    Rev["/Idieresis"] = 0xec;
    Rev["/Igrave"] = 0xed;
    Rev["/J"] = 0x4a;
    Rev["/K"] = 0x4b;
    Rev["/L"] = 0x4c;
    Rev["/M"] = 0x4d;
    Rev["/N"] = 0x4e;
    Rev["/Ntilde"] = 0x84;
    Rev["/O"] = 0x4f;
    Rev["/OE"] = 0xce;
    Rev["/Oacute"] = 0xee;
    Rev["/Ocircumflex"] = 0xef;
    Rev["/Odieresis"] = 0x85;
    Rev["/Ograve"] = 0xf1;
    Rev["/Oslash"] = 0xaf;
    Rev["/Otilde"] = 0xcd;
    Rev["/P"] = 0x50;
    Rev["/Q"] = 0x51;
    Rev["/R"] = 0x52;
    Rev["/S"] = 0x53;
    Rev["/T"] = 0x54;
    Rev["/U"] = 0x55;
    Rev["/Uacute"] = 0xf2;
    Rev["/Ucircumflex"] = 0xf3;
    Rev["/Udieresis"] = 0x86;
    Rev["/Ugrave"] = 0xf4;
    Rev["/V"] = 0x56;
    Rev["/W"] = 0x57;
    Rev["/X"] = 0x58;
    Rev["/Y"] = 0x59;
    Rev["/Ydieresis"] = 0xd9;
    Rev["/Z"] = 0x5a;
    Rev["/a"] = 0x61;
    Rev["/aacute"] = 0x87;
    Rev["/acircumflex"] = 0x89;
    Rev["/acute"] = 0xab;
    Rev["/adieresis"] = 0x8a;
    Rev["/ae"] = 0xbe;
    Rev["/agrave"] = 0x88;
    Rev["/ampersand"] = 0x26;
    Rev["/aring"] = 0x8c;
    Rev["/asciicircum"] = 0x5e;
    Rev["/asciitilde"] = 0x7e;
    Rev["/asterisk"] = 0x2a;
    Rev["/at"] = 0x40;
    Rev["/atilde"] = 0x8b;
    Rev["/b"] = 0x62;
    Rev["/backslash"] = 0x5c;
    Rev["/bar"] = 0x5c;
    Rev["/braceleft"] = 0x7c;
    Rev["/braceright"] = 0x7b;
    Rev["/bracketleft"] = 0x7d;
    Rev["/bracketright"] = 0x5b;
    Rev["/breve"] = 0x5d;
    Rev["/bullet"] = 0xf9;
    Rev["/.notdef"] = 0xa5;
    Rev["/c"] = 0xa5;
    Rev["/caron"] = 0x63;
    Rev["/ccedilla"] = 0xff;
    Rev["/cedilla"] = 0x63;
    Rev["/cent"] = 0xfc;
    Rev["/circumflex"] = 0xa2;
    Rev["/colon"] = 0xf6;
    Rev["/comma"] = 0x3a;
    Rev["/copyright"] = 0x2c;
    Rev["/currency"] = 0xa9;
    Rev["/d"] = 0xdb;
    Rev["/dagger"] = 0x64;
    Rev["/daggerdbl"] = 0x2a;
    Rev["/degree"] = 0x2a;
    Rev["/dieresis"] = 0xe0;
    Rev["/divide"] = 0xa1;
    Rev["/dollar"] = 0xac;
    Rev["/dotaccent"] = 0xd6;
    Rev["/dotlessi"] = 0x24;
    Rev["/e"] = 0xfa;
    Rev["/eacute"] = 0xf5;
    Rev["/ecircumflex"] = 0x65;
    Rev["/edieresis"] = 0x8e;
    Rev["/egrave"] = 0x65;
    Rev["/eight"] = 0x91;
    Rev["/ellipsis"] = 0x65;
    Rev["/emdash"] = 0x38;
    Rev["/endash"] = 0xc9;
    Rev["/equal"] = 0xd1;
    Rev["/exclam"] = 0xd0;
    Rev["/exclamdown"] = 0x3d;
    Rev["/f"] = 0x21;
    Rev["/fi"] = 0xc1;
    Rev["/five"] = 0x66;
    Rev["/fl"] = 0xde;
    Rev["/florin"] = 0x35;
    Rev["/four"] = 0xdf;
    Rev["/fraction"] = 0xc4;
    Rev["/g"] = 0x34;
    Rev["/germandbls"] = 0xda;
    Rev["/grave"] = 0x67;
    Rev["/greater"] = 0xa7;
    Rev["/guillemotleft"] = 0x60;
    Rev["/guillemotright"] = 0x3e;
    Rev["/guilsinglleft"] = 0xc7;
    Rev["/guilsinglright"] = 0xc8;
    Rev["/h"] = 0xdc;
    Rev["/hungarumlaut"] = 0xdd;
    Rev["/hyphen"] = 0x68;
    Rev["/i"] = 0xfd;
    Rev["/iacute"] = 0x2d;
    Rev["/icircumflex"] = 0x69;
    Rev["/idieresis"] = 0x92;
    Rev["/igrave"] = 0x94;
    Rev["/j"] = 0x95;
    Rev["/k"] = 0x93;
    Rev["/l"] = 0x6a;
    Rev["/less"] = 0x6b;
    Rev["/logicalnot"] = 0x6c;
    Rev["/m"] = 0x3c;
    Rev["/macron"] = 0xc2;
    Rev["/mu"] = 0x6d;
    Rev["/n"] = 0xf8;
    Rev["/nine"] = 0xb5;
    Rev["/ntilde"] = 0x6e;
    Rev["/numbersign"] = 0x39;
    Rev["/o"] = 0x96;
    Rev["/oacute"] = 0x23;
    Rev["/ocircumflex"] = 0x6f;
    Rev["/odieresis"] = 0x97;
    Rev["/oe"] = 0x99;
    Rev["/ogonek"] = 0x9a;
    Rev["/ograve"] = 0xcf;
    Rev["/one"] = 0xfe;
    Rev["/ordfeminine"] = 0x98;
    Rev["/ordmasculine"] = 0x31;
    Rev["/oslash"] = 0xbb;
    Rev["/otilde"] = 0xbc;
    Rev["/p"] = 0xbf;
    Rev["/paragraph"] = 0x9b;
    Rev["/parenleft"] = 0x70;
    Rev["/parenright"] = 0xa6;
    Rev["/percent"] = 0x28;
    Rev["/period"] = 0x29;
    Rev["/periodcentered"] = 0x25;
    Rev["/perthousand"] = 0x2e;
    Rev["/plus"] = 0xe1;
    Rev["/plusminus"] = 0xe4;
    Rev["/q"] = 0x2b;
    Rev["/question"] = 0xb1;
    Rev["/questiondown"] = 0x71;
    Rev["/quotedbl"] = 0x3f;
    Rev["/quotedblbase"] = 0xc0;
    Rev["/quotedblleft"] = 0x5c;
    Rev["/quotedblright"] = 0x22;
    Rev["/quoteleft"] = 0xe3;
    Rev["/quoteright"] = 0xd2;
    Rev["/quotesinglbase"] = 0xd3;
    Rev["/quotesingle"] = 0xd4;
    Rev["/r"] = 0xd5;
    Rev["/registered"] = 0xe2;
    Rev["/ring"] = 0x72;
    Rev["/s"] = 0xa8;
    Rev["/section"] = 0xfb;
    Rev["/semicolon"] = 0x73;
    Rev["/seven"] = 0xa4;
    Rev["/six"] = 0x3b;
    Rev["/slash"] = 0x37;
    Rev["/space"] = 0x36;
    Rev["/sterling"] = 0x2f;
    Rev["/t"] = 0x20;
    Rev["/three"] = 0xa3;
    Rev["/tilde"] = 0x74;
    Rev["/trademark"] = 0x33;
    Rev["/two"] = 0xf7;
    Rev["/u"] = 0xaa;
    Rev["/uacute"] = 0x32;
    Rev["/ucircumflex"] = 0x75;
    Rev["/udieresis"] = 0x9c;
    Rev["/ugrave"] = 0x9e;
    Rev["/underscore"] = 0x9f;
    Rev["/v"] = 0x75;
    Rev["/w"] = 0x5f;
    Rev["/x"] = 0x76;
    Rev["/y"] = 0x77;
    Rev["/ydieresis"] = 0x78;
    Rev["/yen"] = 0x79;
    Rev["/z"] = 0xd8;
    Rev["/zero"] = 0xb4;
    Rev["/A"] = 0x7a;
    Rev["/AE"] = 0x30;
  }

  if(encoding == "/WinAnsiEncoding")
  {
    Rev["/A"] = 0x41;
    Rev["/AE"] = 0xc6;
    Rev["/Aacute"] = 0xc1;
    Rev["/Acircumflex"] = 0xc2;
    Rev["/Adieresis"] = 0xc4;
    Rev["/Agrave"] = 0xc0;
    Rev["/Aring"] = 0xc5;
    Rev["/Atilde"] = 0xc3;
    Rev["/B"] = 0x42;
    Rev["/C"] = 0x43;
    Rev["/Ccedilla"] = 0xc7;
    Rev["/D"] = 0x44;
    Rev["/E"] = 0x45;
    Rev["/Eacute"] = 0xc9;
    Rev["/Ecircumflex"] = 0xca;
    Rev["/Edieresis"] = 0xcb;
    Rev["/Egrave"] = 0xc8;
    Rev["/Eth"] = 0xd0;
    Rev["/Euro"] = 0x80;
    Rev["/F"] = 0x46;
    Rev["/G"] = 0x47;
    Rev["/H"] = 0x48;
    Rev["/I"] = 0x49;
    Rev["/Iacute"] = 0xcd;
    Rev["/Icircumflex"] = 0xce;
    Rev["/Idieresis"] = 0xcf;
    Rev["/Igrave"] = 0xcc;
    Rev["/J"] = 0x4a;
    Rev["/K"] = 0x4b;
    Rev["/L"] = 0x4c;
    Rev["/M"] = 0x4d;
    Rev["/N"] = 0x4e;
    Rev["/Ntilde"] = 0xd1;
    Rev["/O"] = 0x4f;
    Rev["/OE"] = 0x8c;
    Rev["/Oacute"] = 0xd3;
    Rev["/Ocircumflex"] = 0xd4;
    Rev["/Odieresis"] = 0xd6;
    Rev["/Ograve"] = 0xd2;
    Rev["/Oslash"] = 0xd8;
    Rev["/Otilde"] = 0xd5;
    Rev["/P"] = 0x50;
    Rev["/Q"] = 0x51;
    Rev["/R"] = 0x52;
    Rev["/S"] = 0x53;
    Rev["/Scaron"] = 0x8a;
    Rev["/T"] = 0x54;
    Rev["/Thorn"] = 0xde;
    Rev["/U"] = 0x55;
    Rev["/Uacute"] = 0xda;
    Rev["/Ucircumflex"] = 0xdb;
    Rev["/Udieresis"] = 0xdc;
    Rev["/Ugrave"] = 0xd9;
    Rev["/V"] = 0x56;
    Rev["/W"] = 0x57;
    Rev["/X"] = 0x58;
    Rev["/Y"] = 0x59;
    Rev["/Yacute"] = 0xdd;
    Rev["/Ydieresis"] = 0x9f;
    Rev["/Z"] = 0x5a;
    Rev["/Zcaron"] = 0x8e;
    Rev["/a"] = 0x61;
    Rev["/aacute"] = 0xe1;
    Rev["/acircumflex"] = 0xe2;
    Rev["/acute"] = 0xb4;
    Rev["/adieresis"] = 0xe4;
    Rev["/ae"] = 0xe6;
    Rev["/agrave"] = 0xe0;
    Rev["/ampersand"] = 0x26;
    Rev["/aring"] = 0xe5;
    Rev["/asciicircum"] = 0x5e;
    Rev["/asciitilde"] = 0x7e;
    Rev["/asterisk"] = 0x2a;
    Rev["/at"] = 0x40;
    Rev["/atilde"] = 0xe3;
    Rev["/b"] = 0x62;
    Rev["/backslash"] = 0x5c;
    Rev["/bar"] = 0x7c;
    Rev["/braceleft"] = 0x7b;
    Rev["/braceright"] = 0x7d;
    Rev["/bracketleft"] = 0x5b;
    Rev["/bracketright"] = 0x5d;
    Rev["/brokenbar"] = 0xa6;
    Rev["/bullet"] = 0x95;
    Rev["/c"] = 0x63;
    Rev["/ccedilla"] = 0xe7;
    Rev["/cedilla"] = 0xb8;
    Rev["/cent"] = 0xa2;
    Rev["/circumflex"] = 0x88;
    Rev["/colon"] = 0x3a;
    Rev["/comma"] = 0x2c;
    Rev["/copyright"] = 0xa9;
    Rev["/currency"] = 0xa4;
    Rev["/d"] = 0x64;
    Rev["/dagger"] = 0x86;
    Rev["/daggerdbl"] = 0x87;
    Rev["/degree"] = 0xb0;
    Rev["/dieresis"] = 0xa8;
    Rev["/divide"] = 0xf7;
    Rev["/dollar"] = 0x24;
    Rev["/e"] = 0x65;
    Rev["/eacute"] = 0xe9;
    Rev["/ecircumflex"] = 0xea;
    Rev["/edieresis"] = 0xeb;
    Rev["/egrave"] = 0xe8;
    Rev["/eight"] = 0x38;
    Rev["/ellipsis"] = 0x85;
    Rev["/emdash"] = 0x97;
    Rev["/endash"] = 0x96;
    Rev["/equal"] = 0x3d;
    Rev["/eth"] = 0xf0;
    Rev["/exclam"] = 0x21;
    Rev["/exclamdown"] = 0xa1;
    Rev["/f"] = 0x66;
    Rev["/five"] = 0x35;
    Rev["/florin"] = 0x83;
    Rev["/four"] = 0x34;
    Rev["/g"] = 0x67;
    Rev["/germandbls"] = 0xdf;
    Rev["/grave"] = 0x60;
    Rev["/greater"] = 0x3e;
    Rev["/guillemotleft"] = 0xab;
    Rev["/guillemotright"] = 0xbb;
    Rev["/guilsinglleft"] = 0x8b;
    Rev["/guilsinglright"] = 0x9b;
    Rev["/h"] = 0x68;
    Rev["/hyphen"] = 0x2d;
    Rev["/i"] = 0x69;
    Rev["/iacute"] = 0xed;
    Rev["/icircumflex"] = 0xee;
    Rev["/idieresis"] = 0xef;
    Rev["/igrave"] = 0xec;
    Rev["/j"] = 0x6a;
    Rev["/k"] = 0x6b;
    Rev["/l"] = 0x6c;
    Rev["/less"] = 0x3c;
    Rev["/logicalnot"] = 0xac;
    Rev["/m"] = 0x6d;
    Rev["/macron"] = 0xaf;
    Rev["/mu"] = 0xb5;
    Rev["/multiply"] = 0xd7;
    Rev["/n"] = 0x6e;
    Rev["/nine"] = 0x39;
    Rev["/ntilde"] = 0xf1;
    Rev["/numbersign"] = 0x23;
    Rev["/o"] = 0x6f;
    Rev["/oacute"] = 0xf3;
    Rev["/ocircumflex"] = 0xf4;
    Rev["/odieresis"] = 0xf6;
    Rev["/oe"] = 0x9c;
    Rev["/ograve"] = 0xf2;
    Rev["/one"] = 0x31;
    Rev["/onehalf"] = 0xbd;
    Rev["/onequarter"] = 0xbc;
    Rev["/onesuperior"] = 0xb9;
    Rev["/ordfeminine"] = 0xaa;
    Rev["/ordmasculine"] = 0xba;
    Rev["/oslash"] = 0xf8;
    Rev["/otilde"] = 0xf5;
    Rev["/p"] = 0x70;
    Rev["/paragraph"] = 0xb6;
    Rev["/parenleft"] = 0x28;
    Rev["/parenright"] = 0x29;
    Rev["/percent"] = 0x25;
    Rev["/period"] = 0x2e;
    Rev["/periodcentered"] = 0xb7;
    Rev["/perthousand"] = 0x89;
    Rev["/plus"] = 0x2b;
    Rev["/plusminus"] = 0xb1;
    Rev["/q"] = 0x71;
    Rev["/question"] = 0x3f;
    Rev["/questiondown"] = 0xbf;
    Rev["/quotedbl"] = 0x22;
    Rev["/quotedblbase"] = 0x84;
    Rev["/quotedblleft"] = 0x93;
    Rev["/quotedblright"] = 0x94;
    Rev["/quoteleft"] = 0x91;
    Rev["/quoteright"] = 0x92;
    Rev["/quotesinglbase"] = 0x82;
    Rev["/quotesingle"] = 0x27;
    Rev["/r"] = 0x72;
    Rev["/registered"] = 0xae;
    Rev["/s"] = 0x73;
    Rev["/scaron"] = 0x9a;
    Rev["/section"] = 0xa7;
    Rev["/semicolon"] = 0x3b;
    Rev["/seven"] = 0x37;
    Rev["/six"] = 0x36;
    Rev["/slash"] = 0x2f;
    Rev["/space"] = 0x20;
    Rev["/sterling"] = 0xa3;
    Rev["/t"] = 0x74;
    Rev["/thorn"] = 0xfe;
    Rev["/three"] = 0x33;
    Rev["/threequarters"] = 0xbe;
    Rev["/threesuperior"] = 0xb3;
    Rev["/tilde"] = 0x98;
    Rev["/trademark"] = 0x99;
    Rev["/two"] = 0x32;
    Rev["/twosuperior"] = 0xb2;
    Rev["/u"] = 0x75;
    Rev["/uacute"] = 0xfa;
    Rev["/ucircumflex"] = 0xfb;
    Rev["/udieresis"] = 0xfc;
    Rev["/ugrave"] = 0xf9;
    Rev["/underscore"] = 0x5f;
    Rev["/v"] = 0x76;
    Rev["/w"] = 0x77;
    Rev["/x"] = 0x78;
    Rev["/y"] = 0x79;
    Rev["/yacute"] = 0xfd;
    Rev["/ydieresis"] = 0xff;
    Rev["/yen"] = 0xa5;
    Rev["/z"] = 0x7a;
    Rev["/zcaron"] = 0x9e;
    Rev["/zero"] = 0x30;
  }

  if(encoding == "/PDFDocEncoding")
  {
    Rev["/A"] = 0x41;
    Rev["/AE"] = 0xc6;
    Rev["/Aacute"] = 0xc1;
    Rev["/Acircumflex"] = 0xc2;
    Rev["/Adieresis"] = 0xc4;
    Rev["/Agrave"] = 0xc0;
    Rev["/Aring"] = 0xc5;
    Rev["/Atilde"] = 0xc3;
    Rev["/B"] = 0x42;
    Rev["/C"] = 0x43;
    Rev["/Ccedilla"] = 0xc7;
    Rev["/D"] = 0x44;
    Rev["/E"] = 0x45;
    Rev["/Eacute"] = 0xc9;
    Rev["/Ecircumflex"] = 0xca;
    Rev["/Edieresis"] = 0xcb;
    Rev["/Egrave"] = 0xc8;
    Rev["/Eth"] = 0xd0;
    Rev["/Euro"] = 0xa0;
    Rev["/F"] = 0x46;
    Rev["/G"] = 0x47;
    Rev["/H"] = 0x48;
    Rev["/I"] = 0x49;
    Rev["/Iacute"] = 0xcd;
    Rev["/Icircumflex"] = 0xce;
    Rev["/Idieresis"] = 0xcf;
    Rev["/Igrave"] = 0xcc;
    Rev["/J"] = 0x4a;
    Rev["/K"] = 0x4b;
    Rev["/L"] = 0x4c;
    Rev["/Lslash"] = 0x95;
    Rev["/M"] = 0x4d;
    Rev["/N"] = 0x4e;
    Rev["/Ntilde"] = 0xd1;
    Rev["/O"] = 0x4f;
    Rev["/OE"] = 0x96;
    Rev["/Oacute"] = 0xd3;
    Rev["/Ocircumflex"] = 0xd4;
    Rev["/Odieresis"] = 0xd6;
    Rev["/Ograve"] = 0xd2;
    Rev["/Oslash"] = 0xd8;
    Rev["/Otilde"] = 0xd5;
    Rev["/P"] = 0x50;
    Rev["/Q"] = 0x51;
    Rev["/R"] = 0x52;
    Rev["/S"] = 0x53;
    Rev["/Scaron"] = 0x97;
    Rev["/T"] = 0x54;
    Rev["/Thorn"] = 0xde;
    Rev["/U"] = 0x55;
    Rev["/Uacute"] = 0xda;
    Rev["/Ucircumflex"] = 0xdb;
    Rev["/Udieresis"] = 0xdc;
    Rev["/Ugrave"] = 0xd9;
    Rev["/V"] = 0x56;
    Rev["/W"] = 0x57;
    Rev["/X"] = 0x58;
    Rev["/Y"] = 0x59;
    Rev["/Yacute"] = 0xdd;
    Rev["/Ydieresis"] = 0x98;
    Rev["/Z"] = 0x5a;
    Rev["/Zcaron"] = 0x99;
    Rev["/a"] = 0x61;
    Rev["/aacute"] = 0xe1;
    Rev["/acircumflex"] = 0xe2;
    Rev["/acute"] = 0xb4;
    Rev["/adieresis"] = 0xe4;
    Rev["/ae"] = 0xe6;
    Rev["/agrave"] = 0xe0;
    Rev["/ampersand"] = 0x26;
    Rev["/aring"] = 0xe5;
    Rev["/asciicircum"] = 0x5e;
    Rev["/asciitilde"] = 0x7e;
    Rev["/asterisk"] = 0x2a;
    Rev["/at"] = 0x40;
    Rev["/atilde"] = 0xe3;
    Rev["/b"] = 0x62;
    Rev["/backslash"] = 0x5c;
    Rev["/bar"] = 0x7c;
    Rev["/braceleft"] = 0x7b;
    Rev["/braceright"] = 0x7d;
    Rev["/bracketleft"] = 0x5b;
    Rev["/bracketright"] = 0x5d;
    Rev["/breve"] = 0x18;
    Rev["/brokenbar"] = 0xa6;
    Rev["/bullet"] = 0x80;
    Rev["/c"] = 0x63;
    Rev["/caron"] = 0x19;
    Rev["/ccedilla"] = 0xe7;
    Rev["/cedilla"] = 0xb8;
    Rev["/cent"] = 0xa2;
    Rev["/circumflex"] = 0x1a;
    Rev["/colon"] = 0x3a;
    Rev["/comma"] = 0x2c;
    Rev["/copyright"] = 0xa9;
    Rev["/currency"] = 0xa4;
    Rev["/d"] = 0x64;
    Rev["/dagger"] = 0x81;
    Rev["/daggerdbl"] = 0x82;
    Rev["/degree"] = 0xb0;
    Rev["/dieresis"] = 0xa8;
    Rev["/divide"] = 0xf7;
    Rev["/dollar"] = 0x24;
    Rev["/dotaccent"] = 0x1b;
    Rev["/dotlessi"] = 0x9a;
    Rev["/e"] = 0x65;
    Rev["/eacute"] = 0xe9;
    Rev["/ecircumflex"] = 0xea;
    Rev["/edieresis"] = 0xeb;
    Rev["/egrave"] = 0xe8;
    Rev["/eight"] = 0x38;
    Rev["/ellipsis"] = 0x83;
    Rev["/emdash"] = 0x84;
    Rev["/endash"] = 0x85;
    Rev["/equal"] = 0x3d;
    Rev["/eth"] = 0xf0;
    Rev["/exclam"] = 0x21;
    Rev["/exclamdown"] = 0xa1;
    Rev["/f"] = 0x66;
    Rev["/fi"] = 0x93;
    Rev["/five"] = 0x35;
    Rev["/fl"] = 0x94;
    Rev["/florin"] = 0x86;
    Rev["/four"] = 0x34;
    Rev["/fraction"] = 0x87;
    Rev["/g"] = 0x67;
    Rev["/germandbls"] = 0xdf;
    Rev["/grave"] = 0x60;
    Rev["/greater"] = 0x3e;
    Rev["/guillemotleft"] = 0xab;
    Rev["/guillemotright"] = 0xbb;
    Rev["/guilsinglleft"] = 0x88;
    Rev["/guilsinglright"] = 0x89;
    Rev["/h"] = 0x68;
    Rev["/hungarumlaut"] = 0x1c;
    Rev["/hyphen"] = 0x2d;
    Rev["/i"] = 0x69;
    Rev["/iacute"] = 0xed;
    Rev["/icircumflex"] = 0xee;
    Rev["/idieresis"] = 0xef;
    Rev["/igrave"] = 0xec;
    Rev["/j"] = 0x6a;
    Rev["/k"] = 0x6b;
    Rev["/l"] = 0x6c;
    Rev["/less"] = 0x3c;
    Rev["/logicalnot"] = 0xac;
    Rev["/lslash"] = 0x9b;
    Rev["/m"] = 0x6d;
    Rev["/macron"] = 0xaf;
    Rev["/minus"] = 0x8a;
    Rev["/mu"] = 0xb5;
    Rev["/multiply"] = 0xd7;
    Rev["/n"] = 0x6e;
    Rev["/nine"] = 0x39;
    Rev["/ntilde"] = 0xf1;
    Rev["/numbersign"] = 0x23;
    Rev["/o"] = 0x6f;
    Rev["/oacute"] = 0xf3;
    Rev["/ocircumflex"] = 0xf4;
    Rev["/odieresis"] = 0xf6;
    Rev["/oe"] = 0x9c;
    Rev["/ogonek"] = 0x1d;
    Rev["/ograve"] = 0xf2;
    Rev["/one"] = 0x31;
    Rev["/onehalf"] = 0xbd;
    Rev["/onequarter"] = 0xbc;
    Rev["/onesuperior"] = 0xb9;
    Rev["/ordfeminine"] = 0xaa;
    Rev["/ordmasculine"] = 0xba;
    Rev["/oslash"] = 0xf8;
    Rev["/otilde"] = 0xf5;
    Rev["/p"] = 0x70;
    Rev["/paragraph"] = 0xb6;
    Rev["/parenleft"] = 0x28;
    Rev["/parenright"] = 0x29;
    Rev["/percent"] = 0x25;
    Rev["/period"] = 0x2e;
    Rev["/periodcentered"] = 0xb7;
    Rev["/perthousand"] = 0x8b;
    Rev["/plus"] = 0x2b;
    Rev["/plusminus"] = 0xb1;
    Rev["/q"] = 0x71;
    Rev["/question"] = 0x3f;
    Rev["/questiondown"] = 0xbf;
    Rev["/quotedbl"] = 0x22;
    Rev["/quotedblbase"] = 0x8c;
    Rev["/quotedblleft"] = 0x8d;
    Rev["/quotedblright"] = 0x8e;
    Rev["/quoteleft"] = 0x8f;
    Rev["/quoteright"] = 0x90;
    Rev["/quotesinglbase"] = 0x91;
    Rev["/quotesingle"] = 0x27;
    Rev["/r"] = 0x72;
    Rev["/registered"] = 0xae;
    Rev["/ring"] = 0x1e;
    Rev["/s"] = 0x73;
    Rev["/scaron"] = 0x9d;
    Rev["/section"] = 0xa7;
    Rev["/semicolon"] = 0x3b;
    Rev["/seven"] = 0x37;
    Rev["/six"] = 0x36;
    Rev["/slash"] = 0x2f;
    Rev["/space"] = 0x20;
    Rev["/sterling"] = 0xa3;
    Rev["/t"] = 0x74;
    Rev["/thorn"] = 0xfe;
    Rev["/three"] = 0x33;
    Rev["/threequarters"] = 0xbe;
    Rev["/threesuperior"] = 0xb3;
    Rev["/tilde"] = 0x1f;
    Rev["/trademark"] = 0x92;
    Rev["/two"] = 0x32;
    Rev["/twosuperior"] = 0xb2;
    Rev["/u"] = 0x75;
    Rev["/uacute"] = 0xfa;
    Rev["/ucircumflex"] = 0xfb;
    Rev["/udieresis"] = 0xfc;
    Rev["/ugrave"] = 0xf9;
    Rev["/underscore"] = 0x5f;
    Rev["/v"] = 0x76;
    Rev["/w"] = 0x77;
    Rev["/x"] = 0x78;
    Rev["/y"] = 0x79;
    Rev["/yacute"] = 0xfd;
    Rev["/ydieresis"] = 0xff;
    Rev["/yen"] = 0xa5;
    Rev["/z"] = 0x7a;
    Rev["/zcaron"] = 0x9e;
    Rev["/zero"] = 0x30;
  }

  std::string res;
  if(Rev.find(s) != Rev.end()) res += (char) Rev[s];
  if(s == "/fi") return "fi";
  if(s == "/fl") return "fl";
  return res;
}

/*--------------------------------------------------------------------------*/
// Removes whitespace from right of a string
void trimRight(std::string& s)
{
  if(s.length() == 0) return;
  for(int i = s.length() - 1; i >= 0; i--)
    if(s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')
      s.resize(i);
    else
      break;
}
