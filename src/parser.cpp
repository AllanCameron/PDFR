//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Parser implementation file                                          //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "parser.h"

//---------------------------------------------------------------------------//

using namespace std;
using namespace Token;

//---------------------------------------------------------------------------//
// This typedef declares fptr as a function pointer

typedef void (Parser::*FunctionPointer)();

//---------------------------------------------------------------------------//
// This statically-declared map allows functions to be called based on strings
// passed to it from the tokenizer

std::unordered_map<std::string, FunctionPointer> Parser::function_map_ =
{
  {"Q",   &Q_}, {"q",   &q_}, {"BT", &BT_}, {"ET", &ET_}, {"cm", &cm_},
  {"Tm", &Tm_}, {"Tf", &Tf_}, {"Td", &Td_}, {"Th", &TH_}, {"Tw", &TW_},
  {"Tc", &TC_}, {"TL", &TL_}, {"T*", &T__}, {"TD", &TD_}, {"'",  &Ap_},
  {"TJ", &TJ_}, {"Tj", &TJ_}
};

//---------------------------------------------------------------------------//
// The Parser constructor has to initialize many variables that allow
// it to track state once instructions are passed to it. After these are set,
// it does no work unless passed instructions by the tokenizer

Parser::Parser(shared_ptr<Page> pag) :      // Long initializer list...
  page_(pag),                               // Pointer to page of interest
  text_box_(Box(page_->GetMinbox())),       // Container for results
  current_font_size_(0),                    // Pointsize of current font
  font_size_stack_({current_font_size_}),   // History of pointsize
  tm_state_(Matrix()),                      // Transformation matrix
  td_state_(Matrix()),                      // Tm modifier
  graphics_state_({Matrix()}),              // Graphics state history
  current_font_(""),                        // Name of current font
  font_stack_({current_font_}),             // Font history
  kerning_(0),                              // Kerning
  tl_(1),                                   // Leading
  tw_(0),                                   // Word spacing
  th_(100),                                 // Horizontal scaling
  tc_(0)                                    // Character spacing
{}

/*---------------------------------------------------------------------------*/
// q operator - pushes a copy of the current graphics state to the stack

void Parser::q_()
{
  graphics_state_.emplace_back(graphics_state_.back()); // push tm matrix
  font_stack_.emplace_back(current_font_);              // push font name
  font_size_stack_.emplace_back(current_font_size_);    // push pointsize
}

/*---------------------------------------------------------------------------*/
// Q operator - pop the graphics state stack

void Parser::Q_()
{
  // Empty graphics state is undefined but graphics_state_[0] is identity
  if (graphics_state_.size() > 1) graphics_state_.pop_back();

  if (font_stack_.size() > 1) // Empty font_stack_ is undefined
  {
    font_stack_.pop_back();
    font_size_stack_.pop_back();         // Pop the font & fontsize stacks
    current_font_ = font_stack_.back();  // Read the top font & size from stack
    current_font_size_ = font_size_stack_.back();
  }
  // The top of stack is now working font
  working_font_ = page_->GetFont(current_font_);
}

/*---------------------------------------------------------------------------*/
// Td operator - applies tranlational changes only to text matrix (Tm)

void Parser::Td_()
{
  Matrix Tds = Matrix();                 //---------------------------------
  Tds[6] = stof(operands_[0]);           //  create 3 x 3 translation matrix
  Tds[7] = stof(operands_[1]);           //---------------------------------

  // Multiply translation and text matrices
  td_state_ *= Tds;

  // Td resets kerning
  kerning_ = 0;
}

/*---------------------------------------------------------------------------*/
// TD operator - same as Td except it also sets the 'leading' (Tl) operator

void Parser::TD_()
{
  Td_();

  // Set text leading to new value
  tl_ = -stof(operands_[1]);
}

/*---------------------------------------------------------------------------*/
// BT operator - signifies start of text

void Parser::BT_()
{
  // Reset text matrix to identity matrix
  tm_state_ = td_state_ = Matrix();

  // Reset word spacing and character spacing
  tw_ = tc_ = 0;
  th_ = 100; // reset horizontal spacing
}

/*---------------------------------------------------------------------------*/
// ET operator - signifies end of text

void Parser::ET_()
{
  BT_();
}

/*---------------------------------------------------------------------------*/
// Tf operator - specifies font and pointsize

void Parser::Tf_()
{
  // Should be 2 operators: 1 is not defined
  if(operands_.size() > 1)
  {
    current_font_ = operands_[0];                  // Read fontID
    working_font_ = page_->GetFont(current_font_); // Get font from fontID
    current_font_size_ = stof(operands_[1]);       // Get font size
    font_size_stack_.back() = current_font_size_;  // Remember changes to state
    font_stack_.back() = current_font_;            // Remember changes to state
  }
}

/*---------------------------------------------------------------------------*/
// TH - sets horizontal spacing

void Parser::TH_()
{
  // Reads operand as new horizontal spacing value
  th_ = stof(operands_.at(0));
}

/*---------------------------------------------------------------------------*/
// Tc operator - sets character spacing

void Parser::TC_()
{
  // Reads operand as new character spacing value
  tc_ = stof(operands_.at(0));
}

/*---------------------------------------------------------------------------*/
// TW operator - sets word spacing

void Parser::TW_()
{
  // Reads operand as new word spacing value
  tw_ = stof(operands_.at(0));
}

/*---------------------------------------------------------------------------*/
// TL operator - sets leading (size of vertical jump to new line)

void Parser::TL_()
{
  // Reads operand as new text leading value
  tl_ = stof(operands_.at(0));
}

