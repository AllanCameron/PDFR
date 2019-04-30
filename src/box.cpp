//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR box implementation file                                             //
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


#include "box.h"
using namespace std;

//---------------------------------------------------------------------------//
// Every vertex of the final polygon surrounding each text element contains
// information about its position. However, to "connect" the vertices so as to
// arrange them in clockwise order, it also needs to know which direction
// the incoming and outgoing edges are "pointing". We do this by working out
// for each vertex whether there is whitespace immediately to the NorthWest,
// NorthEast, SouthEast and SouthWest of the vertex. These are recorded as the
// four lowest order bits in a single "flags" byte; thus a vertex that had
// whitespace to the NorthWest and SouthWest (as it would if it lay along the
// middle of the left edge of a text polygon), would have its flags set to
// 1001 in binary (or 0x9 in hexadecimal, or a byte value of 0x09 provided we
// mask the flag byte with & 0x0f). Since we know that a masked flag value of
// 0x09 must represent a point lying on a left edge, its "incoming" edge must
// be travelling North (since we are concerned with clockwise ordering), and
// its outgoing edge must also be pointing North. To make all this clearer we
// want each Vertex to specify its incoming and outgoing directions. We can
// therefore just look up the four lowest order bytes in each Vertex's flags
// using this unordered map to get the implied direction based on the
// surrounding whitespace.

unordered_map<uint8_t, pair<Direction, Direction>> Vertex::arrows =
{
  {0x00, {None, None}},   {0x01, {North, West}}, {0x02, {West, South}},
  {0x03, {West, West}},   {0x04, {South, East}}, {0x05, {None, None}},
  {0x06, {South, South}}, {0x07, {South, West}}, {0x08, {East, North}},
  {0x09, {North, North}}, {0x0A, {None, None}},  {0x0B, {West, North}},
  {0x0C, {East, East}},   {0x0D, {North, East}}, {0x0E, {East, South}},
  {0x0F, {None, None}}
};
