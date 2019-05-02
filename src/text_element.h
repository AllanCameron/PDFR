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

class text_element : public Box
{

typedef std::shared_ptr<text_element> text_ptr;

public:
  text_element(float, float, float, float,
               std::shared_ptr<font>, std::vector<Unicode>);

  // Inevitably, we need to define some "magic number" constants to define
  // how close together text elements have to be to clump together

  constexpr static float CLUMP_H = 0.01; // horizontal clumping, high = sticky
  constexpr static float CLUMP_V = 0.1;  // vertical clumping, high = sticky
  constexpr static float LINE_CLUMP = 0.7;
  constexpr static float MAX_WORD_GAP = 2.0;
  constexpr static float MAX_ALIGN_IGNORE = 0.41;

  inline void make_left_edge()  { this->set_flag(0x08); }
  inline void make_right_edge() { this->set_flag(0x02); }
  inline void make_centred()    { this->set_flag(0x04); }

  inline bool is_left_edge()  const { return this->has_flag(0x08); }
  inline bool is_right_edge() const { return this->has_flag(0x02); }
  inline bool is_centred()    const { return this->has_flag(0x04); }

  inline void set_join(text_ptr element) { this->m_join = element;}
  inline text_ptr get_join()             { return this->m_join; }
  inline bool has_join() const { if(m_join) return true; else return false;}

  inline std::string get_font() const { return this->m_font->fontname();}
  inline std::vector<Unicode> get_glyph() const { return this->glyph;}
  inline void add_space() { glyph.push_back(0x0020);         }

  inline void pop_last_glyph()
  {
    if (glyph.empty()) throw std::runtime_error("Can't pop empty glyph vector");
    else glyph.pop_back();
  }

  inline bool operator ==(const text_element& a) const
  {
    return (a.get_left()   == this->get_left()    &&
            a.get_bottom() == this->get_bottom()  &&
            a.get_top()    == this->get_top()     &&
            a.get_glyph()  == this->get_glyph()   );
  }

  inline bool is_adjoining_letter(const text_element& other) const
  {
    return
      other.get_left() > get_left() &&
      abs(other.get_bottom() - get_bottom()) < (CLUMP_V * get_size()) &&
      (
        abs(other.get_left() - get_right()) < (CLUMP_H * get_size()) ||
        (other.get_left() < get_right())
      ) ;
  }

  inline bool is_on_same_line_as(const text_element& other) const
  {
    return
    (other.get_bottom() - this->get_bottom() < LINE_CLUMP * this->get_size()) &&
    (this->get_bottom() - other.get_bottom() < LINE_CLUMP * this->get_size());
  }

  inline bool is_way_beyond(const text_element& other) const
  {
    return get_left() - other.get_right() > MAX_WORD_GAP * other.get_size();
  }

  inline bool cannot_join_left_of(const text_element& other) const
  {
    return
    ((other.is_left_edge()  || other.is_centred())  &&
    (other.get_left() - get_right() > MAX_ALIGN_IGNORE * get_size())) ||
    ((this->is_right_edge() || this->is_centred())   &&
    (other.get_left() - get_right() > MAX_ALIGN_IGNORE * this->get_size()));
  }

  void merge_letters(text_element&);
  bool is_elligible_to_join(const text_element&) const;
  void join_words(text_element&);
  void concat_glyph(const std::vector<Unicode>&);


private:
  std::shared_ptr<font> m_font;         // Name of font used to draw text
  std::vector<Unicode> glyph;           // The actual Unicode glyphs encoded
  std::shared_ptr<text_element> m_join; // address of closest adjacent element
};

//---------------------------------------------------------------------------//

typedef std::shared_ptr<text_element> text_ptr;

//---------------------------------------------------------------------------//
// The textbox will be the main data repository for our output. It inherits from
// Box and contains a vector of text_elements. To make it easy to work with, it
// contains functions that allow us to use it as if it was just a vector of
// text_elements. This allows for easy iteration.

class textbox : public Box
{
public:
  // Standard constructor - takes vector of text_element pointers and the minbox
  textbox(std::vector<text_ptr> t, Box m):  Box(m), m_data(t) {}
  textbox(std::vector<text_ptr> t, std::vector<float> b): Box(b), m_data(t) {}
  textbox(std::vector<text_ptr> t, float a, float b, float c, float d):
  Box(a, b, c, d), m_data(t) {}
  textbox(Box m):  Box(m) {}
  textbox() = default;

  // Copy contructor
  textbox(const textbox& t) = default;

  // Lvalue assignment constructor
  textbox& operator=(const textbox& t) = default;

  typedef std::vector<text_ptr>::iterator textbox_iterator;
  typedef std::vector<text_ptr>::const_iterator textbox_const_iterator;

  // Functions to copy the methods of vectors to access main data object
  inline textbox_iterator begin() {return m_data.begin(); }
  inline textbox_iterator end()   {return m_data.end(); }
  inline textbox_const_iterator cbegin() const {return m_data.cbegin(); }
  inline textbox_const_iterator cend() const {return m_data.cend(); }
  inline text_ptr& operator[](int n) { return m_data[n]; }
  inline text_ptr front() const {return m_data.front(); }
  inline text_ptr back() const { return m_data.back(); }
  inline size_t size() const { return m_data.size(); }
  inline bool empty() const { return m_data.empty(); }
  inline void push_back(text_ptr t) { m_data.push_back(t); }
  inline void clear() { m_data.clear(); }
  inline void resize(int a) { m_data.resize(a); }
  inline void swap_data(std::vector<text_ptr>& other){std::swap(m_data, other);}
  void remove_duplicates();

private:
  // The data member
  std::vector<text_ptr> m_data;
};

//---------------------------------------------------------------------------//
// This struct inherits from Box, and is created by feeding it a textbox. It
// converts the vector of text_elements (which is conceptually a vector of
// data frame rows) into columns of the different data types.

struct text_table: public Box
{
  text_table(const textbox&);
  std::vector<std::vector<Unicode>> text;      // vector of unicode code points
  std::vector<float> left, right, bottom, top; // vectors of glyphs' positions
  std::vector<std::string> fonts;              // vector of glyphs' font names
  std::vector<float> get_size();
  void join(text_table& other);
};

#endif
