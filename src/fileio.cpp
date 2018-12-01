#include "pdfr.h"
#include "fileio.h"

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
  Rcpp::stop("Unable to read pdf file");
}


std::string get_partial_file(const std::string& filename, long start, long stop)
{
  if(stop < start || start < 0) Rcpp::stop("Invalid file pointers");
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
  if(in)
  {
    std::string contents;
    contents.resize(stop - start);
    in.seekg(0, std::ios::end);
    if(stop > ((long) in.tellg())) Rcpp::stop("Invalid file pointers");
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  Rcpp::stop("Unable to read pdf file");
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
  std::vector<char>   charraw;
  std::vector<std::string> restring;

  for(std::string::iterator i = x.begin(); i != x.end(); i++)
  {
    if(*i != 0 && *i != 10 && *i != 13) charraw.push_back(*i);
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
  Rcpp::stop("Couldn't load file.");
}