/*---------------------------------------------------------------------------*/
// T* operator - moves to new line

void Parser::T__()
{
  // Decrease y value of text matrix by amount specified by text leading param
  td_state_[7] = td_state_[7] - tl_;

  // This also resets the kerning
  kerning_ = 0;
}

/*---------------------------------------------------------------------------*/
// Tm operator - sets the text matrix (convolve text relative to graphics state)

void Parser::Tm_()
{
  // Reads operands as a 3x3 matrix
  tm_state_ = Matrix(move(operands_));

  // Reset the Td modifier matrix to identity matrix
  td_state_ = Matrix();

  // Reset the kerning
  kerning_ = 0;
}

/*---------------------------------------------------------------------------*/
// cm operator - applies transformation matrix to graphics state

void Parser::cm_()
{
  // Read the operands as a matrix, multiply by top of graphics state stack
  // and replace the top of the stack with the result
  graphics_state_.back() *= Matrix(move(operands_));
}

/*---------------------------------------------------------------------------*/
// The "'" operator is a minor variation of the TJ function. Ap is short for
// apostrophe

void Parser::Ap_()
{
  // The "'" operator is the same as Tj except it moves to the next line first
  td_state_[7] -= tl_;
  TJ_();
}

/*---------------------------------------------------------------------------*/
// TJ operator - prints glyphs to the output. This is the crux of the reading
// process, because it is where all the elements come together to get the
// values needed for each character. Since there are actually 3 operators
// that print text in largely overlapping ways, they are all handled here,
// but that requires an extra parameter to be passed in to specify which
// operation we are dealing with.
//
// This function is heavily commented as a little mistake here can screw
// everything up. YOU HAVE BEEN WARNED!

void Parser::TJ_()
{
  // We create a text space that is the product of Tm and cm matrices
  Matrix text_space = graphics_state_.back() * tm_state_ * td_state_;

  // now we can set the starting x value and scale of our string
  float initial_x = text_space[6], scale = current_font_size_ * text_space[0];

  // We now iterate through our operands and their associated types
  for (size_t index = 0; index < operand_types_.size(); index++)
  {
    // If string is empty, ignore it and get the next operand.
    if (operands_[index] == "") continue;

    // Adjust the text space according to kerning and scale
    text_space[6] = kerning_ * scale / 1000 + initial_x;

    // Depending on the operand type, we process the operand as appropriate
    switch (operand_types_[index])
    {
      case NUMBER    : kerning_ -= stof(operands_[index]); continue;
      case HEXSTRING : raw_ = ConvertHexToRawChar(operands_[index]); break;
      case STRING    : raw_ = ConvertStringToRawChar(operands_[index]); break;
      default        : continue;
    }

    // Now we can process the string given the current user space and font
    ProcessRawChar_(scale, text_space, initial_x);
  }
}

/*---------------------------------------------------------------------------*/
// This method is a helper of / extension of Tj which takes the RawChars
// generated, the userspace and initial userspace to calculate the
// glyphs, sizes and positions intended by the string in the page program

void Parser::ProcessRawChar_(float& t_scale, Matrix& t_text_space,
                             float& t_initial_x)
{
  // Look up the RawChars in the font to get their Unicode values and widths
  vector<pair<Unicode, int>>&& glyph_pairs = working_font_->MapRawChar(raw_);

  // Now, for each character...
  for (auto& glyph_pair : glyph_pairs)
  {
    float glyph_width, left, right, bottom, width;

    // If the first character is not a space, record its position as is
    if(glyph_pair.first != 0x0020)
    {
      left = t_text_space[6];
      bottom = t_text_space[7];
    }

    // if this is a space, just adjust word & char spacing
    if (glyph_pair.first == 0x0020)
    {
      glyph_width = glyph_pair.second + 1000 * (tc_ + tw_) / current_font_size_;
    }
    // Else just char spacing
    else
    {
      glyph_width = glyph_pair.second + tc_ * 1000 / current_font_size_;
    }

    // Adjust the pushright in text space by character width
    kerning_ += glyph_width;

    // Move user space right by the (converted to user space) width of the char
    t_text_space[6] =  kerning_ * t_scale / 1000 + t_initial_x;

    if (glyph_pair.first != 0x0020)
    {
      // record width of char taking Th (horizontal scaling) into account
      width = t_scale * (glyph_width / 1000) * (th_ / 100);
      right = left + width;
      text_box_.push_back(make_shared<TextElement>
                            (left, right, bottom + t_scale,
                             bottom, working_font_,
                             vector<Unicode>{glyph_pair.first}));
    }
  }
  raw_.clear();
}

/*---------------------------------------------------------------------------*/
// The reader takes the instructions generated by the tokenizer and enacts them.
// It does this by reading each token and its type. If it comes across an
// IDENTIFIER it calls the operator function for that symbol. Otherwise,
// it assumes it is reading an operand and places it on the operand stack.
// When an operator function is called, it takes the operands on the stack
// as arguments.

void Parser::Reader(string& t_token, TokenState t_state)
{
  // if it's an identifier, call the operator
  if (t_state == IDENTIFIER)
  {
    // Pass any stored operands on the stack
    if (function_map_.find(t_token) != function_map_.end())
    {
      (this->*function_map_[t_token])();
    }
    // Clear the stack since an operator has been called
    operand_types_.clear();
    operands_.clear();
  }
  else
  {
    // Push operands and their types on stack, awaiting operator
    operand_types_.push_back(t_state);
    operands_.push_back(t_token);
  }
}


