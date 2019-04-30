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

#include "font.h"

//---------------------------------------------------------------------------//
// Before we get to reading the page description program from the PDF, we first
// need to define some data structures we are going to need to store the output
// of the program in a way that can be read and processed efficiently.
//
// One of our fundamental data structures is the box. This is a simple
// collection of four floats representing the edges of a rectangle along with
// a boolean flag that records whether the box is in use (this allows us to
// functionally delete boxes in a collection without moving data around). It
// has several member functions that allow comparisons with other boxes and
// individual vertices. The box is a key element of the text_box
//
// The other structure we need to define is the Vertex. This is an x, y point
// that contains information about the edges that meet there. Although the
// Vertex is created from the corners of boxes, they go on to form vertices
// of more complex polygons formed by merging boxes together.


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
  size_t points_to, group;

  inline Direction In() const {return arrows.at(flags & 0x0f).first;}
  inline Direction Out() const {return arrows.at(flags & 0x0f).second;}

  // We use this to map directions to vertices
  static std::unordered_map<uint8_t, std::pair<Direction, Direction>> arrows;

  Vertex(float a, float b, uint8_t c):
    x(a), y(b), flags(c), points_to(0), group(0) {}

  Vertex(const Vertex& v) = default;
  Vertex& operator=(const Vertex& v) = default;
  Vertex& operator=(Vertex&& v){std::swap(v, *this); return *this;}

  inline bool is_closer_than(const Vertex& j, const float& edge)
  {
    return
    (Out() == North && j.x == x && j.In() == North && j.y > y && j.y < edge) ||
    (Out() == South && j.x == x && j.In() == South && j.y < y && j.y > edge) ||
    (Out() == East  && j.y == y && j.In() == East  && j.x > x && j.x < edge) ||
    (Out() == West  && j.y == y && j.In() == West  && j.x < x && j.x > edge) ;
  }
};

//---------------------------------------------------------------------------//
// The Box struct will form the basis for our page boundary, the text boxes,
// and the whitespace boxes created in page segmentation. Most of the member
// functions are boolean comparisons against other boxes and are inlined here.
// For this reason there is no separate implementation file.

class Box
{
  // Data members
  float left, right, top, bottom;
  bool deletion_flag;

public:
  // Constructors: from separate floats, a vector of floats or default
  Box(float a, float b, float c, float d):
    left(a), right(b), top(c), bottom(d), deletion_flag(false) {}

  Box(std::vector<float> v): deletion_flag(false)
  {
    if(v.size() < 4) throw std::runtime_error("Box creation needs four floats");
    left = v[0];
    right = v[2];
    top = v[3];
    bottom = v[1];
  }

  Box(){}

  // We can use the direction enum to access the edges of the box instead of
  // using getters if we need to calculate the edge we're interested in getting.
  inline float edge(int a) const
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

  // Getters
  inline float get_left()   const   { return this->left;}
  inline float get_right()  const   { return this->right;}
  inline float get_top()    const   { return this->top;}
  inline float get_bottom() const   { return this->bottom;}
  inline float get_size()   const   { return this->top - this->bottom;}

  // Setters
  inline void  set_left   (float a) { left   = a;}
  inline void  set_right  (float a) { right  = a;}
  inline void  set_top    (float a) { top    = a;}
  inline void  set_bottom (float a) { bottom = a;}

  // Make the box dimensions equal to the smallest box that covers this box
  // AND the other box
  inline void merge(const Box& other)
  {
    this->left   = std::min(this->left,   other.left  );
    this->right  = std::max(this->right,  other.right );
    this->bottom = std::min(this->bottom, other.bottom);
    this->top    = std::max(this->top,    other.top   );
  }

  // Make the box dimensions equal to the smallest box that covers this box
  // and the given vertex
  inline void expand_box_to_include_vertex(const Vertex& corner)
  {
      if(corner.x < left)    left   = corner.x;
      if(corner.x > right)   right  = corner.x;
      if(corner.y < bottom)  bottom = corner.y;
      if(corner.y > top)     top    = corner.y;
  }

  // Compare two boxes for exact equality
  inline bool operator==(const Box& other) const
  {
    return left == other.left && right  == other.right &&
           top  == other.top  && bottom == other.bottom;
  }

  // Approximate equality between floats
  inline bool eq(const float& a, const float& b) const
  {
    return (a - b < 0.1) && (b - a < 0.1);
  }

  // Test for non-strict equality
  inline bool is_approximately_same_as(const Box& other) const
  {
    return eq(left, other.left) && eq(right,  other.right) &&
           eq(top,  other.top)  && eq(bottom, other.bottom );
  }

  // Mark for deletion
  inline void remove() {deletion_flag = true;}

  // Check for deletion status
  inline bool is_deleted() const { return deletion_flag;}

  // Simple calculations of width and height
  inline float width()  const { return right - left  ;}
  inline float height() const { return top   - bottom;}

  // Are two given boxes aligned on at least one side?
  inline bool shares_edge(const Box& other) const
  {
    return top  == other.top  || bottom == other.bottom ||
           left == other.left || right  == other.right  ;
  }

  // Is this box immediately to the right of the given box, sharing two
  // vertices? This can be used to merge boxes
  inline bool is_adjacent(const Box& j) const
  {
    return left == j.right && top == j.top && bottom == j.bottom;
  }

  // Check whether one box partially covers another box
  inline bool encroaches(Box& other)
  {
    return (left < other.right && right > other.left) &&
           (bottom < other.top && top > other.bottom);
  }

  // Is another box completely enclosed by this one?
  inline bool engulfs(const Box& j) const
  {
    return  j.bottom - bottom > -0.1 && j.top - top < 0.1 &&
            j.left - left > -0.1 && j.right - right < 0.1 && !(*this == j);
  }

  // The following four functions determine whether, for any given Vertex,
  // moving an arbitrarily small distance in the stated direction will put
  // us inside the box. This allows us to work out on which edges of which
  // boxes the point lies.
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

  // Create a vertex from a given corner of the box
  // (0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right)
  // Note, the given vertex is automatically flagged as being impinged at the
  // correct compass direction
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

  // Marks a box's impingement on a given vertex. This records whether moving
  // an arbitrarily small distance in a given direction from the vertex will
  // place one inside the current box.
  void record_impingement_on(Vertex& v)
  {
    if(is_NW_of(v)) v.flags |= 0x08; // NW
    if(is_NE_of(v)) v.flags |= 0x04; // NE
    if(is_SE_of(v)) v.flags |= 0x02; // SE
    if(is_SW_of(v)) v.flags |= 0x01; // SW
  }

  // Return box dimensions as a vector for output
  inline std::vector<float> vector() const {return {left, bottom, right, top};}

};


#endif
