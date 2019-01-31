//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR grid header file                                                    //
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

#ifndef PDFR_GRID

//---------------------------------------------------------------------------//

#define PDFR_GRID

#include "graphic_state.h"

//---------------------------------------------------------------------------//

struct GSrow
{
  float       left,
              right,
              width,
              bottom,
              size;
  std::string font;
  std::vector<Unicode> glyph;
  bool        consumed;
  std::pair<int, int> rightjoin;
};

//---------------------------------------------------------------------------//

struct sort_left_right
{
    inline bool operator() (const GSrow& row1, const GSrow& row2)
    {
        return (row1.left < row2.left);
    }
};

//---------------------------------------------------------------------------//

class grid
{
public:
  grid(const graphic_state&);
  std::unordered_map<uint8_t, std::vector<GSrow>> output();
  std::vector<float> getBox();


private:
  graphic_state gs;
  std::vector<float> minbox;
  std::vector<uint8_t> gridpoints;
  std::unordered_map<uint8_t, std::vector<GSrow>> gridmap;
  void makegrid();
  void compareCells();
  void matchRight(GSrow&, uint8_t);
  void merge();
};

//---------------------------------------------------------------------------//

#endif
