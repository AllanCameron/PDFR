//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR DCTDecode header file                                               //
//                                                                           //
//  Copyright (C) 2018 - 2021 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_DCT

//---------------------------------------------------------------------------//

#define PDFR_DCT

#include "streams.h"

class DCTDecode : public Stream
{
public:
  DCTDecode(const std::string* input) : Stream(*input) {};
  DCTDecode(const CharString& input) : Stream(input) {};
};

//---------------------------------------------------------------------------//

#endif
