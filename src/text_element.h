//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR TextElement header file                                             //
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
// The "atom" of our output will be the TextElement. This is a class containing
// one or more glyphs as a vector of uint16_t (representing Unicode code points)
// along with its position, size, and the name of the font used to draw it.
// We will need to shuffle these around quite a lot in processing, so we use
// shared pointers to each TextElement to represent each text element. The
// pointers to text_elements are typedef'd as TextPointer for brevity.
//
// We need to be able to process groups of text_elements together; for this we
// could just use a vector of TextPointer. However, we often need to know the
// bounding box of a group of text_elements. We can therefore define a TextBox
// as a struct with a Box and a vector of text_elements.
//
// This header file contains the definitions of the TextElement, TextPointer and
// TextBox classes. Most of their methods are straightforward and inlined, but
// some of the more involved methods are described in text_element.cpp

//---------------------------------------------------------------------------//
// The TextElement is a struct which contains information about each text
// element on a page including the actual unicode glyph(s), the position, the
// font and size of the character(s). It also contains a pair that acts as an
// address for the adjacent glyph which will be found during letter_grouper's
// construction, and Boolean flags to indicate whether it is "consumed" when
// the glyphs are stuck together into words, as well as flags to indicate
// whether the element is at the left, right or centre of a column

class TextElement : public Box
{

typedef std::shared_ptr<TextElement> TextPointer;

public:
  TextElement(float t_left, float t_right, float t_top, float t_bottom,
              std::shared_ptr<Font> t_font, std::vector<Unicode> t_glyphs):
    Box(t_left, t_right, t_top, t_bottom),
    font_(t_font), glyph_(t_glyphs), join_(nullptr) {};

  // Inevitably, we need to define some "magic number" constants to define
  // how close together text elements have to be to clump together

  constexpr static float CLUMP_H = 0.01; // horizontal clumping, high = sticky
  constexpr static float CLUMP_V = 0.1;  // vertical clumping, high = sticky
  constexpr static float LINE_CLUMP = 0.7;
  constexpr static float MAX_WORD_GAP = 2.0;
  constexpr static float MAX_ALIGN_IGNORE = 0.41;

  inline void MakeLeftEdge()  { this->SetFlag(0x08); }
  inline void MakeRightEdge() { this->SetFlag(0x02); }
  inline void MakeCentred()   { this->SetFlag(0x04); }

  inline bool IsLeftEdge()  const { return this->HasFlag(0x08); }
  inline bool IsRightEdge() const { return this->HasFlag(0x02); }
  inline bool IsCentred()   const { return this->HasFlag(0x04); }

  inline void SetJoin(TextPointer element) { this->join_ = element;}
  inline TextPointer GetJoin()             { return this->join_; }
  inline bool HasJoin() const { if(join_) return true; else return false;}

  inline std::string GetFontName() const { return this->font_->fontname();}
  inline std::vector<Unicode> GetGlyph() const { return this->glyph_;}
  inline void AddSpace() { glyph_.push_back(0x0020);         }

  inline void PopLastGlyph()
  {
    if (glyph_.empty()) throw std::runtime_error("Can't pop empty vector");
    else glyph_.pop_back();
  }

  inline bool operator ==(const TextElement& t_other) const
  {
    return (t_other.GetLeft()   == this->GetLeft()    &&
            t_other.GetBottom() == this->GetBottom()  &&
            t_other.GetTop()    == this->GetTop()     &&
            t_other.GetGlyph()  == this->GetGlyph()   );
  }

  inline bool IsAdjoiningLetter(const TextElement& t_other) const
  {
    return
      t_other.GetLeft() > GetLeft() &&
      abs(t_other.GetBottom() - GetBottom()) < (CLUMP_V * GetSize()) &&
      (
        abs(t_other.GetLeft() - GetRight()) < (CLUMP_H * GetSize()) ||
        (t_other.GetLeft() < GetRight())
      ) ;
  }

  inline bool IsOnSameLineAs(const TextElement& t_other) const
  {
    return
    (t_other.GetBottom() - this->GetBottom() < LINE_CLUMP * this->GetSize()) &&
    (this->GetBottom() - t_other.GetBottom() < LINE_CLUMP * this->GetSize());
  }

  inline bool IsWayBeyond(const TextElement& t_other) const
  {
    return GetLeft() - t_other.GetRight() > MAX_WORD_GAP * t_other.GetSize();
  }

