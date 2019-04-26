//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Box header file                                                     //
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

#ifndef PDFR_BOX

//---------------------------------------------------------------------------//

#define PDFR_BOX

#include "page.h"

//---------------------------------------------------------------------------//
// Before we get to reading the page description program from the PDF, we first
// need to define some data structures we are going to need to store the output
// of the program in a way that can be read and processed efficiently.
//


//---------------------------------------------------------------------------//
// This enum allows Vertices to be labelled according to which clockwise
// direction around a polygon their incoming and outgoing edges point. The
// strange numbering is simply to allow a matching to the indices of the crop
// box passed from page.h, so that these labels can be used in place of indices

enum Direction {North = 3, South = 1, East = 2, West = 0, None = 4};

//---------------------------------------------------------------------------//
// Each vertex starts life at the corner of a whitespace box, then most are
// pruned until only those forming the vertices of text boxes remain. Along the
// way a Vertex has to know in which direction whitespace lies, which directions
// the edges of the text box enter and leave it, which other Vertex this one
// points to, and which group of vertices it belongs to. It also needs a
// deletion flad - hence the relatively large number of member variables. Some
// of these have been compressed into a flag byte to save space.

struct Vertex
{
  float x, y;
  uint8_t flags; // bits denote delete-void-void-void-NW-NE-SE-SW
  Direction In, Out;
  size_t points_to, group;

  Vertex(float a, float b, uint8_t c):
    x(a), y(b), flags(c), In(None), Out(None), points_to(0), group(0) {}

  Vertex(const Vertex& v) = default;
  Vertex& operator=(const Vertex& v) = default;
  Vertex& operator=(Vertex&& v){std::swap(v, *this); return *this;}

  inline bool is_closer_than(const Vertex& j, const float& edge)
  {
    return
    (Out == North && j.x == x && j.In  == North && j.y >  y && j.y < edge) ||
    (Out == South && j.x == x && j.In  == South && j.y <  y && j.y > edge) ||
    (Out == East  && j.y == y && j.In  == East  && j.x >  x && j.x < edge) ||
    (Out == West  && j.y == y && j.In  == West  && j.x <  x && j.x > edge) ;
  }
};

//---------------------------------------------------------------------------//
// A lot of the work of Whitespace.cpp is done by identifying and manipulating
// rectangles of whitespace. These are simply a struct of 4 co-ordinates and
// an additional deletion flag that allows a vector of WSbox to be iterated and
// allowing deletions without removing iterator validity.

struct Box
{
  float left, right, top, bottom;
  bool is_deleted;

  Box(float a, float b, float c, float d):
    left(a), right(b), top(c), bottom(d), is_deleted(false) {}

  Box(std::vector<float> v): is_deleted(false)
  {
    if(v.size() < 4) throw std::runtime_error("Box creation needs four floats");
    left = v[0];
    right = v[2];
    top = v[3];
    bottom = v[1];
  }

  Box(){}

  inline float operator[](int a)
  {
    switch(a)
    {
      case 0: return left;
      case 1: return bottom;
      case 2: return right;
      case 3: return top;
      default: throw std::runtime_error("Invalid box index");
    }
  }

  inline bool operator==(const Box& other) const
  {
    return left == other.left && right  == other.right &&
           top  == other.top  && bottom == other.bottom;
  }

  inline bool eq(const float& a, const float& b) const
  {
    return a - b < 0.1 && b - a < 0.1;
  }

  inline void expand_box_to_include_vertex(const Vertex& corner)
  {
      if(corner.x < left)    left   = corner.x;
      if(corner.x > right)   right  = corner.x;
      if(corner.y < bottom)  bottom = corner.y;
      if(corner.y > top)     top    = corner.y;
  }

  inline bool is_approximately_same_as(const Box& other) const
  {
    return eq(left, other.left) && eq(right, other.right) &&
           eq(top, other.top)  && eq(bottom, other.bottom);
  }

  inline void remove() {is_deleted = true;}

  inline bool is_NW_of(Vertex& v) const
  {
    return right >= v.x && left < v.x && top > v.y && bottom <= v.y;
  }

  inline bool is_NE_of(Vertex& v) const
  {
    return right > v.x && left <= v.x && top > v.y && bottom <= v.y;
  }

  inline bool is_SE_of(Vertex& v) const
  {
    return right > v.x && left <= v.x && top >= v.y && bottom < v.y;
  }

  inline bool is_SW_of(Vertex& v) const
  {
    return right >= v.x && left < v.x && top >= v.y && bottom < v.y;
  }

  inline float width() const { return right - left;}

  inline float height() const { return top - bottom;}

  inline bool shares_edge(const Box& other) const
  {
    return top  == other.top  || bottom == other.bottom ||
           left == other.left || right  == other.right  ;
  }

  inline bool is_adjacent(const Box& j)
  {
    return left == j.right && top == j.top && bottom == j.bottom;
  }

  Vertex get_vertex(int j)
  {
    switch(j)
    {
      case 0 : return Vertex  (left, top, 0x02);
      case 1 : return Vertex  (right, top, 0x01);
      case 2 : return Vertex  (left, bottom, 0x04);
      case 3 : return Vertex  (right, bottom, 0x08);
      default: return Vertex  (0, 0, 0);
    }
    return Vertex  (0, 0, 0);
  }

  void record_impingement_on(Vertex& v)
  {
    if(is_NW_of(v)) v.flags |= 0x08; // NW
    if(is_NE_of(v)) v.flags |= 0x04; // NE
    if(is_SE_of(v)) v.flags |= 0x02; // SE
    if(is_SW_of(v)) v.flags |= 0x01; // SW
  }

  inline bool engulfs(const Box& j)
  {
    return  j.bottom - bottom > -0.1 && j.top - top < 0.1 &&
            j.left - left > -0.1 && j.right - right < 0.1 && !(*this == j);
  }

};


#endif
