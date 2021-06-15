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

#include "utilities.h"
#include "font.h"
#include "text_element.h"
#include "page.h"
#include "parser.h"
#include<iostream>

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
  {"Q",   &Parser::Q_  }, {"q",  &Parser::q_ }, {"BT",  &Parser::BT_ },
  {"ET",  &Parser::ET_ }, {"cm", &Parser::cm_}, {"Tm",  &Parser::Tm_ },
  {"Tf",  &Parser::Tf_ }, {"Td", &Parser::Td_}, {"Th",  &Parser::TH_ },
  {"Tw",  &Parser::TW_ }, {"Tc", &Parser::TC_}, {"TL",  &Parser::TL_ },
  {"T*",  &Parser::T__ }, {"TD", &Parser::TD_}, {"'",   &Parser::Ap_ },
  {"TJ",  &Parser::TJ_ }, {"Tj", &Parser::TJ_}, {"re",  &Parser::re_ },
  {"l",   &Parser::l_  }, {"m",  &Parser::m_ }, {"w",   &Parser::w_  },
  {"f",   &Parser::f_  }, {"F",  &Parser::f_ }, {"f*",  &Parser::f_  },
  {"s",   &Parser::s_  }, {"S",  &Parser::S_ }, {"CS",  &Parser::CS_ },
  {"cs",  &Parser::cs_ }, {"SC", &Parser::SC_}, {"sc",  &Parser::sc_ },
  {"h",   &Parser::h_  }, {"rg", &Parser::rg_}, {"RG",  &Parser::RG_ },
  {"G",   &Parser::G_  }, {"g",  &Parser::g_ }, {"scn", &Parser::scn_},
  {"SCN", &Parser::SCN_}, {"K",  &Parser::K_ }, {"k",   &Parser::k_  },
  {"c",   &Parser::c_  }, {"v",  &Parser::v_ }, {"y",   &Parser::y_  }
};

//---------------------------------------------------------------------------//
// Creates a 100 point Bezier interpolation for start point p1, end point p4
// and control points p2 and p3. Used only in implementing Bezier operators.
// This has to be used once for the x co-ordinates and once for the y
// co-ordinates.

std::vector<float> bezier(float p1, float p2, float p3, float p4) {

  std::vector<float> result(100);

  for(int i = 0; i < 100; i++)
  {
    float t1 = (i + 1) * 0.01;
    float t2 = 1 - t1;
    result[i] = 1 * t2 * t2 * t2 * p1 +
                3 * t1 * t2 * t2 * p2 +
                3 * t1 * t1 * t2 * p3 +
                1 * t1 * t1 * t1 * p4;
  }

   return result;

}

//---------------------------------------------------------------------------//
// The Parser constructor has to initialize many variables that allow
// it to track state once instructions are passed to it. After these are set,
// it does no work unless passed instructions by the tokenizer

Parser::Parser(shared_ptr<Page> page_ptr) :
  page_(page_ptr),
  text_box_(unique_ptr<TextBox>(new TextBox(Box(*(page_->GetMinbox()))))),
  graphics_state_({GraphicsState(page_ptr)}),  // Graphics state stack
  kerning_(0)
{}


/*---------------------------------------------------------------------------*/
// re operator - defines a rectangle

void Parser::re_()
{
  graphics_.emplace_back(std::make_shared<Path>());

  float left  = std::stof(operands_[0]);
  float width = std::stof(operands_[2]);
  float right = left + width;

  float bottom  = std::stof(operands_[1]);
  float height  = std::stof(operands_[3]);
  float top     = bottom + height;

  auto lb = graphics_state_.back().CTM.transformXY(left, bottom);
  auto rb = graphics_state_.back().CTM.transformXY(right, bottom);
  auto lt = graphics_state_.back().CTM.transformXY(left, top);
  auto rt = graphics_state_.back().CTM.transformXY(right, top);

  graphics_.back()->SetX({lb[0], lt[0], rt[0], rb[0], lb[0]});

  graphics_.back()->SetY({lb[1], lt[1], rt[1], rb[1], lb[1]});

  graphics_.back()->SetClosed(true);
}


