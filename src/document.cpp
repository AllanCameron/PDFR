#include "pdfr.h"
#include "document.h"
#include "streams.h"
#include "stringfunctions.h"
#include "crypto.h"



/*---------------------------------------------------------------------------*/

document::document(const std::string& filename) : file(filename)
{
  get_file();
  Xref = xref(*this);
  trailer = Xref.trailer();
  filekey = get_cryptkey();
  getCatalogue();
  getPageDir();
  isLinearized();
  getPageHeaders();
}

/*---------------------------------------------------------------------------*/

document::document(const std::vector<uint8_t>& bytevector)
{
  filestring = bytestostring(bytevector);
  filesize = (int) filestring.size();
  Xref = xref(*this);
  trailer = Xref.trailer();
  filekey = get_cryptkey();
  getCatalogue();
  getPageDir();
  isLinearized();
  getPageHeaders();
}

/*---------------------------------------------------------------------------*/

void document::get_file()
{
    std::ifstream in(file.c_str(), std::ios::in | std::ios::binary);
    fileCon = &in;
    fileCon->seekg(0, std::ios::end);
    filesize = fileCon->tellg();
    filestring.resize(filesize);
    fileCon->seekg(0, std::ios::beg);
    fileCon->read(&filestring[0], filestring.size());
    fileCon->seekg(0, std::ios::beg);
    fileCon->close();
}


/*---------------------------------------------------------------------------*/

object_class document::getobject(int n)
{
  if(this->objects.find(n) == this->objects.end())
    objects[n] = object_class(*this, n);
  return objects[n];
}

/*---------------------------------------------------------------------------*/

void document::getCatalogue()
{
  std::vector<int> rootnums = trailer.getInts("/Root");
  int bytenum = 0;
  if (rootnums.size() == 0) Rcpp::stop("Couldn't find catalogue from trailer");
  if (Xref.objectExists(rootnums.at(0))) bytenum = Xref.getStart(rootnums.at(0));
  catalogue = dictionary(filestring, bytenum);
}

/*---------------------------------------------------------------------------*/

void document::getPageDir()
{
  if(!catalogue.hasInts("/Pages")) Rcpp::stop("No valid /Pages entry");
  int pagenum = catalogue.getInts("/Pages").at(0);
  pagedir = getobject(pagenum);
}

/*---------------------------------------------------------------------------*/

void document::isLinearized()
{
  std::string tx(subfile(0, 100));
  std::vector < int > lin = stringloc(tx, "<</Linearized", "start");
  if (lin.size() == 0) linearized = false; else linearized = true;
}

/*---------------------------------------------------------------------------*/

std::vector <int> document::expandKids(std::vector<int> objnums)
{
  std::vector<bool> ispage(objnums.size(), true);

  unsigned int i = 0;
  while (i < objnums.size())
  {
    object_class o = getobject(objnums[i]);

    if (o.hasKids())
    {
      std::vector<int> tmpvec = o.getKids();
      objnums.insert( objnums.end(), tmpvec.begin(), tmpvec.end() );
      while(ispage.size() < objnums.size()) ispage.push_back(true);
      ispage[i] = false;
    }
    i++;
  }

  std::vector<int> res;
  for(i = 0; i < objnums.size(); i++) if(ispage[i]) res.push_back(objnums[i]);
  return res;
}


/*---------------------------------------------------------------------------*/

std::vector <int> document::expandContents(std::vector<int> objnums)
{
  unsigned int i = 0;
  while (i < objnums.size())
  {
    object_class o = getobject(objnums[i]);
    if (o.hasContents())
    {
      std::vector<int> tmpvec = o.getContents();
      objnums.erase(objnums.begin() + i);
      objnums.insert(objnums.begin() + i, tmpvec.begin(), tmpvec.end());
      i = 0;
    }
    else i++;
  }
  return objnums;
}

/*---------------------------------------------------------------------------*/

void document::getPageHeaders()
{
  if (pagedir.hasKids())
  {
    std::vector<int> finalkids = expandKids(pagedir.getKids());
    for (auto i : finalkids) pageheaders.push_back(getobject(i).getDict());
  }
}

/*---------------------------------------------------------------------------*/

page document::getPage(int pagenum)
{
  return page(*this, pagenum);
}

/*---------------------------------------------------------------------------*/


std::string document::subfile(int startbyte, int len)
{
  return filestring.substr(startbyte, len);
}



