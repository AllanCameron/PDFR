//---------------------------------------------------------------------------//
//                                                                           //
// PDFR encoding header file                                                 //
//                                                                           //
// Copyright (C) 2019 by Allan Cameron                                       //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining     //
// a copy of this software and associated documentation files                //
// (the "Software"), to deal in the Software without restriction, including  //
// without limitation the rights to use, copy, modify, merge, publish,       //
// distribute, sublicense, and/or sell copies of the Software, and to        //
// permit persons to whom the Software is furnished to do so, subject to     //
// the following conditions:                                                 //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.    //
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY      //
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,      //
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE         //
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                    //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_ENCODING
#define PDFR_ENCODING

#include "document.h"

class Encoding
{
private:
  static std::unordered_map<std::string, Unicode> AdobeToUnicode;
  std::unordered_map<RawChar, Unicode> EncodingMap;
  void parseDifferences(const std::string&,
                        std::unordered_map<RawChar, Unicode>&);
  static std::unordered_map<RawChar, Unicode> macRomanEncodingToUnicode;
  static std::unordered_map<RawChar, Unicode> winAnsiEncodingToUnicode;
  static std::unordered_map<RawChar, Unicode> pdfDocEncodingToUnicode;
  dictionary fontref;
  document* d;
  std::string BaseEncoding;
  void getEncoding(dictionary&, document*);
  void processUnicodeRange(std::vector<std::string>&);
  void processUnicodeChars(std::vector<std::string>&);
  void mapUnicode(dictionary&, document*);

public:
  Encoding(const dictionary&, document*);
  Unicode Interpret(RawChar);
  std::vector<RawChar> encKeys();
};

#endif