/*---------------------------------------------------------------------------*/
// m operator moves the current graphics co-ordinate

void Parser::m_() {

  auto xy = graphics_state_.back().CTM.transformXY(std::stof(operands_[0]),
                                                   std::stof(operands_[1]));

  graphics_.emplace_back(std::make_shared<Path>());
  graphics_.back()->SetX({xy[0]});
  graphics_.back()->SetY({xy[1]});
}

/*---------------------------------------------------------------------------*/
// CS operator sets current color space for strokes

void Parser::CS_() {
  graphics_state_.back().colour_space_stroke = {operands_[0]};
}

/*---------------------------------------------------------------------------*/
// cs operator sets current color space for fills

void Parser::cs_() {
  graphics_state_.back().colour_space_fill  = {operands_[0]};
}

/*---------------------------------------------------------------------------*/
// SC operator sets stroke colour

void Parser::SC_() {

  size_t n = operands_.size();

  if(n == 1) G_();
  if(n == 3) RG_();
  if(n == 4) K_();
}


/*---------------------------------------------------------------------------*/
// SCN operator sets stroke colour via CMYK

void Parser::K_() {

  graphics_state_.back().colour_space_stroke = {"/DeviceCMYK"};

  // CMYK approximation
    float black = 1 - std::stof(operands_[3]);

    graphics_state_.back().colour = {
        (1 - std::stof(operands_[0])) * black,
        (1 - std::stof(operands_[1])) * black,
        (1 - std::stof(operands_[2])) * black
    };
}

/*---------------------------------------------------------------------------*/
// SCN operator sets stroke colour or pattern

void Parser::SCN_() {

  SC_();
}

/*---------------------------------------------------------------------------*/
// SCN operator sets fill colour or pattern

void Parser::scn_() {

  sc_();
}

/*---------------------------------------------------------------------------*/
// RG operator sets stroke colour

void Parser::RG_() {
  graphics_state_.back().colour_space_stroke = {"/DeviceRGB"};

  graphics_state_.back().colour = { std::stof(operands_[0]),
                                    std::stof(operands_[1]),
                                    std::stof(operands_[2])
                                  };
}

/*---------------------------------------------------------------------------*/
// rg operator sets fill colour

void Parser::rg_() {
  graphics_state_.back().colour_space_fill = {"/DeviceRGB"};
  graphics_state_.back().fill = { std::stof(operands_[0]),
                                  std::stof(operands_[1]),
                                  std::stof(operands_[2])
                                };
}

/*---------------------------------------------------------------------------*/
// RG operator sets stroke colour

void Parser::G_() {
  graphics_state_.back().colour_space_stroke = {"/DeviceGray"};
  graphics_state_.back().colour = { std::stof(operands_[0]),
                                    std::stof(operands_[0]),
                                    std::stof(operands_[0])
                                  };
}

/*---------------------------------------------------------------------------*/
// g operator sets fill colour

void Parser::g_() {
  graphics_state_.back().colour_space_fill = {"/DeviceGray"};
  graphics_state_.back().fill = { std::stof(operands_[0]),
                                  std::stof(operands_[0]),
                                  std::stof(operands_[0])
                                };
}
/*---------------------------------------------------------------------------*/
// sc operator sets fill colour

void Parser::sc_() {

  size_t n = operands_.size();

  if(n == 1) g_();

  if(n == 3) rg_();

  if(n == 4) k_();
}

/*---------------------------------------------------------------------------*/
// k operator sets fill colour

void::Parser::k_() {

  graphics_state_.back().colour_space_fill = {"/DeviceCMYK"};

  float black = 1 - std::stof(operands_[3]);

  graphics_state_.back().fill = {
      (1 - std::stof(operands_[0])) * black,
      (1 - std::stof(operands_[1])) * black,
      (1 - std::stof(operands_[2])) * black
  };
}

/*---------------------------------------------------------------------------*/
// l operator constructs a path segment

