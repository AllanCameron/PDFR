#ifndef PDFR_STRINGFUNCTIONS
#define PDFR_STRINGFUNCTIONS

#include <Rcpp.h>
#include "pdfr.h"

/*---------------------------------------------------------------------------*/

std::vector<std::string> Rex(const std::string& s, std::string r);

std::vector<std::string> Rex(const std::vector<std::string>& s, std::string r);

bool RexIn(const std::string& s, std::string r);

std::vector<int> stringloc(const std::string& s,
                           const std::string& r,
                           std::string sf = "start");

std::vector<std::vector<int>> stringloc2(const std::string& s, std::string r);

std::vector<std::string> splitter(const std::string& s, const std::string& m);

std::string carveout(const std::string& subject,
                     const std::string& precarve,
                     const std::string& postcarve);

bool IsAscii(const std::string& tempint);

std::vector<uint16_t> strtoint(std::string x);

std::string intToString(uint16_t a);

std::vector<float> getnums(const std::string& s);

std::vector<int> getints(const std::string& s);

int dec2oct(int x);

int oct2dec(int x);

std::vector<unsigned char> bytesFromArray(const std::string& s);

std::vector<uint8_t> stringtobytes(const std::string& s);

std::string bytestostring(const std::vector<uint8_t>& v);

std::vector<float> matmul(std::vector<float> b, std::vector<float> a);

std::vector<float> six2nine(std::vector<float> a);

std::vector<float> stringvectomat(std::vector<std::string> b);

std::vector<float> stringtofloat(std::vector<std::string> b);

std::string intToHexstring(int i);

std::vector<std::string> splitfours(std::string s);

std::vector<std::string> splittwos(std::string s);

std::string byteStringToString(const std::string& s);

std::vector<int> getObjRefs(std::string ds);

bool isDictString(const std::string& s);

char symbol_type(const char c);

std::string namesToChar(std::string s, const std::string& encoding);

void trimRight(std::string& s);

#endif
