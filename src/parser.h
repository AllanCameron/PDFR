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
    NEWSYMBOL,
    IDENTIFIER,
    NUMBER,
    RESOURCE,
    STRING,
    HEXSTRING,
    ARRAY,
    DICT,
    WAIT,
    OPERATOR
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
              width,            // width of text element
              bottom,           // y position of bottom edge of text
              size;             // point size of font used to draw text
  std::string font;             // Name of font used to draw text
  std::vector<Unicode> glyph;   // The actual Unicode glyphs encoded
  bool        consumed;         // Should element be ignored in output?
  std::pair<int, int> rightjoin;// address of closest adjacent element
  bool isLeftEdge, isRightEdge, isMid;

  textrow(float l, float r, float w, float b, float s,
          std::string f, std::vector<Unicode> g): left(l), right(r), width(w),
          bottom(b), size(s), font(f), glyph(g), consumed(false),
          rightjoin(std::make_pair(-1, -1)), isLeftEdge(false),
          isRightEdge(false), isMid(false) {};

  bool operator ==(const textrow& a) const
  {
    return (a.left == this->left && a.bottom == this->bottom &&
            a.size == this->size && a.glyph == this->glyph);
  }
};

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
  std::vector<float> width;       // vector of glyphs' widths in text space
  std::vector<float> minbox;

  std::vector<textrow> transpose()
  {
    std::vector<textrow> res;
    if(!left.empty())
      for(size_t i = 0; i < left.size(); i++)
        res.emplace_back(textrow(left[i], right[i], width[i], bottom[i],
                                 size[i], fonts[i], text[i]));
    return res;
  }
};

struct textrows
{
  std::vector<textrow>::iterator begin(){return _data.begin();}
  std::vector<textrow>::iterator end(){return _data.end();}
  textrow& operator[](int n){return _data[n];}
  textrows(std::vector<textrow> t, std::vector<float> m):
    _data(t), minbox(m), data_size(t.size()) {}
  textrows(const textrows& t):
    _data(t._data), minbox(t.minbox), data_size(t._data.size()) {}
  textrows() = default;
  void push_back(textrow t){_data.push_back(t); data_size++;}
  size_t size(){return data_size;}

  GSoutput transpose(){
    GSoutput res;
    res.minbox = this->minbox;
    for(auto i : this->_data)
    {
      if(!i.consumed)
      {
        res.text.push_back(i.glyph);
        res.left.push_back(i.left);
        res.bottom.push_back(i.bottom);
        res.right.push_back(i.right);
        res.fonts.push_back(i.font);
        res.size.push_back(i.size);
        res.width.push_back(i.width);
      }
    }
    return res;
  }
  std::vector<textrow> _data;
  std::vector<float> minbox;
  size_t data_size;
};

//---------------------------------------------------------------------------//

class parser
{
public:
  // constructor
  parser(std::shared_ptr<page>);
  void reader(std::string&, Token::TState);

  // access results
  GSoutput& output();
  std::string getOperand();
  std::string getXobject(std::string inloop) const {
    return p->getXobject(inloop);
  };

private:
  //private data members - used to maintain state between calls to parser
  typedef void (parser::*fptr)();

  std::shared_ptr<page>           p;              // pointer to creating page
  std::shared_ptr<font>           wfont;          // pointer to "working" font
  float                           currfontsize;   // Current font size
  std::array<float, 9>            initstate;      // Identity 3x3 matrix as vec9
  std::vector<float>              fontsizestack;  // stack of current font size
  std::array<float, 9>            Tmstate,        // Text matrix state
                                  Tdstate;        // Temp modification to Tm
  std::vector<std::array<float, 9>> gs;           // stack of graphics state
  std::string                     currentfont;    // Name of current font
  std::vector<std::string>        fontstack,      // stack of font history
                                  Operands;
  std::vector<Token::TState>      OperandTypes;
  int                             PRstate;        // current kerning state
  float                           Tl,             // Leading (line spacing)
                                  Tw,             // Word spacing
                                  Th,             // Horizontal scaling
                                  Tc;             // Character spacing

  // The main output struct

  GSoutput db;

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
  void Tf();              //  data members to maintain
  void TJ();              //
  void Ap();              //---------------------------------//

  // This is a helper function for the TJ method which otherwise would become
  // a bit of a "hairball". It uses the font information and current graphics
  // state to identify the intended glyph, size and position from a character
  // in a pdf string object
  void processRawChar(std::vector<RawChar>&, float&,
                      std::array<float, 9>&,   float&);

  // Multiplies to 3x3 matrices represented as length-9 vector floats
  void matmul(const std::array<float, 9>& , std::array<float, 9>&);

  // Converts pdfs' 6-token string representation of matrices to a 3x3 matrix
  std::array<float, 9> stringvectomat(const std::vector<std::string>&);

  static std::unordered_map<std::string, fptr> fmap;
};



//---------------------------------------------------------------------------//

#endif
