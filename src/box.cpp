//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR box implementation file                                             //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
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

unordered_map<uint8_t, pair<Direction, Direction>> Vertex::arrows_ =
{
  {0x00, {None, None}},   {0x01, {North, West}}, {0x02, {West, South}},
  {0x03, {West, West}},   {0x04, {South, East}}, {0x05, {None, None}},
  {0x06, {South, South}}, {0x07, {South, West}}, {0x08, {East, North}},
  {0x09, {North, North}}, {0x0A, {None, None}},  {0x0B, {West, North}},
  {0x0C, {East, East}},   {0x0D, {North, East}}, {0x0E, {East, South}},
  {0x0F, {None, None}}
};

//---------------------------------------------------------------------------//
// Create a vertex from a given corner of the box
// (0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right)
// Note, the given vertex is automatically flagged as being impinged at the
// correct compass direction

shared_ptr<Vertex> Box::GetVertex(int t_corner)
{
  switch(t_corner)
  {
    case 0 : return std::make_shared<Vertex>(left_,  top_,    0x02);
    case 1 : return std::make_shared<Vertex>(right_, top_,    0x01);
    case 2 : return std::make_shared<Vertex>(left_,  bottom_, 0x04);
    case 3 : return std::make_shared<Vertex>(right_, bottom_, 0x08);
    default: return std::make_shared<Vertex>(0, 0, 0);
  }
  return std::make_shared<Vertex>  (0, 0, 0);
}

//---------------------------------------------------------------------------//
// Marks a box's impingement on a given vertex. This records whether moving
// an arbitrarily small distance in a given direction from the vertex will
// place one inside the current box.

void Box::RecordImpingementOn(Vertex& t_vertex)
{
  if(IsNorthWestOf(t_vertex)) t_vertex.SetFlags(0x08);
  if(IsNorthEastOf(t_vertex)) t_vertex.SetFlags(0x04);
  if(IsSouthEastOf(t_vertex)) t_vertex.SetFlags(0x02);
  if(IsSouthWestOf(t_vertex)) t_vertex.SetFlags(0x01);
}
