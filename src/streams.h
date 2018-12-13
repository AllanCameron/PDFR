//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR streams implementation file                                         //
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


#ifndef PDFR_STREAMS
#define PDFR

#include "pdfr.h"

bool isFlateDecode(const std::string& filestring, int startpos);

std::string FlateDecode(const std::string& s);

std::string getStreamContents(document& d, const std::string& filestring,
                              int objstart);

bool objHasStream(const std::string& filestring, int objectstart);

bool isObject(const std::string& filestring, int objectstart);

std::string objectPreStream(const std::string& filestring, int objectstart);

std::vector<std::vector<int> >
plainbytetable(std::vector<int> V, std::vector<int> ArrayWidths);

std::vector<std::vector<int>>
decodeString(document& d, const std::string& filestring, int objstart);

#endif

