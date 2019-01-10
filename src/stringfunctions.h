//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR stringfunctions header file                                         //
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


#ifndef PDFR_STRINGFUNCTIONS
#define PDFR_STRINGFUNCTIONS

using namespace std;

/*---------------------------------------------------------------------------*/

vector<string> splitter(const string& s, const string& m);
// [[Rcpp::export]]
std::string carveout(const std::string& subject, const std::string& pre,
                     const std::string& post);
bool IsAscii(const string& tempint);
// [[Rcpp::export]]
std::vector<float> getnums(const std::string& s);
// [[Rcpp::export]]
std::vector<int> getints(const std::string& s);
int oct2dec(int x);
vector<unsigned char> bytesFromArray(const string& s);
string bytestostring(const vector<uint8_t>& v);
vector<float> matmul(vector<float> b, vector<float> a);
vector<float> stringvectomat(vector<string> b);
vector<float> stringtofloat(vector<string> b);
string intToHexstring(int i);
vector<string> splitfours(string s);
vector<int> getObjRefs(string& ds);
bool isDictString(const string& s);
char symbol_type(const char c);
void trimRight(string& s);
size_t firstmatch(string& s, string m, int startpos);
void upperCase(string& s);
vector<RawChar> HexstringToRawChar(string& s);
vector<RawChar> StringToRawChar(string& s);
std::vector<int> refFinder(const std::string& s);
// [[Rcpp::export]]
std::vector<std::string>
  multicarve(const std::string& s, const std::string& a, const std::string& b);
#endif
