//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Box header file                                                     //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_BOX

//---------------------------------------------------------------------------//

#define PDFR_BOX

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

#include<utility>
#include<unordered_map>
#include<vector>
#include<algorithm>
#include<memory>
#include<stdexcept>

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

class Vertex
{
 public:
  Vertex(float x, float y, uint8_t flags):
    x_(x), y_(y), flags_(flags), points_to_(0), group_(0) {}

  Vertex(const Vertex& other) = default;
  Vertex& operator=(const Vertex& other) = default;
  Vertex& operator=(Vertex&& other) {std::swap(other, *this); return *this;}

  // Getters
  Direction In()        const {return arrows_.at(flags_ & 0x0f).first;}
  Direction Out()       const {return arrows_.at(flags_ & 0x0f).second;}
  float     GetX()      const {return x_;}
  float     GetY()      const {return y_;}
  uint8_t   GetFlags()  const {return flags_;}
  size_t    GetGroup()  const {return group_;}
  size_t    PointsTo()  const {return points_to_;}

  // Setters
  void SetFlags(uint8_t new_flag) { flags_ |= new_flag  ;}
  void SetGroup(size_t group)     { group_ = group      ;}
  void PointAt(size_t element)    { points_to_ = element;}

  bool IsCloserThan(const Vertex& other, const float& edge)
  {
    return
    (Out() == North && other.x_ == x_ && other.In() == North &&
    other.y_ > y_ && other.y_ < edge) ||
    (Out() == South && other.x_ == x_ && other.In() == South &&
    other.y_ < y_ && other.y_ > edge) ||
    (Out() == East  && other.y_ == y_ && other.In() == East  &&
    other.x_ > x_ && other.x_ < edge) ||
    (Out() == West  && other.y_ == y_ && other.In() == West  &&
    other.x_ < x_ && other.x_ > edge) ;
  }

private:
  float x_, y_;
  uint8_t flags_; // bits denote delete-void-void-void-NW-NE-SE-SW
  size_t points_to_,
         group_;
  static std::unordered_map<uint8_t, std::pair<Direction, Direction>> arrows_;
};

//---------------------------------------------------------------------------//
// The Box struct will form the basis for our page boundary, the text boxes,
// and the whitespace boxes created in page segmentation. Most of the member
// functions are boolean comparisons against other boxes and are inlined here.
// For this reason there is no separate implementation file.

class Box
{
 public:
  // Constructor from four separate floats
  Box(float left, float right, float top, float bottom)
   :  left_(left), right_(right), top_(top), bottom_(bottom), flags_(0){}

  // Constructor from length-4 vector
  Box(std::vector<float> floats): flags_(0)
  {
    if (floats.size() != 4) throw std::runtime_error("Box needs four floats");
    left_   = floats[0];
    right_  = floats[2];
    top_    = floats[3];
    bottom_ = floats[1];
  }

  // Default constructor
  Box(){}

  virtual ~Box() = default; //Box is a base class - make its destructor virtual
  Box(Box&&) = default;
  Box(const Box&) = default;
  Box& operator=(const Box&) = default;
  Box& operator=(Box&&) noexcept = default;

  // We can use the direction enum to access the edges of the box instead of
  // using getters if we need to calculate the edge we're interested in getting.
  float Edge(int side) const
  {
    switch (side)
    {
      case 0: return left_;
      case 1: return bottom_;
      case 2: return right_;
      case 3: return top_;
      default: throw std::runtime_error("Invalid box index");
    }
  }

  // Getters
  float GetLeft()             const { return this->left_               ;}
  float GetRight()            const { return this->right_              ;}
  float GetTop()              const { return this->top_                ;}
  float GetBottom()           const { return this->bottom_             ;}
  virtual float GetSize()     const { return this->top_ - this->bottom_;}
  bool  HasFlag(uint8_t flag) const { return (flags_ & flag) == flag   ;}

  // Setters
  void  SetLeft   (float   left)   { left_   = left  ;}
  void  SetRight  (float   right)  { right_  = right ;}
  void  SetTop    (float   top)    { top_    = top   ;}
  void  SetBottom (float   bottom) { bottom_ = bottom;}
  void  SetFlag   (uint8_t flag)   { flags_ |= flag  ;}

  // Make the box dimensions equal to the smallest box that covers this box
  // AND the other box
  void Merge(Box& other)
  {
    if(&other == this) return;
    this->left_   = std::min(this->left_,   other.left_  );
    this->right_  = std::max(this->right_,  other.right_ );
    this->bottom_ = std::min(this->bottom_, other.bottom_);
    this->top_    = std::max(this->top_,    other.top_   );
    other.Consume();
  }

