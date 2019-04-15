//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR parser header file                                                  //
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

#ifndef PDFR_GS

//---------------------------------------------------------------------------//

#define PDFR_GS

/* The job of the parser class is to parse the pdf page description
 * language into a table of glyphs, positions, sizes and fontnames - one row
 * for each character on the page. The instructions from the page description
 * language have already been "compiled" by the lexer into an instruction set,
 * but we now need to interpret and enact those instructions.
 *
 * Conceptually, this is done using operators and operands. This is actually
 * made a bit easier by the grammar of the page description language, which
 * operates on a stack system - most tokens are operands, and are loaded onto
 * the stack until an operator is reached. When the operator is reached, it
 * performs an action on the operands then clears the stack.
 *
 * In order that parser can interpret the operands, it needs to know
 * about the fonts on the page, the content string, and any xobjects that
 * are called to be inserted on the page. It therefore needs to use the page's
 * public interface to get these data, and in fact is created by giving the
 * constructor a pointer to a page. It takes the content string, sends it to
 * the tokenizer to compile the instructions, then uses an instruction reader
 * which loads the stack with operands and calls the appropriate function when
 * it reaches an operator, until the instructions are finished.
 *
 * The functions that the instruction reader calls take up most of the code
 * in the implementation. They do work on the operands to change the global
 * graphics state, set fonts, write characters, handle kerning etc. There are
 * a number of private data members which maintain state between loops of the
 * instruction reader, and some which record the entire history of the state.
 *
 * The final output of parser is a collection of vectors, all of the same
 * length, comprising the Unicode symbol, width, font size, font name and x/y
 * position of every character on the page. This is output as a specific struct
 * to reduce the passing around of several parameters.
 */

#include "page.h"
#include<functional>
#include<iostream>

//---------------------------------------------------------------------------//
// The states of the lexer are defined by this enum. It is defined in its own
// namespace rather than within the class because its states are also used
// as type labels in the instruction set it produces. These therefore need
// to be made available to the instruction reader in the parser class

namespace Token
{
  enum TState
  {
    NEWSYMBOL,  IDENTIFIER, NUMBER, RESOURCE, STRING,
    HEXSTRING,  ARRAY,      DICT,   WAIT,     OPERATOR
  };
};

//---------------------------------------------------------------------------//
// The textrow is a struct which acts as a "row" of information about each text
// element on a page including the actual unicode glyph(s), the position, the
// font and size of the character(s). It also contains a pair that acts as an
// address for the adjacent glyph which will be found during letter_grouper's
// construction, and Boolean flags to indicate whether it is "consumed" when
// the glyphs are stuck together into words, as well as flags to indicate
// whether the element is at the left, right or centre of a column

struct textrow
{
  float       left,             // position of left edge of text on page
              right,            // position of right edge of text on page
              bottom,           // y position of bottom edge of text
              size;             // point size of font used to draw text
  std::string font;             // Name of font used to draw text
  std::vector<Unicode> glyph;   // The actual Unicode glyphs encoded
  std::pair<int, int> r_join;   // address of closest adjacent element
  uint8_t flags;

  constexpr static float CLUMP_H = 0.1; // horizontal clumping, high = sticky
  constexpr static float CLUMP_V = 0.1; // vertical clumping, high = sticky

  inline void make_left_edge()      { flags |= 0x08; }
  inline void make_right_edge()     { flags |= 0x02; }
  inline void make_centred()        { flags |= 0x04; }
  inline void consume()             { flags |= 0x01; }
  inline bool is_left_edge()  const { return (flags & 0x08) == 0x08; }
  inline bool is_right_edge() const { return (flags & 0x02) == 0x02; }
  inline bool is_centred()    const { return (flags & 0x04) == 0x04; }
  inline bool is_consumed()   const { return (flags & 0x01) == 0x01; }

  textrow(float l, float r, float b, float s,
          std::string f, std::vector<Unicode> g): left(l), right(r),
          bottom(b), size(s), font(f), glyph(g),
          r_join(std::make_pair(-1, -1)), flags(0) {};

  bool operator ==(const textrow& a) const
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

  void merge_letters(textrow& matcher)
  {
     // paste the left glyph to the right glyph
    concat(this->glyph, matcher.glyph);

    // make the right glyph now contain both glyphs
    matcher.glyph = this->glyph;

    // make the right glyph now start where the left glyph started
    matcher.left = this->left;

    // Ensure bottom is the lowest value of the two glyphs
    if(this->bottom < matcher.bottom) matcher.bottom = this->bottom;

    // The checked glyph is now consumed - move to the next
    this->consume();
  }

  bool is_elligible_to_join(const textrow& j) const
  {
    if(j.is_consumed() ||
       (j.left < this->right) ||
       (j.bottom - this->bottom > 0.7 * this->size) ||
       (this->bottom - j.bottom > 0.7 * this->size) ||
       (j.left - this->right > 2 * this->size) ||
        ((j.is_left_edge()  || j.is_centred())           &&
        (j.left - this->right > 0.51 * this->size)) ||
       ((this->is_right_edge() || this->is_centred())   &&
        (j.left - this->right > 0.51 * this->size))  )  return false;
    else return true;
  }

