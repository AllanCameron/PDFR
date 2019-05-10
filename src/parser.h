//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Parser header file                                                  //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_GS

//---------------------------------------------------------------------------//

#define PDFR_GS

/* The job of the Parser class is to parse the pdf page description
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
 * In order that Parser can interpret the operands, it needs to know
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
 * The final output of Parser is a collection of vectors, all of the same
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
// to be made available to the instruction reader in the Parser class

namespace Token
{
  enum TokenState
  {
    NEWSYMBOL,  IDENTIFIER, NUMBER, RESOURCE, STRING,
    HEXSTRING,  ARRAY,      DICT,   WAIT,     OPERATOR
  };
};

//---------------------------------------------------------------------------//

class Matrix
{
public:
  // The default constructor returns a 3 x 3 identity matrix
  Matrix(): data_(std::array<float, 9> {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0}) {}

  Matrix(std::array<float, 9> t_array): data_(t_array){}

  // The other way of creating a 3 x 3 matrix is from a 6-element string vector,
  // as this is how Matrices are described in pdf.
  // For example, the entry "11 12 13 14 15 16 Tm" represents the following
  // 3x3 matrix:
  //
  //                      |   11    12    0  |
  //                      |                  |
  //                      |   13    14    0  |
  //                      |                  |
  //                      |   15    16    1  |
  //
  Matrix(const std::vector<std::string>& t_string)
  {
    data_ = {stof(t_string[0]), stof(t_string[1]), 0,
             stof(t_string[2]), stof(t_string[3]), 0,
             stof(t_string[4]), stof(t_string[5]), 1};
  }

  // Assignment constructor
  Matrix& operator=(const Matrix& t_other)
  {
    this->data_ = t_other.data_;
    return *this;
  }

  Matrix operator*(const Matrix& t_other)
  {
    std::array<float, 9> new_data {};

    // Clever use of indices to allow fill by loop
    for(size_t i = 0; i < 9; ++i)
    {
      new_data[i] = (data_[i % 3 + 0] * t_other.data_[3 * (i / 3) + 0] +
                     data_[i % 3 + 3] * t_other.data_[3 * (i / 3) + 1] +
                     data_[i % 3 + 6] * t_other.data_[3 * (i / 3) + 2] );
    }

    return Matrix(new_data);
  }

  void operator*=(const Matrix& t_other)
  {
    std::array<float, 9> new_data {};

    // Clever use of indices to allow fill by loop
    for(size_t i = 0; i < 9; ++i)
    {
      new_data[i] = (data_[i % 3 + 0] * t_other.data_[3 * (i / 3) + 0] +
                     data_[i % 3 + 3] * t_other.data_[3 * (i / 3) + 1] +
                     data_[i % 3 + 6] * t_other.data_[3 * (i / 3) + 2] );
    }

    std::swap(this->data_, new_data);
  }

  float& operator[](size_t index)
  {
    return data_[index];
  }

 private:
  std::array<float, 9> data_;
};

//---------------------------------------------------------------------------//

class Parser
{
public:
  // Basic constructor
  Parser(std::shared_ptr<Page>);

  // Copy constructor
  Parser(const Parser& prs): text_box_(prs.text_box_){}

  // Move constructor
  Parser(Parser&& prs) noexcept : text_box_(std::move(prs.text_box_)){}

  // Assignment constructor
  Parser& operator=(const Parser& pr)
  {
    text_box_ = std::move(pr.text_box_);
    return *this;
  }

  // Public function called by tokenizer to update graphics state
  void Reader(std::string&, Token::TokenState);

  // Access results
  TextBox& Output();

  // To recursively pass xobjects, we need to be able to see the operand
  std::string GetOperand();

  // This allows us to process an xObject
  std::shared_ptr<std::string> GetXObject(const std::string& inloop) const {
    return page_->GetXObject(inloop);
  };

private:
  //private data members - used to maintain state between calls to Parser
  std::shared_ptr<Page>             page_;              // pointer to this page
  std::shared_ptr<Font>             working_font_;  // pointer to working font
  float                             current_font_size_;   // Current font size
  std::vector<float>                font_size_stack_;  // stack of font size
  Matrix                            tm_state_,        // Text matrix state
                                    td_state_;      // Temp modification to Tm
  std::vector<Matrix>               graphics_state_;// stack of graphics state
  std::string                       current_font_;    // Name of current font
  std::vector<std::string>          font_stack_,      // stack of font history
                                    operands_;       // The actual data read
  std::vector<Token::TokenState>    operand_types_;   // The type of data read
  int                               kerning_;        // current kerning state
  float                             tl_,             // Leading (line spacing)
                                    tw_,             // Word spacing
                                    th_,             // Horizontal scaling
                                    tc_;             // Character spacing
  TextBox                           text_box_;             // The main output struct

  // This typedef allows us to create a map of function pointers
  typedef void (Parser::*fptr)();

  // Static private members
  static std::unordered_map<std::string, fptr> function_map_;
  static const std::array<float, 9> identity_;

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
  void ProcessRawChar(std::vector<RawChar>&, float&, Matrix&, float&);
};



#endif
