//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR textrow header file                                                 //
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

#ifndef PDFR_TEXTROW

//---------------------------------------------------------------------------//

#define PDFR_TEXTROW

#include "page.h"

//---------------------------------------------------------------------------//
// The textrow is a struct which acts as a "row" of information about each text
// element on a page including the actual unicode glyph(s), the position, the
// font and size of the character(s). It also contains a pair that acts as an
// address for the adjacent glyph which will be found during letter_grouper's
// construction, and Boolean flags to indicate whether it is "consumed" when
// the glyphs are stuck together into words, as well as flags to indicate
// whether the element is at the left, right or centre of a column

class textrow
{
public:
  textrow(float, float, float, float, std::string, std::vector<Unicode>);

  constexpr static float CLUMP_H = 0.01; // horizontal clumping, high = sticky
  constexpr static float CLUMP_V = 0.1; // vertical clumping, high = sticky

  inline void make_left_edge()      { m_flags |= 0x08; }
  inline void make_right_edge()     { m_flags |= 0x02; }
  inline void make_centred()        { m_flags |= 0x04; }
  inline void consume()             { m_flags |= 0x01; }
  inline void set_join(int key, int ind) { r_join = {key, ind};}
  inline void pop_last_glyph(){ glyph.pop_back();}
  inline void add_space(){glyph.push_back(0x0020);}
  inline bool is_left_edge()  const { return (m_flags & 0x08) == 0x08; }
  inline bool is_right_edge() const { return (m_flags & 0x02) == 0x02; }
  inline bool is_centred()    const { return (m_flags & 0x04) == 0x04; }
  inline bool is_consumed()   const { return (m_flags & 0x01) == 0x01; }
  inline float get_left()     const { return this->left;}
  inline float get_top()      const { return this->bottom + this->size;}
  inline float get_right()    const { return this->right;}
  inline float get_bottom()   const { return this->bottom;}
  inline float get_size()     const { return this->size;}
  inline int grid_num()       const { return this->r_join.first;}
  inline int vec_num()        const { return this->r_join.second;}
  inline bool no_join()       const { return this-> r_join.first == -1;}
  inline std::vector<Unicode> get_glyph() const { return this->glyph;}
  inline std::string get_font() const { return this->font;}


  inline bool operator ==(const textrow& a) const
  {
    return (a.left == this->left && a.bottom == this->bottom &&
            a.size == this->size && a.glyph  == this->glyph);
  }

  inline bool is_adjoining_letter(const textrow& cell) const
  {
       return (cell.left > this->left &&
        abs(cell.bottom - this->bottom) < (CLUMP_V * this->size) &&
        (abs(cell.left  -  this->right ) < (CLUMP_H * this->size) ||
        (cell.left < this->right)) );
  }

  void merge_letters(textrow&);
  bool is_elligible_to_join(const textrow&) const;
  void join_words(textrow&);
  void concat_glyph(const std::vector<Unicode>&);


private:
  float       left,             // position of left edge of text on page
              right,            // position of right edge of text on page
              bottom,           // y position of bottom edge of text
              size;             // point size of font used to draw text
  std::string font;             // Name of font used to draw text
  std::vector<Unicode> glyph;   // The actual Unicode glyphs encoded
  std::pair<int, int> r_join;   // address of closest adjacent element
  uint8_t m_flags;
};

//---------------------------------------------------------------------------//

typedef std::shared_ptr<textrow> text_ptr;

//---------------------------------------------------------------------------//
// This struct is a container for the output of the parser class. All
// of the vectors are the same length, so it can be thought of as a table with
// one row for each glyph on the page. This makes it straightforward to output
// to other formats if needed.

struct GSoutput
{
  std::vector<std::vector<Unicode>> text;      // vector of unicode code points
  std::vector<float> left;        // vector of glyphs' left edges
  std::vector<float> bottom;      // vector of glyphs' bottom edges
  std::vector<float> right;       // vector of glyphs' right edges
  std::vector<std::string> fonts; // vector of glyphs' font names
  std::vector<float> size;        // vector of glyphs' point size
  std::vector<float> minbox;

  std::vector<textrow> transpose()
  {
    std::vector<textrow> res;
    if(!left.empty())
      for(size_t i = 0; i < left.size(); ++i)
        res.emplace_back(textrow(left[i], right[i], bottom[i],
                                 size[i], fonts[i], text[i]));
    return res;
  }
};

//---------------------------------------------------------------------------//
// The textrows struct will be the main data repository for our output.
// It is essentially a vector of textrows which also houses the page dimensions
// as a seperate vector. To make it easy to work with, it contains functions
// that allow us to use it as if it was just a vector of textrows. This allows
// for easy iteration.

struct textrows
{
  // Standard constructor - takes vector of textrow pointers and the minbox
  textrows(std::vector<text_ptr> t, std::vector<float> m):
    m_data(t), minbox(m) {}

  // Copy contructor
  textrows(const textrows& t):
    m_data(t.m_data), minbox(t.minbox) {}

  // Move constructor
  textrows(textrows&& t) noexcept :
    m_data(std::move(t.m_data)), minbox(std::move(t.minbox)) {}

  // Rvalue assignment constructor
  textrows& operator=(textrows&& t) noexcept
  {
    std::swap(this->m_data, t.m_data);
    std::swap(this->minbox, t.minbox);
    return *this;
  }

  // Lvalue assignment constructor
  textrows& operator=(const textrows& t)
  {
    this->m_data = t.m_data;
    this->minbox = t.minbox;
    return *this;
  }

  // Default constructor
  textrows() = default;

  // Functions to copy the methods of vectors to access main data object
  inline std::vector<text_ptr>::iterator begin(){return m_data.begin();}
  inline std::vector<text_ptr>::iterator end(){return m_data.end();}
  inline text_ptr& operator[](int n){return m_data[n];}
  inline void push_back(text_ptr t){m_data.push_back(t);}
  inline size_t size(){return m_data.size();}
  inline bool empty(){return m_data.empty();}

  // Converts textrows to GSoutput
  GSoutput transpose()
  {
    GSoutput res;
    res.minbox = this->minbox;
    for(auto i : this->m_data)
    {
      if(!i->is_consumed())
      {
        res.text.push_back(i->get_glyph());
        res.left.push_back(i->get_left());
        res.bottom.push_back(i->get_bottom());
        res.right.push_back(i->get_right());
        res.fonts.push_back(i->get_font());
        res.size.push_back(i->get_size());
      }
    }
    return res;
  }

  // The data members
  std::vector<text_ptr> m_data;
  std::vector<float> minbox;
};



#endif
