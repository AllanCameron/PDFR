//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR fileio implementation file                                          //
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
#include "fileio.h"

//---------------------------------------------------------------------------//

std::string get_file_contents(const std::string& filename)
{
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
  if(in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  throw "Unable to read pdf file";
}

//---------------------------------------------------------------------------//

std::string get_partial_file(const std::string& filename, long start, long stop)
{
  if(stop < start || start < 0)
    throw "Invalid file pointers";
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
  if(in)
  {
    std::string contents;
    contents.resize(stop - start);
    in.seekg(0, std::ios::end);
    if(stop > ((long) in.tellg())) throw "Invalid file pointers";
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  throw "Unable to read pdf file";
}

/*---------------------------------------------------------------------------*/

std::vector<uint8_t> read_bytes(const std::string& x)
{
  std::vector<uint8_t> res(x.begin(), x.end());
  return res;
}

/*---------------------------------------------------------------------------*/

std::vector<uint8_t> read_file_bytes(const std::string& filename)
{
  return read_bytes(get_file_contents(filename));
}

/*---------------------------------------------------------------------------*/

std::vector<std::string> sanitize_string(std::string x)
{
  std::vector<char> charraw;
  std::vector<std::string> restring;

  for(std::string::iterator i = x.begin(); i != x.end(); i++)
  {
    if(*i != 0 && *i != 10 && *i != 13)
      charraw.push_back(*i);
    if((*i == 10 || *i == 13) && !charraw.empty())
    {
      restring.push_back(std::string(charraw.data(), charraw.size()));
      charraw.clear();
    }
  }
  return restring;
}

/*---------------------------------------------------------------------------*/

std::vector<std::string> read_file_string(std::string myfile)
{
  return sanitize_string(get_file_contents(myfile));
}

/*---------------------------------------------------------------------------*/

int file_size(const std::string& file)
{
  std::ifstream f;
  f.open(file.c_str(), std::ios_base::binary | std::ios_base::in);
  if (!f.good() || f.eof() || !f.is_open()) { return 0; }
  f.seekg(0, std::ios_base::beg);
  std::ifstream::pos_type begin_pos = f.tellg();
  f.seekg(0, std::ios_base::end);
  return static_cast<int>(f.tellg() - begin_pos);
}

/*---------------------------------------------------------------------------*/

std::string partial_file(const std::string& filename, int startpos, int endpos)
{
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
  if (in)
  {
    int filesize = file_size(filename);
    if((endpos == 0) || (endpos > filesize)) endpos = filesize;
    if(startpos < 0) startpos = filesize + startpos;
    if(endpos < 0) endpos = filesize + endpos;
    int contentsize = endpos - startpos;
    if(startpos < endpos)
    {
      std::string contents;
      contents.resize(contentsize);
      in.seekg(startpos, std::ios::beg);
      in.read(&contents[0], contentsize);
      in.close();
      return(contents);
    }
  }
  throw "Couldn't load file.";
}


