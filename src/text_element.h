//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR TextElement header file                                             //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_TEXT_ELEMENT

//---------------------------------------------------------------------------//

#define PDFR_TEXT_ELEMENT

#include<string>
#include "box.h"

class Font;
using Unicode = uint16_t;

//---------------------------------------------------------------------------//
// The "atom" of our output will be the TextElement. This is a class containing
// one or more glyphs as a vector of uint16_t (representing Unicode code points)
// along with its position, size, and the name of the font used to draw it.
// We will need to shuffle these around quite a lot in processing, so we use
// shared pointers to each TextElement to represent each text element. The
// pointers to text_elements are typedef'd as TextPointer for brevity.

//---------------------------------------------------------------------------//
// The TextElement is a class which contains information about each text
// element on a page including the actual unicode glyph(s), the position, the
// font and size of the character(s). It also contains a pair that acts as an
// address for the adjacent glyph which will be found during LetterGrouper's
// construction, and Boolean flags to indicate whether it is "consumed" when
// the glyphs are stuck together into words, as well as flags to indicate
// whether the element is at the left, right or centre of a column

class TextElement : public Box
{
  typedef std::shared_ptr<TextElement> TextPointer;

 public:
  TextElement(float t_left, float t_right, float t_top, float t_bottom,
              float t_size, std::shared_ptr<Font> t_font,
              std::vector<Unicode> t_glyphs)
    : Box(t_left, t_right, t_top, t_bottom), size_(t_size),
      font_(t_font), glyph_(t_glyphs), join_(nullptr) {};

  // Inevitably, we need to define some "magic number" constants to define
  // how close together text elements have to be to clump together

  constexpr static float CLUMP_H = 0.01; // horizontal clumping, high = sticky
  constexpr static float CLUMP_V = 0.1;  // vertical clumping, high = sticky
  constexpr static float LINE_CLUMP = 0.7;
  constexpr static float MAX_WORD_GAP = 0.5;
  constexpr static float MAX_ALIGN_IGNORE = 0.0;

  inline void MakeLeftEdge()  { this->SetFlag(0x04); }
  inline void MakeRightEdge() { this->SetFlag(0x02); }
  inline void MakeCentred()   { this->SetFlag(0x06); }
  inline float GetSize() const override {return this->size_;}
  inline bool IsLeftEdge()  const { return this->HasFlag(0x04); }
  inline bool IsRightEdge() const { return this->HasFlag(0x02); }
  inline bool IsCentred()   const { return this->HasFlag(0x06); }

  inline void SetJoin(TextPointer element) { this->join_ = element;}
  inline TextPointer GetJoin()             { return this->join_; }
  inline bool HasJoin() const { if (join_) return true; else return false;}

  std::string GetFontName() const; // can't inline without including font.h
  inline std::vector<Unicode> GetGlyph() const { return this->glyph_;}
  inline void AddSpace() { glyph_.push_back(0x0020);         }

  inline void PopLastGlyph()
  {
    if (glyph_.empty()) throw std::runtime_error("Can't pop empty vector");
    else glyph_.pop_back();
  }

  inline bool operator ==(const TextElement& t_other) const
  {
    if (&t_other == this) return true;
    return (t_other.GetLeft()   == this->GetLeft()    &&
            t_other.GetBottom() == this->GetBottom()  &&
            t_other.GetTop()    == this->GetTop()     &&
            t_other.GetGlyph()  == this->GetGlyph()   );
  }

  inline bool IsAdjoiningLetter(const TextElement& t_other) const
  {
    if (&t_other == this) return false;
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
    if (&t_other == this) return true;
    return
    (t_other.GetBottom() - this->GetBottom() < LINE_CLUMP * this->GetSize()) &&
    (this->GetBottom() - t_other.GetBottom() < LINE_CLUMP * this->GetSize());
  }

  inline bool IsWayBeyond(const TextElement& t_other) const
  {
    if (&t_other == this) return false;
    return GetLeft() - t_other.GetRight() > MAX_WORD_GAP * t_other.GetSize();
  }

  inline bool CannotJoinLeftOf(const TextElement& t_other) const
  {
    if (&t_other == this) return true;
    return
    ( t_other.IsLeftEdge()  || t_other.IsCentred()  ||
      this->IsRightEdge()   || this->IsCentred())   &&
    (t_other.GetLeft() - this->GetRight()) > (MAX_ALIGN_IGNORE * GetSize());
  }

  void MergeLetters(TextElement&);
  bool IsElligibleToJoin(const TextElement&) const;
  void JoinWords(TextElement&);
  void ConcatenateUnicode(const std::vector<Unicode>&);
  std::string Utf();


 private:
  float size_;                           // The font size
  std::shared_ptr<Font> font_;           // Font used to draw text
  std::vector<Unicode> glyph_;           // The actual Unicode glyphs encoded
  std::shared_ptr<TextElement> join_;    // address of closest adjacent element
};


#endif
