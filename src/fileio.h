#ifndef RCPP_FILEIO
#define RCPP_FILEIO

#include "pdfr.h"

std::string get_file_contents(const std::string& filename);
std::string get_partial_file(const std::string& filename, long start, long stop);
std::vector<uint8_t> read_bytes(const std::string& x);
std::vector<uint8_t> read_file_bytes(const std::string& filename);
std::vector<std::string> sanitize_string(std::string x);
std::vector<std::string> read_file_string(std::string myfile);
int file_size(const std::string& file);
std::string partial_file(const std::string& filename, int startpos, int endpos);


#endif
