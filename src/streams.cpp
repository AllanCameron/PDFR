#include "pdfr.h"
#include "streams.h"
#include "document.h"
#include "Rex.h"
#include "stringfunctions.h"
#include "miniz.h"
#include "debugtools.h"


/*---------------------------------------------------------------------------*/

bool isFlateDecode(const std::string& filestring, int startpos)
{
  dictionary dict = dictionary(filestring, startpos);
  return Rex(dict.get("/Filter"), "/FlateDecode").has();
}

/*---------------------------------------------------------------------------*/

std::string FlateDecode(const std::string& s)
{
  z_stream stream ;
  int factor = 20;

  while(true)
  {
    char * out = new char[s.length()*factor];
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = s.length();
    stream.next_in = (Bytef*)s.c_str();
    stream.avail_out = s.length()*factor;
    stream.next_out = (Bytef*)out;
    inflateInit(&stream);
    inflate(&stream, Z_FINISH);
    inflateEnd(&stream)       ;

    if(stream.total_out >= factor*s.length())
    {
      delete[] out;
      factor *= 2;
      continue;
    }

    std::string result;
    for(unsigned long i = 0; i < stream.total_out; i++) result += out[i];
    delete[] out;
    return result;
  }
}


/*---------------------------------------------------------------------------*/

std::string
getStreamContents(document& d, const std::string& filestring, int objstart)
{
  dictionary dict = dictionary(filestring, objstart);
  if(dict.has("stream")) if(dict.has("/Length"))
  {
    int streamlen;
    if(dict.hasRefs("/Length"))
    {
      int lengthob = dict.getRefs("/Length")[0];
      streamlen = std::stoi(d.getobject(lengthob).getStream());
    }
    else
    {
      streamlen = std::stoi(dict.get("/Length"));
    }
    int streamstart = std::stoi(dict.get("stream"));
    return filestring.substr(streamstart, streamlen);
  }
  return "";
}

/*---------------------------------------------------------------------------*/

bool objHasStream(const std::string& filestring, int objectstart)
{
  dictionary dict = dictionary(filestring, objectstart);
  return dict.has("stream");
}

/*---------------------------------------------------------------------------*/

bool isObject(const std::string& filestring, int objectstart)
{
  std::string ts(filestring.begin() + objectstart,
                 filestring.begin() + objectstart + 20);
  return  Rex(ts, "\\d+ \\d+ obj").has();
}

/*---------------------------------------------------------------------------*/

std::string objectPreStream(const std::string& filestring, int objectstart)
{
  std::vector<int> streamstart, nextobjstart, mn;
  std::string dic;
  int nchars = 0;
  while(mn.size() == 0 && nchars < 3000)
  {
    nchars += 1500;
    dic.assign(filestring.begin() + objectstart,
               filestring.begin() + objectstart + nchars);
    streamstart = Rex(dic, "stream").pos();
    nextobjstart = Rex(dic, "endobj").pos();
    if(!streamstart.empty()) mn.push_back(streamstart[0]);
    if(!nextobjstart.empty()) mn.push_back(nextobjstart[0]);
  }
  if(mn.size()>0) return dic.substr(0, *std::min_element(mn.begin(), mn.end()));

  Rcpp::stop("No object found");
}

/*---------------------------------------------------------------------------*/


std::vector<std::vector<int> >
plainbytetable(std::vector<int> V, std::vector<int> ArrayWidths)
{
  std::vector<int> multarray;
  std::vector<std::vector<int> > rowarray;
  std::vector<int> fixedarray;
  fixedarray.push_back(1);
  fixedarray.push_back(256);
  fixedarray.push_back(256 * 256);
  fixedarray.push_back(256 * 256 * 256);

  for(size_t i = 0; i < ArrayWidths.size(); i++)
  {
    int j = ArrayWidths[i];
    while (j > 0)
    {
      multarray.push_back(fixedarray[j - 1]);
      j--;
    }
  }
  size_t ncol = multarray.size();
  if(ncol == 0) Rcpp::stop("No columns");
  int nrow = V.size() / ncol;
  for(int i = 0; i < (nrow); i++)
  {
    std::vector<int> tmprow;
    int cumval = 0;
    for(size_t j = 0; j < ncol; j++)
    {
      cumval += (V[i * ncol + j] * multarray[j]);
      if(multarray[j] == 1)
      {
        tmprow.push_back(cumval);
        cumval = 0;
      }
    }
    rowarray.push_back(tmprow);
  }
  std::vector<std::vector<int> > colarray;
  for(size_t i = 0; i < rowarray[0].size(); i++)
  {
    std::vector<int> newcol;
    for(size_t j = 0; j < rowarray.size(); j++)
      newcol.push_back(rowarray[j][i]);
    colarray.push_back(newcol);
  }
  std::vector<int> objind;
  for(size_t i = 0; i < rowarray.size(); i++)
    objind.push_back((int) i);
  colarray.push_back(objind);
  return colarray;
}