void Parser::l_() {

  auto xy = graphics_state_.back().CTM.transformXY(std::stof(operands_[0]),
                                                   std::stof(operands_[1]));

  graphics_.back()->SetLineWidth(graphics_state_.back().line_width *
                                graphics_state_.back().CTM[0]);

  graphics_.back()->AppendX(xy[0]);
  graphics_.back()->AppendY(xy[1]);

  }

/*---------------------------------------------------------------------------*/
// c operator constructs a bezier curve with two control points

void Parser::c_() {

    std::array<float, 2> xy0 = {graphics_.back()->GetX().back(),
                                graphics_.back()->GetY().back()};

    auto xy1 = graphics_state_.back().CTM.transformXY(std::stof(operands_[0]),
                                                      std::stof(operands_[1]));
    auto xy2 = graphics_state_.back().CTM.transformXY(std::stof(operands_[2]),
                                                      std::stof(operands_[3]));
    auto xy3 = graphics_state_.back().CTM.transformXY(std::stof(operands_[4]),
                                                      std::stof(operands_[5]));
    auto new_x = bezier(xy0[0], xy1[0], xy2[0], xy3[0]);
    auto new_y = bezier(xy0[1], xy1[1], xy2[1], xy3[1]);

    auto old_x = graphics_.back()->GetX();
    auto old_y = graphics_.back()->GetY();

    Concatenate(old_x, new_x);
    Concatenate(old_y, new_y);

    graphics_.back()->SetX(old_x);
    graphics_.back()->SetY(old_y);
}

/*---------------------------------------------------------------------------*/
// v operator constructs a bezier curve with single control point (first point
// also acting as control point)

void Parser::v_() {

  std::array<float, 2> xy0 = {graphics_.back()->GetX().back(),
                              graphics_.back()->GetY().back()};

  auto xy1 = xy0;
  auto xy2 = graphics_state_.back().CTM.transformXY(std::stof(operands_[0]),
                                  std::stof(operands_[1]));
  auto xy3 = graphics_state_.back().CTM.transformXY(std::stof(operands_[2]),
                                  std::stof(operands_[3]));
  auto new_x = bezier(xy0[0], xy1[0], xy2[0], xy3[0]);
  auto new_y = bezier(xy0[1], xy1[1], xy2[1], xy3[1]);

  auto old_x = graphics_.back()->GetX();
  auto old_y = graphics_.back()->GetY();

  Concatenate(old_x, new_x);
  Concatenate(old_y, new_y);

  graphics_.back()->SetX(old_x);
  graphics_.back()->SetY(old_y);
}

/*---------------------------------------------------------------------------*/
// y operator constructs a bezier curve with single control point (last point
// also acting as control point)

void Parser::y_() {

  std::array<float, 2> xy0 = {graphics_.back()->GetX().back(),
                              graphics_.back()->GetY().back()};

  auto xy1 = graphics_state_.back().CTM.transformXY(std::stof(operands_[0]),
                                  std::stof(operands_[1]));
  auto xy2 = graphics_state_.back().CTM.transformXY(std::stof(operands_[2]),
                                  std::stof(operands_[3]));
  auto xy3 = xy2;

  auto new_x = bezier(xy0[0], xy1[0], xy2[0], xy3[0]);
  auto new_y = bezier(xy0[1], xy1[1], xy2[1], xy3[1]);

  auto old_x = graphics_.back()->GetX();
  auto old_y = graphics_.back()->GetY();

  Concatenate(old_x, new_x);
  Concatenate(old_y, new_y);

  graphics_.back()->SetX(old_x);
  graphics_.back()->SetY(old_y);
}

/*---------------------------------------------------------------------------*/
// h operator closes path

void Parser::h_() {

  graphics_.back()->SetClosed(true);
  graphics_.back()->AppendX(graphics_.back()->GetX()[0]);
  graphics_.back()->AppendY(graphics_.back()->GetY()[0]);

  }

/*---------------------------------------------------------------------------*/
// w operator sets line width

void Parser::w_() {

  graphics_state_.back().line_width = std::stof(operands_[0]);

}

/*---------------------------------------------------------------------------*/
// f operator fills the previous path

