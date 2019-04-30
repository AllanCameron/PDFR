//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR text_element header file                                            //
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

#ifndef PDFR_TEXT_ELEMENT

//---------------------------------------------------------------------------//

#define PDFR_TEXT_ELEMENT

#include "page.h"

//---------------------------------------------------------------------------//
// The "atom" of our output will be the text_element. This is a class containing
// one or more glyphs as a vector of uint16_t (representing Unicode code points)
// along with its position, size, and the name of the font used to draw it.
// We will need to shuffle these around quite a lot in processing, so we use
// shared pointers to each text_element to represent each text element. The
// pointers to text_elements are typedef'd as text_ptr for brevity.
//
// We need to be able to process groups of text_elements together; for this we
// could just use a vector of text_ptr. However, we often need to know the
// bounding box of a group of text_elements. We can therefore define a textbox
// as a struct with a Box and a vector of text_elements.
//
// This header file contains the definitions of the text_element, text_ptr and
// textbox classes. Most of their methods are straightforward and inlined, but
// some of the more involved methods are described in text_element.cpp

//---------------------------------------------------------------------------//
// The text_element is a struct which contains information about each text
// element on a page including the actual unicode glyph(s), the position, the
// font and size of the character(s). It also contains a pair that acts as an
// address for the adjacent glyph which will be found during letter_grouper's
// construction, and Boolean flags to indicate whether it is "consumed" when
// the glyphs are stuck together into words, as well as flags to indicate
// whether the element is at the left, right or centre of a column

class text_element
{
public:
  text_element(float, float, float, float, std::string, std::vector<Unicode>);

  constexpr static float CLUMP_H = 0.01; // horizontal clumping, high = sticky
  constexpr static float CLUMP_V = 0.1;  // vertical clumping, high = sticky

  inline void make_left_edge()      { m_flags |= 0x08; }
  inline void make_right_edge()     { m_flags |= 0x02; }
  inline void make_centred()        { m_flags |= 0x04; }
  inline void consume()             { m_flags |= 0x01; }
  inline void set_join(int key, int ind) { r_join = {key, ind};}
  inline void add_space(){glyph.push_back(0x0020);}
  inline bool is_left_edge()  const { return (m_flags & 0x08) == 0x08; }
  inline bool is_right_edge() const { return (m_flags & 0x02) == 0x02; }
  inline bool is_centred()    const { return (m_flags & 0x04) == 0x04; }
  inline bool is_consumed()   const { return (m_flags & 0x01) == 0x01; }
  inline float get_left()     const { return this->left;}
  inline float get_right()    const { return this->right;}
  inline float get_bottom()   const { return this->bottom;}
  inline float get_top()      const { return this->top;}
  inline float get_size()     const { return this->top - this->bottom;}
  inline int grid_num()       const { return this->r_join.first;}
  inline int vec_num()        const { return this->r_join.second;}
  inline bool no_join()       const { return this-> r_join.first == -1;}
  inline std::vector<Unicode> get_glyph() const { return this->glyph;}
  inline std::string get_font() const { return this->font;}

  inline void pop_last_glyph()
  {
    if(glyph.empty()) throw std::runtime_error("Can't pop empty glyph vector");
    glyph.pop_back();
  }

  inline bool operator ==(const text_element& a) const
  {
    return (a.left == this->left && a.bottom == this->bottom &&
            a.top == this->top && a.glyph  == this->glyph);
  }

  inline bool is_adjoining_letter(const text_element& cell) const
  {
       return (cell.left > this->left &&
        abs(cell.bottom - this->bottom) < (CLUMP_V * this->get_size()) &&
        (abs(cell.left  -  this->right ) < (CLUMP_H * this->get_size()) ||
        (cell.left < this->right)) );
  }

  void merge_letters(text_element&);
  bool is_elligible_to_join(const text_element&) const;
  void join_words(text_element&);
  void concat_glyph(const std::vector<Unicode>&);


private:

  float       left,             // position of left edge of text on page
              right,            // position of right edge of text on page
              bottom,           // y position of bottom edge of text
              top;              // top edge of glyph as y co-ordinate
  std::string font;             // Name of font used to draw text
  std::vector<Unicode> glyph;   // The actual Unicode glyphs encoded
  std::pair<int, int> r_join;   // address of closest adjacent element
  uint8_t m_flags;
};

