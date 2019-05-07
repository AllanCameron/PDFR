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
  float x_, y_;
  uint8_t flags_; // bits denote delete-void-void-void-NW-NE-SE-SW
  size_t points_to_, group_;

  inline Direction In() const {return arrows_.at(flags_ & 0x0f).first;}
  inline Direction Out() const {return arrows_.at(flags_ & 0x0f).second;}

  // We use this to map directions to vertices
  static std::unordered_map<uint8_t, std::pair<Direction, Direction>> arrows_;

  Vertex(float t_x, float t_y, uint8_t t_flags):
    x_(t_x), y_(t_y), flags_(t_flags), points_to_(0), group_(0) {}

  Vertex(const Vertex& t_other) = default;
  Vertex& operator=(const Vertex& t_other) = default;
  Vertex& operator=(Vertex&& t_other) {std::swap(t_other, *this); return *this;}

  inline bool IsCloserThan(const Vertex& t_other, const float& edge)
  {
    return
    (Out() == North && t_other.x_ == x_ && t_other.In() == North &&
    t_other.y_ > y_ && t_other.y_ < edge) ||
    (Out() == South && t_other.x_ == x_ && t_other.In() == South &&
    t_other.y_ < y_ && t_other.y_ > edge) ||
    (Out() == East  && t_other.y_ == y_ && t_other.In() == East  &&
    t_other.x_ > x_ && t_other.x_ < edge) ||
    (Out() == West  && t_other.y_ == y_ && t_other.In() == West  &&
    t_other.x_ < x_ && t_other.x_ > edge) ;
  }
};

//---------------------------------------------------------------------------//
// The Box struct will form the basis for our page boundary, the text boxes,
// and the whitespace boxes created in page segmentation. Most of the member
// functions are boolean comparisons against other boxes and are inlined here.
// For this reason there is no separate implementation file.

class Box
{
 private:
  // Data members
  float left_, right_, top_, bottom_;
  uint8_t flags_;

 public:
  // Constructor from four separate floats
  Box(float t_left, float t_right, float t_top, float t_bottom):
    left_(t_left), right_(t_right), top_(t_top), bottom_(t_bottom), flags_(0){}

  // Constructor from length-4 vector
  Box(std::vector<float> t_vector): flags_(0)
  {
    if(t_vector.size() < 4) throw std::runtime_error("Box needs four floats");
    left_   = t_vector[0];
    right_  = t_vector[2];
    top_    = t_vector[3];
    bottom_ = t_vector[1];
  }

  // Default constructor
  Box(){}

  // We can use the direction enum to access the edges of the box instead of
  // using getters if we need to calculate the edge we're interested in getting.
  inline float edge(int t_side) const
  {
    switch(t_side)
    {
      case 0: return left_;
      case 1: return bottom_;
      case 2: return right_;
      case 3: return top_;
      default: throw std::runtime_error("Invalid box index");
    }
  }

  inline void SetFlag(uint8_t t_flag) { flags_ |= t_flag;}

  inline bool HasFlag(uint8_t t_flag) const
  {
    return (flags_ & t_flag) == t_flag;
  }

  // Getters
  inline float GetLeft()   const   { return this->left_;}
  inline float GetRight()  const   { return this->right_;}
  inline float GetTop()    const   { return this->top_;}
  inline float GetBottom() const   { return this->bottom_;}
  inline float GetSize()   const   { return this->top_ - this->bottom_;}

  // Setters
  inline void  SetLeft   (float t_left)   { left_   = t_left  ;}
  inline void  SetRight  (float t_right)  { right_  = t_right ;}
  inline void  SetTop    (float t_top)    { top_    = t_top   ;}
  inline void  SetBottom (float t_bottom) { bottom_ = t_bottom;}

  // Make the box dimensions equal to the smallest box that covers this box
  // AND the other box
  inline void Merge(Box& t_other)
  {
    this->left_   = std::min(this->left_,   t_other.left_  );
    this->right_  = std::max(this->right_,  t_other.right_ );
    this->bottom_ = std::min(this->bottom_, t_other.bottom_);
    this->top_    = std::max(this->top_,    t_other.top_   );
    t_other.Consume();
  }

  // Make the box dimensions equal to the smallest box that covers this box
  // and the given vertex
  inline void ExpandBoxToIncludeVertex(const Vertex& t_corner)
  {
      if(t_corner.x_ < left_)    left_   = t_corner.x_;
      if(t_corner.x_ > right_)   right_  = t_corner.x_;
      if(t_corner.y_ < bottom_)  bottom_ = t_corner.y_;
      if(t_corner.y_ > top_)     top_    = t_corner.y_;
  }

