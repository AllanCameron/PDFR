//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR crypto header file                                                  //
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

#ifndef PDFR_CRYPTO
#define PDFR_CRYPTO

typedef unsigned long UL;

std::vector<uint8_t> perm(std::string str);
std::vector<uint8_t> upw();
UL rotateLeft(UL x, int y);
UL md5first(UL a, UL b, UL c, UL d, UL e, UL f, UL g);
UL md5second(UL a, UL b, UL c, UL d, UL e, UL f, UL g);
UL md5third(UL a, UL b, UL c,UL d, UL e, UL f,UL g);
UL md5fourth(UL a, UL b, UL c,UL d, UL e, UL f,UL g);
std::vector<uint8_t> md5(std::vector<uint8_t> input);
std::vector<uint8_t> md5(std::string input);
std::vector<uint8_t> rc4(std::vector<uint8_t> msg, std::vector<uint8_t> key);
std::string decryptStream(std::string streamstr, std::vector<uint8_t> key,
                          int objNum, int objGen);


#endif