void Parser::f_() {

  graphics_.back()->SetFilled(true);
  graphics_.back()->SetFillColour(graphics_state_.back().fill);
}

/*---------------------------------------------------------------------------*/
// S operator strokes the path

void Parser::S_() {

  graphics_.back()->SetStroke(true);
  graphics_.back()->SetColour(graphics_state_.back().colour);
  graphics_.back()->SetLineWidth(graphics_state_.back().line_width *
                                graphics_state_.back().CTM[0]);

}

/*---------------------------------------------------------------------------*/
// s operator closes and strokes the path

void Parser::s_() {

  h_();
  S_();

}


/*---------------------------------------------------------------------------*/
// q operator - pushes a copy of the current graphics state to the stack

void Parser::q_()
{
  graphics_state_.emplace_back(graphics_state_.back());
}

/*---------------------------------------------------------------------------*/
// Q operator - pop the graphics state stack

void Parser::Q_()
{
  // Empty graphics state is undefined but graphics_state_[0] is identity
  if (graphics_state_.size() > 1) graphics_state_.pop_back();
}

/*---------------------------------------------------------------------------*/
// Td operator - applies tranlational changes only to text matrix (Tm)

void Parser::Td_()
{
  Matrix Tds = Matrix();                 //---------------------------------
  Tds[6] = ParseFloats(operands_[0])[0]; //  create 3 x 3 translation matrix
  Tds[7] = ParseFloats(operands_[1])[0]; //---------------------------------

  // Multiply translation and text matrices
  graphics_state_.back().td_state *= Tds;

  // Td resets kerning
  kerning_ = 0;
}

/*---------------------------------------------------------------------------*/
// TD operator - same as Td except it also sets the 'leading' (Tl) operator

void Parser::TD_()
{
  Td_();
  // Set text leading to new value
  graphics_state_.back().text_state.tl = -ParseFloats(operands_[1])[0];
}

/*---------------------------------------------------------------------------*/
// BT operator - signifies start of text

void Parser::BT_()
{
  // Reset text matrix to identity matrix
  graphics_state_.back().tm_state = Matrix();
  graphics_state_.back().td_state = Matrix();

  // Reset word spacing and character spacing
  graphics_state_.back().text_state.tw = graphics_state_.back().text_state.tc = 0;
  graphics_state_.back().text_state.th = 100; // reset horizontal spacing
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
  if (operands_.size() > 1)
  {
    graphics_state_.back().text_state.tf = operands_[0];
    graphics_state_.back().text_state.current_font =
      page_->GetFont(graphics_state_.back().text_state.tf);
    graphics_state_.back().text_state.tfs = ParseFloats(operands_[1])[0];
  }
}

/*---------------------------------------------------------------------------*/
// TH - sets horizontal spacing

void Parser::TH_()
{
  // Reads operand as new horizontal spacing value
  graphics_state_.back().text_state.th = stof(operands_.at(0));
}

/*---------------------------------------------------------------------------*/
// Tc operator - sets character spacing

void Parser::TC_()
{
  // Reads operand as new character spacing value
  graphics_state_.back().text_state.tc = stof(operands_.at(0));
}

/*---------------------------------------------------------------------------*/
// TW operator - sets word spacing

void Parser::TW_()
{
  // Reads operand as new word spacing value
  graphics_state_.back().text_state.tw = stof(operands_.at(0));
}

/*---------------------------------------------------------------------------*/
// TL operator - sets leading (size of vertical jump to new line)

void Parser::TL_()
{
  // Reads operand as new text leading value
  graphics_state_.back().text_state.tl = stof(operands_.at(0));
}

/*---------------------------------------------------------------------------*/
// T* operator - moves to new line

void Parser::T__()
{
  // Decrease y value of text matrix by amount specified by text leading param
  graphics_state_.back().td_state[7] = graphics_state_.back().td_state[7] -
                                       graphics_state_.back().text_state.tl;

  // This also resets the kerning
  kerning_ = 0;
}

/*---------------------------------------------------------------------------*/
// Tm operator - sets the text matrix (convolve text relative to graphics state)