  // Compare two boxes for exact equality
  inline bool operator==(const Box& t_other) const
  {
    return left_ == t_other.left_ && right_  == t_other.right_  &&
           top_  == t_other.top_  && bottom_ == t_other.bottom_;
  }

  // Approximate equality between floats
  inline bool Eq(const float& t_lhs, const float& t_rhs) const
  {
    return (t_lhs - t_rhs < 0.1) && (t_rhs - t_lhs < 0.1);
  }

  // Test for non-strict equality
  inline bool IsApproximatelySameAs(const Box& t_other) const
  {
    return Eq(left_, t_other.left_) && Eq(right_,  t_other.right_) &&
           Eq(top_,  t_other.top_)  && Eq(bottom_, t_other.bottom_ );
  }

  inline bool IsBeyond(const Box& t_other) const
  {
    return left_ > t_other.right_;
  }

  // Mark for deletion
  inline void Consume() { flags_ |= 0x01; }
  inline bool IsConsumed() const { return (flags_ & 0x01) == 0x01; }

  // Simple calculations of width and height
  inline float Width()  const { return right_ - left_  ;}
  inline float Height() const { return top_   - bottom_;}

  // Are two given boxes aligned on at least one side?
  inline bool SharesEdge(const Box& t_other) const
  {
    return this->top_  == t_other.top_  || this->bottom_ == t_other.bottom_ ||
           this->left_ == t_other.left_ || this->right_  == t_other.right_  ;
  }

  // Is this box immediately to the right of the given box, sharing two
  // vertices? This can be used to merge boxes
  inline bool IsAdjacent(const Box& t_other) const
  {
    return left_   == t_other.right_  &&
           top_    == t_other.top_    &&
           bottom_ == t_other.bottom_  ;
  }

  // Check whether one box partially covers another box
  inline bool Encroaches(Box& t_other)
  {
    return (left_ < t_other.right_ && right_ > t_other.left_) &&
           (bottom_ < t_other.top_ && top_ > t_other.bottom_);
  }

  // Is another box completely enclosed by this one?
  inline bool Engulfs(const Box& t_other) const
  {
    return  t_other.bottom_ - bottom_ > -0.1 && t_other.top_ - top_ < 0.1 &&
            t_other.left_ - left_ > -0.1 && t_other.right_ - right_ < 0.1 &&
            !(*this == t_other);
  }

  // The following four functions determine whether, for any given Vertex,
  // moving an arbitrarily small distance in the stated direction will put
  // us inside this box. This allows us to work out on which edges of which
  // boxes the point lies.
  inline bool IsNorthWestOf(Vertex& t_vertex) const
  {
    return right_ >= t_vertex.x_ && left_   <  t_vertex.x_ &&
           top_   >  t_vertex.y_ && bottom_ <= t_vertex.y_;
  }

  inline bool IsNorthEastOf(Vertex& t_vertex) const
  {
    return right_ >  t_vertex.x_ && left_   <= t_vertex.x_ &&
           top_   >  t_vertex.y_ && bottom_ <= t_vertex.y_;
  }

  inline bool IsSouthEastOf(Vertex& t_vertex) const
  {
    return right_ >  t_vertex.x_ && left_   <= t_vertex.x_ &&
           top_   >= t_vertex.y_ && bottom_ <  t_vertex.y_;
  }

  inline bool IsSouthWestOf(Vertex& t_vertex) const
  {
    return right_ >= t_vertex.x_ && left_   <  t_vertex.x_ &&
           top_   >= t_vertex.y_ && bottom_ <  t_vertex.y_;
  }

  // Create a vertex from a given corner of the box
  // (0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right)
  // Note, the given vertex is automatically flagged as being impinged at the
  // correct compass direction
  std::shared_ptr<Vertex> GetVertex(int t_corner)
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

  // Marks a box's impingement on a given vertex. This records whether moving
  // an arbitrarily small distance in a given direction from the vertex will
  // place one inside the current box.
  void RecordImpingementOn(Vertex& t_vertex)
  {
    if(IsNorthWestOf(t_vertex)) t_vertex.flags_ |= 0x08;
    if(IsNorthEastOf(t_vertex)) t_vertex.flags_ |= 0x04;
    if(IsSouthEastOf(t_vertex)) t_vertex.flags_ |= 0x02;
    if(IsSouthWestOf(t_vertex)) t_vertex.flags_ |= 0x01;
  }

  // Return box dimensions as a vector for output
  inline std::vector<float> vector() const
  {
    return {left_, bottom_, right_, top_};
  }

};


#endif