std::vector<std::vector<int>>
decodeString(document& d, const std::string& filestring, int objstart)
{
  std::vector<std::vector<int>> ReAr, FAr, FA;
  std::vector<int> ArW, objind;
  int startingobj;
  dictionary dict = dictionary(filestring, objstart);
  std::string CS, IMatch, WM;
  if(dict.has("/Index")) IMatch = dict.get("/Index");
  if(dict.has("/W"))  WM = dict.get("/W");
  if(IMatch.size() == 0) startingobj = 0;
  else
  {
    std::vector<std::string> IMatchs = Rex(IMatch, "(\\d|\\s)+").get();
    if(IMatchs.size() == 0) startingobj = 0;
    else startingobj = std::stoi(Rex(IMatchs[0], "\\d+").get()[0]);
  }
  if(dict.has("/DecodeParms"))
  {
    dictionary subdict = dictionary(dict.get("/DecodeParms"));
    if(subdict.has("/Columns")) CS = subdict.get("/Columns");
  }
  int ncols = 0;
  if(!CS.empty()) ncols  =  std::stoi(Rex(CS, "\\d{1,2}").get()[0]) + 1;
  if(!WM.empty())
    {
      std::vector<std::string> WMs = Rex(WM, "(\\d|\\s)+").get();
      WMs = splitter(WMs[0], " ");
      for(auto i : WMs) ArW.push_back(std::stoi(i));
      std::string SS = getStreamContents(d, filestring, objstart);
      if(isFlateDecode(filestring, objstart)) SS = FlateDecode(SS);
      std::vector<unsigned char> rawarray(SS.begin(), SS.end());
      std::vector<int> intAr(rawarray.begin(), rawarray.end());
      if(!dict.has("/DecodeParms")) return plainbytetable(intAr, ArW);
      int nrows = intAr.size() / ncols;
      for(int i = 0; i < nrows; i++)
      {
        std::vector<int>
        tmp(intAr.begin() + ncols * i + 1, intAr.begin() + ncols * (i + 1));
        ReAr.push_back(tmp);
      }
      for(unsigned int i = 0; i < ReAr.size(); i++ ) if(i > 0)
      {
        std::vector<int> tAr = ReAr[i];
        for(size_t j = 0; j<tAr.size(); j++)
          ReAr[i][j] = tAr[j] + ReAr[i - 1][j];
      }
      for(unsigned int i = 0; i < ReAr[0].size(); i++)
      {
        std::vector<int> temarray;
        for(int j = 0; j < nrows; j++) temarray.push_back(ReAr[j][i] % 256);
        FAr.push_back(temarray);
      }
      std::vector<int> BI {16777216, 65536, 256, 1};
      std::vector<int> CA;
      for(auto i: ArW) CA.insert(CA.end(), BI.end() - i, BI.end());
      for(size_t i = 0; i < FAr.size(); i++) for(auto &j : FAr[i]) j *= CA[i];
      int cumsum = 0;
      for(auto i : ArW)
      {
        std::vector<int> newcolumn = FAr[cumsum];
        if(i > 1) for(int j = 1; j < i; j++)
          for(size_t k = 0; k < FAr[cumsum + j].size(); k++)
            newcolumn[k] += FAr[cumsum + j][k];
        FA.push_back(newcolumn);
        cumsum += i;
      }
      for(size_t i = 0; i<FA[0].size(); i++) objind.push_back(i + startingobj);
      FA.push_back(objind);
    }
  return FA;
}
