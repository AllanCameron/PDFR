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
  Vertex(float p_x, float p_y, uint8_t p_flags):
    m_x(p_x), m_y(p_y), m_flags(p_flags), m_points_to(0), m_group(0) {}

  Vertex(const Vertex& p_other) = default;
  Vertex& operator=(const Vertex& p_other) = default;
  Vertex& operator=(Vertex&& p_other) {std::swap(p_other, *this); return *this;}

  // Getters
  inline Direction In()  const {return sm_arrows.at(m_flags & 0x0f).first;}
  inline Direction Out() const {return sm_arrows.at(m_flags & 0x0f).second;}
  inline float     GetX()      const {return m_x;}
  inline float     GetY()      const {return m_y;}
  inline uint8_t   GetFlags()  const {return m_flags;}
  inline size_t    GetGroup()  const {return m_group;}
  inline size_t    PointsTo()  const {return m_points_to;}

  // Setters
  inline void SetFlags(uint8_t p_new_flag) { m_flags |= p_new_flag;}
  inline void SetGroup(size_t p_group) { m_group = p_group;}
  inline void PointAt(size_t p_element) { m_points_to = p_element;}

  inline bool IsCloserThan(const Vertex& p_other, const float& edge)
  {
    return
    (Out() == North && p_other.m_x == m_x && p_other.In() == North &&
    p_other.m_y > m_y && p_other.m_y < edge) ||
    (Out() == South && p_other.m_x == m_x && p_other.In() == South &&
    p_other.m_y < m_y && p_other.m_y > edge) ||
    (Out() == East  && p_other.m_y == m_y && p_other.In() == East  &&
    p_other.m_x > m_x && p_other.m_x < edge) ||
    (Out() == West  && p_other.m_y == m_y && p_other.In() == West  &&
    p_other.m_x < m_x && p_other.m_x > edge) ;
  }

