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

#ifndef PDFR_PARSER

//---------------------------------------------------------------------------//

#define PDFR_PARSER

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

#include "textbox.h"
#include "page.h"
#include "graphicsstate.h"


using RawChar = uint16_t;

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

inline std::string ShowToken(const Token::TokenState& token)
{
  switch(token)
  {
    case Token::NEWSYMBOL : return "NEWSYMBOL";
    case Token::IDENTIFIER : return "IDENTIFIER";
    case Token::NUMBER : return "NUMBER";
    case Token::RESOURCE : return "RESOURCE";
    case Token::STRING : return "STRING";
    case Token::HEXSTRING : return "HEXSTRING";
    case Token::ARRAY : return "ARRAY";
    case Token::DICT : return "DICT";
    case Token::WAIT : return "WAIT";
    case Token::OPERATOR : return "OPERATOR";
    default : return "Unknown operator";
  }
}


class Parser
{
 public:
  // Basic constructor
  Parser(std::shared_ptr<Page>);

  // Move constructor
  Parser(Parser&& other) noexcept : text_box_(std::move(other.text_box_)){}

  // Public function called by tokenizer to update graphics state
  void Reader(const std::string& token, Token::TokenState token_type);

  // Access results
  std::unique_ptr<TextBox> Output() {return std::move(this->text_box_);}
  std::vector<std::shared_ptr<GraphicObject>> GetGraphics() {
    return this->graphics_;
  }

  // To allow recursive parsing of form xobjects, the tokenizer needs to access
  // the name of the xobject. At the point when the "Do" identifier is read by
  // the tokenizer, the name of the xobject is sitting on the top of the
  // operands stack. This public method passes that name out of the Parser.
  std::string GetOperand()
  {
    if (this->operands_.empty()) return std::string {};
    else return this->operands_[0];
  }

  // This allows us to process an xObject
  std::shared_ptr<std::string> GetXObject(const std::string& inloop) const;

  std::shared_ptr<Page> PagePointer() {return this->page_;}

 private:
  // Private data members
  std::shared_ptr<Page>                       page_;
  std::unique_ptr<TextBox>                    text_box_;
  std::vector<std::shared_ptr<GraphicObject>> graphics_;

  // Variables used to maintain state between calls
  std::vector<GraphicsState>                  graphics_state_;
  std::vector<std::string>                    operands_;
  std::vector<Token::TokenState>              operand_types_;
  int                                         kerning_;
  std::vector<RawChar>                        raw_;

  // This typedef allows us to create a map of function pointers
  typedef void (Parser::*FunctionPointer)();

  // A map that can look up which function to call based on the instruction sent
  static std::unordered_map<std::string, FunctionPointer> function_map_;

  // The reader method takes the compiled instructions and writes operands
  // to a "stack", or calls an operator method depending on the label given
  // to each token in the instruction set. It loops through the entire
  // instruction set, after which the data just needs tidied and wrapped.

  void Do_();              //----------------------------------//
  void Q_();               //  OPERATOR METHODS
  void q_();               //
  void TH_();              //  These functions do the
  void TW_();              //  work of performing actions
  void TC_();              //  on the graphics state and
  void TL_();              //  writing the results. They
  void T__();              //  are called by the reader()
  void Tm_();              //  method according to the
  void cm_();              //  operator it encounters, and
  void Td_();              //  act on any operands sitting
  void TD_();              //  on the stack. Each is named
  void BT_();              //  for the operator it enacts.
  void ET_();              //  These functions use private
  void Tf_();              //  data members to maintain state
  void TJ_();              //
  void re_();              //
  void m_();               //
  void w_();               //
  void l_();               //
  void g_();               //
  void G_();               //
  void f_();               //
  void s_();               //
  void S_();               //
  void CS_();              //
  void cs_();              //
  void sc_();              //
  void SC_();              //
  void h_();               //
  void rg_();              //
  void RG_();              //
  void scn_();             //
  void SCN_();             //
  void k_();               //
  void K_();               //
  void c_();               //
  void Ap_();              //---------------------------------//

  // This is a helper function for the TJ method which otherwise would become
  // a bit of a "hairball". It uses the font information and current graphics
  // state to identify the intended glyph, size and position from a character
  // in a pdf string object
  void ProcessRawChar_(float&, Matrix&, float&);
};

#endif
