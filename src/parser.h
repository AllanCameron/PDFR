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

#include "text_element.h"
#include<functional>

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

class parser
{
public:
  // Basic constructor
  parser(std::shared_ptr<Page>);

  // Copy constructor
  parser(const parser& prs): m_db(prs.m_db){}

  // Move constructor
  parser(parser&& prs) noexcept : m_db(std::move(prs.m_db)){}

  // Assignment constructor
  parser& operator=(const parser& pr){m_db = std::move(pr.m_db); return *this;}

  // Public function called by tokenizer to update graphics state
  void reader(std::string&, Token::TState);

  // Access results
  TextBox& output();

  // To recursively pass xobjects, we need to be able to see the operand
  std::string getOperand();

  // This allows us to process an xObject
  std::shared_ptr<std::string> getXobject(const std::string& inloop) const {
    return m_p->get_XObject(inloop);
  };

private:
  //private data members - used to maintain state between calls to parser
  std::shared_ptr<Page>             m_p;              // pointer to this page
  std::shared_ptr<Font>             m_wfont;          // pointer to working font
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
  TextBox                           m_db;             // The main output struct

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
