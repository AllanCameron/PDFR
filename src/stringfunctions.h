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

#include <Rcpp.h>
#include "pdfr.h"

/*---------------------------------------------------------------------------*/

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

void trimRight(std::string& s);

#endif