  void join_words(textrow& j)
  {
      // This element is elligible for joining - start by adding a space to it
      this->glyph.push_back(0x0020);

      // If the gap is wide enough, add two spaces
      if(j.left - this->right > 1 * this->size) this->glyph.push_back(0x0020);

      // Stick contents together
      concat(this->glyph, j.glyph);

      // The rightmost glyph's right edge properties are also copied over
      this->right = j.right;
      if(j.is_right_edge()) this->make_right_edge();

      // The word will take up the size of its largest glyph
      this->size = std::max(this->size, j.size);

      // The element on the right is now consumed
      j.consume();
  }

};

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
    m_data(t), minbox(m), data_size(t.size()) {}

  // Copy contructor
  textrows(const textrows& t):
    m_data(t.m_data), minbox(t.minbox), data_size(t.m_data.size()) {}

  // Move constructor
  textrows(textrows&& t) noexcept :
    m_data(std::move(t.m_data)), minbox(std::move(t.minbox)),
    data_size(m_data.size()) {}

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
  inline void push_back(text_ptr t){m_data.push_back(t); data_size++;}
  inline size_t size(){return data_size;}
  inline bool empty(){return data_size == 0;}

  // Converts textrows to GSoutput
  GSoutput transpose(){
    GSoutput res;
    res.minbox = this->minbox;
    for(auto i : this->m_data)
    {
      if(!i->is_consumed())
      {
        res.text.push_back(i->glyph);
        res.left.push_back(i->left);
        res.bottom.push_back(i->bottom);
        res.right.push_back(i->right);
        res.fonts.push_back(i->font);
        res.size.push_back(i->size);
      }
    }
    return res;
  }

  // The data members
  std::vector<text_ptr> m_data;
  std::vector<float> minbox;
  size_t data_size;
};

//---------------------------------------------------------------------------//

class parser
{
public:
  // Basic constructor
  parser(std::shared_ptr<page>);

  // Copy constructor
  parser(const parser& prs): m_db(prs.m_db){}

  // Move constructor
  parser(parser&& prs) noexcept : m_db(std::move(prs.m_db)){}

  // Assignment constructor
  parser& operator=(const parser& pr){m_db = std::move(pr.m_db); return *this;}

  // Public function called by tokenizer to update graphics state
  void reader(std::string&, Token::TState);

  // Access results
  textrows& output();

  // To recursively pass xobjects, we need to be able to see the operand
  std::string getOperand();

  // This allows us to process an xObject
  std::shared_ptr<std::string> getXobject(const std::string& inloop) const {
    return m_p->getXobject(inloop);
  };

private:
  //private data members - used to maintain state between calls to parser
  std::shared_ptr<page>             m_p;              // pointer to this page
  std::shared_ptr<font>             m_wfont;          // pointer to working font
  float                             m_currfontsize;   // Current font size
  std::vector<float>                m_fontsizestack;  // stack of font size
  std::array<float, 9>              m_Tmstate,        // Text matrix state
                                    m_Tdstate;        // Temp modification to Tm
  std::vector<std::array<float, 9>> m_gs;             // stack of graphics state
  std::string                       m_currentfont;    // Name of current font
  std::vector<std::string>          m_fontstack,      // stack of font history
                                    m_Operands;       // The actual data read
  std::vector<Token::TState>        m_OperandTypes;   // The type of data read
  int                               m_PRstate;        // current kerning state
  float                             m_Tl,             // Leading (line spacing)
                                    m_Tw,             // Word spacing
                                    m_Th,             // Horizontal scaling
                                    m_Tc;             // Character spacing
  textrows                          m_db;             // The main output struct

  // This typedef allows us to create a map of function pointers
  typedef void (parser::*fptr)();

  // Static private members
  static std::unordered_map<std::string, fptr> sm_fmap;
  static const std::array<float, 9> sm_initstate;

  // Private methods

  // The reader method takes the compiled instructions and writes operands
  // to a "stack", or calls an operator method depending on the label given
  // to each token in the instruction set. It loops through the entire
  // instruction set, after which the data just needs tidied and wrapped.

  void Do();              //----------------------------------//
  void Q();               //  OPERATOR METHODS
  void q();               //
  void TH();              //  These functions do the
  void TW();              //  work of performing actions
  void TC();              //  on the graphics state and
  void TL();              //  writing the results. They
  void T_();              //  are called by the reader()
  void Tm();              //  method according to the
  void cm();              //  operator it encounters, and
  void Td();              //  act on any operands sitting
  void TD();              //  on the stack. Each is named
  void BT();              //  for the operator it enacts.
  void ET();              //  These functions use private
  void Tf();              //  data members to maintain state
  void TJ();              //
  void Ap();              //---------------------------------//

  // This is a helper function for the TJ method which otherwise would become
  // a bit of a "hairball". It uses the font information and current graphics
  // state to identify the intended glyph, size and position from a character
  // in a pdf string object
  void processRawChar(std::vector<RawChar>&, float&,
                      std::array<float, 9>&, float&);

  // Multiplies to 3x3 matrices represented as length-9 vector floats
  void matmul(const std::array<float, 9>& , std::array<float, 9>&);

  // Converts pdfs' 6-token string representation of matrices to a 3x3 matrix
  std::array<float, 9> stringvectomat(const std::vector<std::string>&);
};



//---------------------------------------------------------------------------//

#endif