private:
  float m_x, m_y;
  uint8_t m_flags; // bits denote delete-void-void-void-NW-NE-SE-SW
  size_t m_points_to,
         m_group;
  static std::unordered_map<uint8_t, std::pair<Direction, Direction>> sm_arrows;
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
  Box(float p_left, float p_right, float p_top, float p_bottom)
   : m_left(p_left), m_right(p_right), m_top(p_top),
     m_bottom(p_bottom), m_flags(0){}

  // Constructor from length-4 vector
  Box(std::vector<float> p_vector): m_flags(0)
  {
    if (p_vector.size() < 4) throw std::runtime_error("Box needs four floats");
    m_left   = p_vector[0];
    m_right  = p_vector[2];
    m_top    = p_vector[3];
    m_bottom = p_vector[1];
  }

  // Default constructor
  Box(){}

  virtual ~Box() = default; //Box is a base class - make its destructor virtual

  // We can use the direction enum to access the edges of the box instead of
  // using getters if we need to calculate the edge we're interested in getting.
  inline float Edge(int p_side) const
  {
    switch (p_side)
    {
      case 0: return m_left;
      case 1: return m_bottom;
      case 2: return m_right;
      case 3: return m_top;
      default: throw std::runtime_error("Invalid box index");
    }
  }

  // Getters
  inline float GetLeft()   const   { return this->m_left               ;}
  inline float GetRight()  const   { return this->m_right              ;}
  inline float GetTop()    const   { return this->m_top                ;}
  inline float GetBottom() const   { return this->m_bottom             ;}
  inline virtual float GetSize() const { return this->m_top - this->m_bottom;}
  inline bool  HasFlag(uint8_t p_flag) const
  {
    return (m_flags & p_flag) == p_flag;
  }

  // Setters
  inline void  SetLeft   (float   p_left)   { m_left   = p_left  ;}
  inline void  SetRight  (float   p_right)  { m_right  = p_right ;}
  inline void  SetTop    (float   p_top)    { m_top    = p_top   ;}
  inline void  SetBottom (float   p_bottom) { m_bottom = p_bottom;}
  inline void  SetFlag   (uint8_t p_flag)   { m_flags |= p_flag  ;}

  // Make the box dimensions equal to the smallest box that covers this box
  // AND the other box
  inline void Merge(Box& p_other)
  {
    if(&p_other == this) return;
    this->m_left   = std::min(this->m_left,   p_other.m_left  );
    this->m_right  = std::max(this->m_right,  p_other.m_right );
    this->m_bottom = std::min(this->m_bottom, p_other.m_bottom);
    this->m_top    = std::max(this->m_top,    p_other.m_top   );
    p_other.Consume();
  }

  // Make the box dimensions equal to the smallest box that covers this box
  // and the given vertex
  inline void ExpandBoxToIncludeVertex(const Vertex& p_corner)
  {
    this->m_left   = std::min(this->m_left,   p_corner.GetX());
    this->m_right  = std::max(this->m_right,  p_corner.GetX());
    this->m_bottom = std::min(this->m_bottom, p_corner.GetY());
    this->m_top    = std::max(this->m_top,    p_corner.GetY());
  }

  // Compare two boxes for exact equality
  inline bool operator==(const Box& p_other) const
  {
    if (&p_other == this) return true;
    return m_left == p_other.m_left && m_right  == p_other.m_right  &&
           m_top  == p_other.m_top  && m_bottom == p_other.m_bottom;
  }

  // Approximate equality between floats
  inline bool Eq(const float& p_lhs, const float& p_rhs) const
  {
    if (p_lhs == p_rhs) return true;
    return (p_lhs - p_rhs < 0.1) && (p_rhs - p_lhs < 0.1);
  }

  // Test for non-strict equality
  inline bool IsApproximatelySameAs(const Box& p_other) const
  {
    if (&p_other == this) return true;
    return Eq(m_left, p_other.m_left) && Eq(m_right,  p_other.m_right) &&
           Eq(m_top,  p_other.m_top)  && Eq(m_bottom, p_other.m_bottom );
  }

  inline bool IsBeyond(const Box& p_other) const
  {
    return m_left > p_other.m_right;
  }

  // Mark for deletion
  inline void Consume() { m_flags |= 0x01; }
  inline bool IsConsumed() const { return (m_flags & 0x01) == 0x01; }

  // Simple calculations of width and height
  inline float Width()  const { return m_right - m_left  ;}
  inline float Height() const { return m_top   - m_bottom;}

  // Are two given boxes aligned on at least one side?
  inline bool SharesEdge(const Box& p_other) const
  {
    return this->m_top == p_other.m_top || this->m_bottom == p_other.m_bottom ||
           this->m_left == p_other.m_left || this->m_right == p_other.m_right;
  }

  // Is this box immediately to the right of the given box, sharing two
  // vertices? This can be used to merge boxes
  inline bool IsAdjacent(const Box& p_other) const
  {
    return m_left   == p_other.m_right  &&
           m_top    == p_other.m_top    &&
           m_bottom == p_other.m_bottom  ;
  }

  // Check whether one box partially covers another box
  inline bool Encroaches(Box& p_other)
  {
    if (&p_other == this) return true;
    return (m_left < p_other.m_right && m_right > p_other.m_left) &&
           (m_bottom < p_other.m_top && m_top > p_other.m_bottom);
  }

  // Is another box completely enclosed by this one?
  inline bool Engulfs(const Box& p_other) const
  {
    return  p_other.m_bottom - m_bottom > -0.1 && p_other.m_top - m_top < 0.1 &&
            p_other.m_left - m_left > -0.1 && p_other.m_right - m_right < 0.1 &&
            !(*this == p_other);
  }

  // The following four functions determine whether, for any given Vertex,
  // moving an arbitrarily small distance in the stated direction will put
  // us inside this box. This allows us to work out on which edges of which
  // boxes the point lies.
  inline bool IsNorthWestOf(Vertex& p_vertex) const
  {
    return m_right >= p_vertex.GetX() && m_left   <  p_vertex.GetX() &&
           m_top   >  p_vertex.GetY() && m_bottom <= p_vertex.GetY();
  }

  inline bool IsNorthEastOf(Vertex& p_vertex) const
  {
    return m_right >  p_vertex.GetX() && m_left   <= p_vertex.GetX() &&
           m_top   >  p_vertex.GetY() && m_bottom <= p_vertex.GetY();
  }

  inline bool IsSouthEastOf(Vertex& p_vertex) const
  {
    return m_right >  p_vertex.GetX() && m_left   <= p_vertex.GetX() &&
           m_top   >= p_vertex.GetY() && m_bottom <  p_vertex.GetY();
  }

  inline bool IsSouthWestOf(Vertex& p_vertex) const
  {
    return m_right >= p_vertex.GetX() && m_left   <  p_vertex.GetX() &&
           m_top   >= p_vertex.GetY() && m_bottom <  p_vertex.GetY();
  }

  // Create a vertex from a given corner of the box
  // (0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right)
  // Note, the given vertex is automatically flagged as being impinged at the
  // correct compass direction
  std::shared_ptr<Vertex> GetVertex(int corner);

  // Marks a box's impingement on a given vertex. This records whether moving
  // an arbitrarily small distance in a given direction from the vertex will
  // place one inside the current box.
  void RecordImpingementOn(Vertex& vertex);

  // Return box dimensions as a vector for output
  inline std::vector<float> vector() const
  {
    return {m_left, m_bottom, m_right, m_top};
  }

 private:
  float   m_left,
          m_right,
          m_top,
          m_bottom;
  uint8_t m_flags; // void-void-void-void-void- no left - no right - delete
};


#endif