//---------------------------------------------------------------------------//

typedef std::shared_ptr<text_element> text_ptr;

//---------------------------------------------------------------------------//
// This struct is a container for the output of the parser class. All
// of the vectors are the same length, so it can be thought of as a table with
// one row for each glyph on the page. This makes it straightforward to output
// to other formats if needed.

struct text_table
{
  std::vector<std::vector<Unicode>> text; // vector of unicode code points
  std::vector<float> left;        // vector of glyphs' left edges
  std::vector<float> bottom;      // vector of glyphs' bottom edges
  std::vector<float> right;       // vector of glyphs' right edges
  std::vector<std::string> fonts; // vector of glyphs' font names
  std::vector<float> top;         // vector of glyphs' top edge
  Box m_box;

  std::vector<float> get_size()
  {
    std::vector<float> res;
    if(bottom.size() != top.size() || bottom.empty()) return res;
    for(size_t i = 0; i < bottom.size(); ++i) res.push_back(bottom[i] + top[i]);
    return res;
  }

  std::vector<text_element> transpose()
  {
    std::vector<text_element> res;
    if(!left.empty())
      for(size_t i = 0; i < left.size(); ++i)
        res.emplace_back(text_element(left[i], right[i], bottom[i],
                                      top[i], fonts[i], text[i]));
    return res;
  }

  void join(const text_table& other)
  {
    m_box.merge(other.m_box);
    concat(this->text, other.text);
    concat(this->left, other.left);
    concat(this->bottom, other.bottom);
    concat(this->right, other.right);
    concat(this->fonts, other.fonts);
    concat(this->top, other.top);
  }
};

//---------------------------------------------------------------------------//
// The textbox struct will be the main data repository for our output.
// It is essentially a vector of text_elements which also houses the containing box
// as a seperate member. To make it easy to work with, it contains functions
// that allow us to use it as if it was just a vector of text_elements. This allows
// for easy iteration.

class textbox : public Box
{
public:
  // Standard constructor - takes vector of text_element pointers and the minbox
  textbox(std::vector<text_ptr> t, Box m):  Box(m), m_data(t) {}

  textbox(std::vector<text_ptr> t, std::vector<float> b): Box(b), m_data(t) {}

  textbox(std::vector<text_ptr> t, float a, float b, float c, float d):
  Box(a, b, c, d), m_data(t) {}

  textbox(Box m):  Box(m) {}

  // Copy contructor
  textbox(const textbox& t) = default;

  // Lvalue assignment constructor
  textbox& operator=(const textbox& t) = default;

  // Default constructor
  textbox() = default;

  // Functions to copy the methods of vectors to access main data object
  inline std::vector<text_ptr>::iterator begin(){return m_data.begin();}
  inline std::vector<text_ptr>::iterator end(){return m_data.end();}
  inline text_ptr& operator[](int n){return m_data[n];}
  inline text_ptr front() const {return m_data.front();}
  inline text_ptr back() const { return m_data.back();}
  inline size_t size() const {return m_data.size();}
  inline bool empty(){return m_data.empty();}
  inline void push_back(text_ptr t){m_data.push_back(t);}
  inline void clear(){ m_data.clear();}
  inline void resize(int a){ m_data.resize(a);}
  inline void swap_data(std::vector<text_ptr>& other){std::swap(m_data, other);}

  // Converts textbox to text_table
  text_table transpose()
  {
    text_table res;
    res.m_box = *this;
    this->remove_duplicates();
    for (auto i : this->m_data)
    {
      if(!i->is_consumed())
      {
        res.text.push_back(i->get_glyph());
        res.left.push_back(i->get_left());
        res.bottom.push_back(i->get_bottom());
        res.right.push_back(i->get_right());
        res.fonts.push_back(i->get_font());
        res.top.push_back(i->get_top());
      }
    }
    return res;
  }

  void remove_duplicates()
  {
    for (auto this_row = m_data.begin(); this_row != m_data.end(); ++this_row)
    {
      if ((*this_row)->is_consumed()) continue;
      for (auto other_row = this_row; other_row != m_data.end(); ++other_row)
      {
        if(other_row == this_row) continue;

        if (**other_row == **this_row)
        {
          (*other_row)->consume();
        }
      }
    }
  }

private:
  // The data members
  std::vector<text_ptr> m_data;
};



#endif