  inline bool CannotJoinLeftOf(const TextElement& t_other) const
  {
    return
    ((t_other.IsLeftEdge()  || t_other.IsCentred())  &&
    (t_other.GetLeft() - GetRight() > MAX_ALIGN_IGNORE * GetSize())) ||
    ((this->IsRightEdge() || this->IsCentred())   &&
    (t_other.GetLeft() - GetRight() > MAX_ALIGN_IGNORE * this->GetSize()));
  }

  void MergeLetters(TextElement&);
  bool IsElligibleToJoin(const TextElement&) const;
  void JoinWords(TextElement&);
  void ConcatenateUnicode(const std::vector<Unicode>&);
  std::string Utf();


private:
  std::shared_ptr<Font> font_;           // Font used to draw text
  std::vector<Unicode> glyph_;           // The actual Unicode glyphs encoded
  std::shared_ptr<TextElement> join_;    // address of closest adjacent element
};

//---------------------------------------------------------------------------//

typedef std::shared_ptr<TextElement> TextPointer;

//---------------------------------------------------------------------------//
// The TextBox will be the main data repository for our output. It inherits from
// Box and contains a vector of text_elements. To make it easy to work with, it
// contains functions that allow us to use it as if it was just a vector of
// text_elements. This allows for easy iteration.

class TextBox : public Box
{
public:
  // Standard constructor - takes vector of TextElement pointers and the minbox
  TextBox(std::vector<TextPointer> t_text, Box t_box):
  Box(t_box), data_(t_text) {}

  TextBox(std::vector<TextPointer> t_text, std::vector<float> t_vector):
  Box(t_vector), data_(t_text) {}

  TextBox(std::vector<TextPointer> t_text, float a, float b, float c, float d):
  Box(a, b, c, d), data_(t_text) {}

  TextBox(Box t_box):  Box(t_box) {}

  TextBox() = default;

  // Copy contructor
  TextBox(const TextBox& t_textbox) = default;

  // Lvalue assignment constructor
  TextBox& operator=(const TextBox& t_textbox) = default;

  typedef std::vector<TextPointer>::iterator TextBoxIterator;
  typedef std::vector<TextPointer>::const_iterator TextBoxConstIterator;

  // Functions to copy the methods of vectors to access main data object
  inline TextBoxIterator begin() {return data_.begin(); }
  inline TextBoxIterator end()   {return data_.end(); }
  inline TextBoxConstIterator cbegin() const {return data_.cbegin(); }
  inline TextBoxConstIterator cend() const {return data_.cend(); }
  inline TextPointer& operator[](int t_index) { return data_[t_index]; }
  inline TextPointer front() const {return data_.front(); }
  inline TextPointer back() const { return data_.back(); }
  inline size_t size() const { return data_.size(); }
  inline bool empty() const { return data_.empty(); }
  inline void push_back(TextPointer t_text_ptr) { data_.push_back(t_text_ptr);}
  inline void clear() { data_.clear(); }
  inline void resize(int t_new_size) { data_.resize(t_new_size); }
  void RemoveDuplicates();
  inline void SwapData(std::vector<TextPointer>& t_other)
  {
    std::swap(data_, t_other);
  }

private:
  // The data member
  std::vector<TextPointer> data_;
};

//---------------------------------------------------------------------------//
// This struct inherits from Box, and is created by feeding it a TextBox. It
// converts the vector of text_elements (which is conceptually a vector of
// data frame rows) into columns of the different data types.

struct TextTable: public Box
{
 public:
  TextTable(const TextBox&);
  void Join(TextTable&);
  inline std::vector<float>       GetLefts()     const { return this->lefts_;  }
  inline std::vector<float>       GetRights()    const { return this->rights_; }
  inline std::vector<float>       GetTops()      const { return this->tops_;   }
  inline std::vector<float>       GetBottoms()   const { return this->bottoms_;}
  inline std::vector<float>       GetSizes()     const { return this->sizes_;  }
  inline std::vector<std::string> GetFontNames() const { return this->fonts_;  }
  inline std::vector<std::string> GetText()      const { return this->text_;   }

 private:
  std::vector<std::string> text_, fonts_;
  std::vector<float> lefts_, rights_, bottoms_, tops_, sizes_;
};

#endif