  // Make the box dimensions equal to the smallest box that covers this box
  // and the given vertex
  void ExpandBoxToIncludeVertex(const Vertex& vertex)
  {
    this->left_   = std::min(this->left_,   vertex.GetX());
    this->right_  = std::max(this->right_,  vertex.GetX());
    this->bottom_ = std::min(this->bottom_, vertex.GetY());
    this->top_    = std::max(this->top_,    vertex.GetY());
  }

  // Compare two boxes for exact equality
  bool operator==(const Box& other) const
  {
    if (&other == this) return true;
    return left_ == other.left_ && right_  == other.right_  &&
           top_  == other.top_  && bottom_ == other.bottom_;
  }

  // Approximate equality between floats
  bool Eq(const float& lhs, const float& rhs) const
  {
    if (lhs == rhs) return true;
    return (lhs - rhs < 0.1) && (rhs - lhs < 0.1);
  }

  // Test for non-strict equality
  bool IsApproximatelySameAs(const Box& other) const
  {
    if (&other == this) return true;
    return Eq(left_, other.left_) && Eq(right_,  other.right_) &&
           Eq(top_,  other.top_)  && Eq(bottom_, other.bottom_ );
  }

  bool IsBeyond(const Box& other) const
  {
    return left_ > other.right_;
  }

  // Mark for deletion
  void Consume() { flags_ |= 0x01; }
  bool IsConsumed() const { return (flags_ & 0x01) == 0x01; }

  // Simple calculations of width and height
  float Width()  const { return right_ - left_  ;}
  float Height() const { return top_   - bottom_;}

  // Are two given boxes aligned on at least one side?
  bool SharesEdge(const Box& other) const
  {
    return this->top_  == other.top_  || this->bottom_ == other.bottom_ ||
           this->left_ == other.left_ || this->right_  == other.right_  ;
  }

  // Is this box immediately to the right of the given box, sharing two
  // vertices? This can be used to merge boxes
  bool IsAdjacent(const Box& other) const
  {
    return left_   == other.right_  &&
           top_    == other.top_    &&
           bottom_ == other.bottom_  ;
  }

  // Check whether one box partially covers another box
  bool Encroaches(Box& other)
  {
    if (&other == this) return true;
    return (left_ < other.right_ && right_ > other.left_) &&
           (bottom_ < other.top_ && top_ > other.bottom_);
  }

  // Is another box completely enclosed by this one?
  bool Engulfs(const Box& other) const
  {
    return  other.bottom_ - bottom_ > -0.1 && other.top_ - top_ < 0.1 &&
            other.left_ - left_ > -0.1 && other.right_ - right_ < 0.1 &&
            !(*this == other);
  }

  // The following four functions determine whether, for any given Vertex,
  // moving an arbitrarily small distance in the stated direction will put
  // us inside this box. This allows us to work out on which edges of which
  // boxes the point lies.
  bool IsNorthWestOf(Vertex& vertex) const
  {
    return right_ >= vertex.GetX() && left_   <  vertex.GetX() &&
           top_   >  vertex.GetY() && bottom_ <= vertex.GetY();
  }

  bool IsNorthEastOf(Vertex& vertex) const
  {
    return right_ >  vertex.GetX() && left_   <= vertex.GetX() &&
           top_   >  vertex.GetY() && bottom_ <= vertex.GetY();
  }

  bool IsSouthEastOf(Vertex& vertex) const
  {
    return right_ >  vertex.GetX() && left_   <= vertex.GetX() &&
           top_   >= vertex.GetY() && bottom_ <  vertex.GetY();
  }

  bool IsSouthWestOf(Vertex& vertex) const
  {
    return right_ >= vertex.GetX() && left_   <  vertex.GetX() &&
           top_   >= vertex.GetY() && bottom_ <  vertex.GetY();
  }

  // Create a vertex from a given corner of the box
  // (0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right)
  // Note, the given vertex is automatically flagged as being impinged at the
  // correct compass direction
  std::shared_ptr<Vertex> GetVertex(int corner);

  // Marks a box's impingement on a given vertex. This records whether moving
  // an arbitrarily small distance in a given direction from the vertex will
  // place one inside the current box.
  void RecordImpingementOn(Vertex& corner);

  // Return box dimensions as a vector for output
  std::vector<float> Vector() const { return {left_, bottom_, right_, top_};}

 private:
  float   left_, right_, top_, bottom_;
  uint8_t flags_; // void-void-void-void-void- no left - no right - delete
};


#endif