std::vector<uint8_t> document::get_cryptkey()
{
  dictionary dict = this->trailer;
  std::vector<std::string> res;
  std::vector<uint8_t> ubytes, obytes, idbytes, pbytes, Fstring, blank;
  std::vector<std::vector<uint8_t>> vecvec;
  std::string encstr, idstr;
  int rnum;
  unsigned int cryptlen;
  if(dict.has("/Encrypt"))
  {
    this->encrypted = true;
    std::string encheader;
    int encnum = dict.getInts("/Encrypt")[0];
    if(this->Xref.objectExists(encnum))
    {
      encheader = objectPreStream(this->filestring,
                                  this->Xref.getStart(encnum));
    }
    int ehl = encheader.length();

    if(ehl > 0)
    {
      std::vector<std::string> ps = Rex(encheader, "/P( )+(-)?\\d+");
      if(ps.size() > 0)
      {
        pbytes = perm(ps[0].substr(2, ps[0].length() - 2));
      }
      std::vector<std::string> rs = Rex(encheader, "/R \\d");
      if(rs.size() > 0)
      {
        rnum = stoi(rs[0].substr(3, rs[0].length() - 3));
      }
      else
      {
        rnum = 2;
      }
      std::vector<std::string> ls = Rex(encheader, "/Length \\d+");

      if(ls.size() > 0 && rnum > 2)
      {
        cryptlen = stoi(ls[0].substr(8, ls[0].length() - 8))/8;
      }
      else
      {
        cryptlen = 5;
      }
      std::vector<int> ostarts = stringloc(encheader, "/O\\(", "end");
      std::vector<int> ustarts = stringloc(encheader, "/U\\(", "end");
      if(ostarts.size() > 0)
      {
        int ostart = ostarts[0];
        if(ostart + 32 < ehl)
        {
          std::string ostring = encheader.substr(ostart, 32);
          for(auto j : ostring) obytes.push_back(j);
        }
      }
      if(ustarts.size() > 0)
      {
        int ustart = ustarts[0];
        if(ustart + 32 < ehl)
        {
          std::string ustring = encheader.substr(ustart, 32);
          for(auto j : ustring) ubytes.push_back(j);
        }
      }
      std::string idstr = dict.get("/ID");
      if(idstr.length() > 0)
      {
        idbytes = bytesFromArray(idstr);
        while(idbytes.size() > 16) idbytes.pop_back();
      }
      std::vector<uint8_t> up = upw();
      Fstring = up;
      Fstring.insert(Fstring.end(), obytes.begin(), obytes.end());
      Fstring.insert(Fstring.end(), pbytes.begin(), pbytes.end());
      Fstring.insert(Fstring.end(), idbytes.begin(), idbytes.end());
      std::vector<uint8_t> filekey = md5(Fstring);
      while(filekey.size() > cryptlen) filekey.pop_back();

      if(rnum > 2)
      {
        for(unsigned it = 0; it < 50; it++)
        {
          filekey = md5(filekey);
          while(filekey.size() > cryptlen) filekey.pop_back();
        }
      }

      // filekey produced - now check it;
      std::vector<uint8_t> checkans;

      // algorithm for revision 2
      if(rnum == 2)
      {
        checkans = rc4(up, filekey);
        if(checkans.size() == 32)
        {
          int m = 0;
          for(unsigned int l = 0; l < 32; l++)
          {
            if(checkans[l] != ubytes[l]) break;
            m++;
          }
          if(m == 32)
          {
            return filekey;
          }
          else
          {
            return blank;
          }
        }
      }
      if(rnum > 2)
      {
        //std::cout << "rnum = " << rnum << std::endl;
        std::vector<uint8_t> buf = up;
        buf.insert(buf.end(), idbytes.begin(), idbytes.end());
        checkans = md5(buf);
        checkans = rc4(checkans, filekey);
        for (int iii = 19; iii >= 0; iii--)
        {
          uint8_t ii = iii;
          std::vector<uint8_t> tmpkey;
          for (unsigned int jj = 0; jj < filekey.size(); ++jj)
          {
            tmpkey.push_back(filekey[jj] ^ ii);
          }
          checkans = rc4(checkans, tmpkey);
        }
        int m = 0;
        for(unsigned int l = 0; l < 16; l++)
        {
          if(checkans[l] != ubytes[l]) break;
          m++;
        }
        if(m == 16)
        {
          return filekey;
        }
        else
        {
          /* std::cout << "filekey does not match user string."
                       << std::endl; */
          return filekey;
        }
      }
    }
  }
  this->encrypted = false;
  return blank;
}