void Parser::Tm_()
{
  // Reads operands as a 3x3 matrix
  graphics_state_.back().tm_state = Matrix(operands_);

  // Reset the Td modifier matrix to identity matrix
  graphics_state_.back().td_state = Matrix();

  // Reset the kerning
  kerning_ = 0;
}

/*---------------------------------------------------------------------------*/
// cm operator - applies transformation matrix to graphics state

void Parser::cm_()
{
  // Read the operands as a matrix, multiply by top of graphics state stack
  // and replace the top of the stack with the result
  graphics_state_.back().CTM *= Matrix(move(operands_));
}

/*---------------------------------------------------------------------------*/
// The "'" operator is a minor variation of the TJ function. Ap is short for
// apostrophe

void Parser::Ap_()
{
  // The "'" operator is the same as Tj except it moves to the next line first
  graphics_state_.back().td_state[7] -= graphics_state_.back().text_state.tl;
  kerning_ = 0;
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
  // Creates text space that is the product of Tm, td and cm matrices
  // and sets the starting x value and scale of our string
  Matrix text_space = graphics_state_.back().CTM *
                      graphics_state_.back().tm_state *
                      graphics_state_.back().td_state;
  float initial_x   = text_space[6],
        scale       = graphics_state_.back().text_state.tfs * text_space[0];

  // We now iterate through our operands and their associated types
  for (size_t index = 0; index < operand_types_.size(); index++)
  {
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
   if (!operands_[index].empty()) ProcessRawChar_(scale, text_space, initial_x);
  }
}

/*---------------------------------------------------------------------------*/
// This method is a helper of / extension of Tj which takes the RawChars
// generated, the userspace and initial userspace to calculate the
// glyphs, sizes and positions intended by the string in the page program

void Parser::ProcessRawChar_(float& scale, Matrix& text_space,
                             float& initial_x)
{
  // Look up the RawChars in the font to get their Unicode values and widths
  vector<pair<Unicode, float>>&& glyph_pairs =
    graphics_state_.back().text_state.current_font->MapRawChar(raw_);

  // Now, for each character...
  for (auto& glyph_pair : glyph_pairs)
  {
    float glyph_width, left, right, bottom, width;

    // If the first character is not a space, record its position as is and
    // adjust for character spacing
    if (glyph_pair.first != 0x0020)
    {
      left = text_space[6];
      bottom = text_space[7];
      glyph_width = glyph_pair.second + graphics_state_.back().text_state.tc * 1000 /
                    graphics_state_.back().text_state.tfs;
    }
    else // if this is a space, just adjust word & char spacing
    {
      glyph_width = glyph_pair.second +
                    1000 * (graphics_state_.back().text_state.tc +
                    graphics_state_.back().text_state.tw) /
                    graphics_state_.back().text_state.tfs;
    }

    // Adjust the kerning in text space by character width
    kerning_ += glyph_width;

    // Move user space right by the (converted to user space) width of the char
    text_space[6] =  kerning_ * scale / 1000 + initial_x;

    if (glyph_pair.first != 0x0020)
    {
      // record width of char taking Th (horizontal scaling) into account
      width = scale * (glyph_width / 1000) *
              (graphics_state_.back().text_state.th / 100);
      right = left + width;
      text_box_->emplace_back(make_shared<TextElement>
                             (left, right, bottom + scale,
                              bottom, scale,
                              graphics_state_.back().text_state.current_font,
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

void Parser::Reader(const string& token, TokenState state)
{
  // if it's an identifier, call the operator
  if (state == IDENTIFIER)
  {
    // Pass any stored operands on the stack
    auto finder = function_map_.find(token);
    if (finder != function_map_.end()) (this->*function_map_[token])();

    // Clear the stack since an operator has been called
    operand_types_.clear();
    operands_.clear();
  }
  else
  {
    // Push operands and their types on stack, awaiting operator
    operand_types_.push_back(state);
    operands_.push_back(token);
  }
}

/*---------------------------------------------------------------------------*/
// Can't inline this without including page.h in header
shared_ptr<string> Parser::GetXObject(const string& inloop) const
{
  return page_->GetXObject(inloop);
};
