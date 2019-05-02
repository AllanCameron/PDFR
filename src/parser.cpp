//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR parser implementation file                                          //
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

#include "parser.h"

//---------------------------------------------------------------------------//

using namespace std;
using namespace Token;

//---------------------------------------------------------------------------//
// This typedef declares fptr as a function pointer

typedef void (parser::*fptr)();

//---------------------------------------------------------------------------//
// This statically-declared map allows functions to be called based on strings
// passed to it from the tokenizer

std::unordered_map<std::string, fptr> parser::sm_fmap =
{
  {"Q",   &Q}, {"q",   &q}, {"BT", &BT}, {"ET", &ET}, {"cm", &cm}, {"Tm", &Tm},
  {"Tf", &Tf}, {"Td", &Td}, {"Th", &TH}, {"Tw", &TW}, {"Tc", &TC}, {"TL", &TL},
  {"T*", &T_}, {"TD", &TD}, {"'",  &Ap}, {"TJ", &TJ}, {"Tj", &TJ}
};

const std::array<float, 9> parser::sm_initstate = {1, 0, 0, 0, 1, 0, 0, 0, 1};

//---------------------------------------------------------------------------//
// The parser constructor has to initialize many variables that allow
// it to track state once instructions are passed to it. After these are set,
// it does no work unless passed instructions by the tokenizer

parser::parser(shared_ptr<page> pag) : // long initializer list...
  m_p(pag),                            // pointer to page of interest
  m_currfontsize(0),                   // pointsize specified by page program
  m_fontsizestack({m_currfontsize}),   // history of pointsize
  m_Tmstate(sm_initstate),             // text transformation matrix
  m_Tdstate(sm_initstate),             // Tm modifier
  m_gs({sm_initstate}),                // graphics state history
  m_currentfont(""),                   // name of current font
  m_fontstack({m_currentfont}),        // font history
  m_PRstate(0),                        // kerning
  m_Tl(1),                             // leading
  m_Tw(0),                             // word spacing
  m_Th(100),                           // horizontal scaling
  m_Tc(0),                             // character spacing
  m_db(Box(m_p->getminbox()))
{}

//---------------------------------------------------------------------------//
// To allow recursive parsing of form xobjects, the tokenizer needs to access
// the name of the xobject. At the point when the "Do" identifier is read by
// the tokenizer, the name of the xobject is sitting on the top of the
// operands stack. This public method passes that name on.

std::string parser::getOperand()
{
  if(m_Operands.empty())
  {
    return string {};
  }
  else
  {
    return m_Operands[0];
  }
}

//---------------------------------------------------------------------------//
// The public getter of the main data member

textbox& parser::output()
{
  return m_db;
}

/*---------------------------------------------------------------------------*/
// q operator - pushes a copy of the current graphics state to the stack

void parser::q()
{
  m_gs.emplace_back(m_gs.back());               // push transformation matrix
  m_fontstack.emplace_back(m_currentfont);      // push font name
  m_fontsizestack.emplace_back(m_currfontsize); // push pointsize
}

/*---------------------------------------------------------------------------*/
// Q operator - pop the graphics state stack

void parser::Q()
{
  // Empty graphics state is undefined but m_gs[0] is identity
  if (m_gs.size() > 1)
  {
    m_gs.pop_back();
  }

  if (m_fontstack.size() > 1) // Empty m_fontstack is undefined
  {
    m_fontstack.pop_back();
    m_fontsizestack.pop_back();         // pop the font & fontsize stacks
    m_currentfont = m_fontstack.back(); // read the top font & size from stack
    m_currfontsize = m_fontsizestack.back();
  }
  // The top of stack is now working font
  m_wfont = m_p->getFont(m_currentfont);
}

/*---------------------------------------------------------------------------*/
// Td operator - applies tranlational changes only to text matrix (Tm)

void parser::Td()
{
  array<float, 9> Tds = sm_initstate;     //---------------------------------
  Tds[6] = stof(m_Operands[0]);           //  create 3 x 3 translation matrix
  Tds[7] = stof(m_Operands[1]);           //---------------------------------

  // Multiply translation and text matrices
  matmul(Tds, m_Tdstate);

  // Td resets kerning
  m_PRstate = 0;
}

/*---------------------------------------------------------------------------*/
// TD operator - same as Td except it also sets the 'leading' (Tl) operator

void parser::TD()
{
  Td();

  // Set text leading to new value
  m_Tl = -stof(m_Operands[1]);
}

/*---------------------------------------------------------------------------*/
// BT operator - signifies start of text

void parser::BT()
{
  // Reset text matrix to identity matrix
  m_Tmstate = m_Tdstate = sm_initstate;

  // Reset word spacing and character spacing
  m_Tw = m_Tc = 0;
  m_Th = 100; // reset horizontal spacing
}

/*---------------------------------------------------------------------------*/
// ET operator - signifies end of text

void parser::ET()
{
  BT();
}

/*---------------------------------------------------------------------------*/
// Tf operator - specifies font and pointsize

void parser::Tf()
{
  // Should be 2 operators: 1 is not defined
  if(m_Operands.size() > 1)
  {
    m_currentfont = m_Operands[0];           // Read fontID
    m_wfont = m_p->getFont(m_currentfont);   // Get font from fontID
    m_currfontsize = stof(m_Operands[1]);    // Get font size
    m_fontsizestack.back() = m_currfontsize; // Remember changes to state
    m_fontstack.back() = m_currentfont;      // Remember changes to state
  }
}

/*---------------------------------------------------------------------------*/
// TH - sets horizontal spacing

void parser::TH()
{
  // Reads operand as new horizontal spacing value
  m_Th = stof(m_Operands.at(0));
}

/*---------------------------------------------------------------------------*/
// Tc operator - sets character spacing

void parser::TC()
{
  // Reads operand as new character spacing value
  m_Tc = stof(m_Operands.at(0));
}

/*---------------------------------------------------------------------------*/
// TW operator - sets word spacing

void parser::TW()
{
  // Reads operand as new word spacing value
  m_Tw = stof(m_Operands.at(0));
}

/*---------------------------------------------------------------------------*/
// TL operator - sets leading (size of vertical jump to new line)

void parser::TL()
{
  // Reads operand as new text leading value
  m_Tl = stof(m_Operands.at(0));
}

/*---------------------------------------------------------------------------*/
// T* operator - moves to new line

void parser::T_()
{
  // Decrease y value of text matrix by amount specified by text leading param
  m_Tdstate.at(7) = m_Tdstate.at(7) - m_Tl;

  // This also resets the kerning
  m_PRstate = 0;
}

/*---------------------------------------------------------------------------*/
// Tm operator - sets the text matrix (convolve text relative to graphics state)

void parser::Tm()
{
  // Reads operands as a 3x3 matrix
  m_Tmstate = stringvectomat(move(m_Operands));

  // Reset the Td modifier matrix to identity matrix
  m_Tdstate = sm_initstate;

  // Reset the kerning
  m_PRstate = 0;
}

/*---------------------------------------------------------------------------*/
// cm operator - applies transformation matrix to graphics state

void parser::cm()
{
  // Read the operands as a matrix, multiply by top of graphics state stack
  // and replace the top of the stack with the result
  matmul(stringvectomat(move(m_Operands)), m_gs.back());
}

/*---------------------------------------------------------------------------*/
// The "'" operator is a minor variation of the TJ function. Ap is short for
// apostrophe

void parser::Ap()
{
  // the "'" operator is the same as Tj except it moves to the next line first
  m_Tdstate[7] -= m_Tl;
  TJ();
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

void parser::TJ()
{
  // We create a text space that is the product of Tm and cm matrices
  array<float, 9> textspace = m_gs.back();
  matmul(m_Tmstate, textspace);

  // we now use the translation-only Td matrix to get our final text space
  matmul(m_Tdstate, textspace);

  // now we can set the starting x value of our string
  float txtspcinit = textspace[6];

  // The overall size of text is the font size times the textspace scale
  float scale = m_currfontsize * textspace[0];

  // We now iterate through our operands, paying attention to their types to
  // perform the correct operations
  for (size_t z = 0; z < m_OperandTypes.size(); z++)
  {
    // If the operand type is a number, it is a kerning adjustment
    if (m_OperandTypes[z] == NUMBER)
    {
      // PR (pushright) state is kerning * -1
      m_PRstate -= stof(m_Operands[z]);

      // Translate user space per m_PRstate
      textspace[6] = m_PRstate * scale / 1000 + txtspcinit;

      // skip to the next operand - important!
      continue;
    }
    float PRscaled = m_PRstate * scale / 1000; // scale kerning to user space
    textspace[6] = PRscaled + txtspcinit; // translate user space per kerning

    // If string is empty, ignore it and get the next operand.
    if (m_Operands[z] == "")
    {
      continue;
    }

    // Container for rawchar vector (cast from strings)
    vector<RawChar> raw;

    // cast "<001F00AA>" style hexstring to vector of RawChar (uint16_t)
    if (m_OperandTypes[z] == HEXSTRING)
    {
      // scale kerning to user space
      float PRscaled = m_PRstate * scale / 1000;

      // translate user space per kerning
      textspace[6] = PRscaled + txtspcinit;

      // Convert the hexstring to raw char
      raw = HexstringToRawChar(m_Operands[z]);
    }

    // cast "(cat on mat)" style string to vector of RawChar (uint16_t)
    if (m_OperandTypes[z] == STRING)
    {
      // scale kerning to user space
      float PRscaled = m_PRstate * scale / 1000;

      // translate user space per kerning
      textspace[6] = PRscaled + txtspcinit;

      raw = StringToRawChar(m_Operands[z]);
    }

    // Now we can process the string given the current user space and font
    processRawChar(raw, scale, textspace, txtspcinit);
  }
}

/*---------------------------------------------------------------------------*/
// This method is a helper of / extension of Tj which takes the RawChars
// generated, the userspace and initial userspace to calculate the
// glyphs, sizes and positions intended by the string in the page program

void parser::processRawChar(vector<RawChar>& raw, float& scale,
                             array<float, 9>& textspace, float& txtspcinit)
{
  // look up the RawChars in the font to get their Unicode values and widths
  vector<pair<Unicode, int>>&& glyphpairs = m_wfont->mapRawChar(raw);

  // Now, for each character...
  for (auto& j : glyphpairs)
  {
    float glyphwidth, left, right, bottom, width;

    // If the first character is not a space, record its position as is
    if(j.first != 0x0020)
    {
      left = textspace[6];
      bottom = textspace[7];
    }

    // if this is a space, just adjust word & char spacing
    if (j.first == 0x0020)
    {
      glyphwidth = j.second + 1000 * (m_Tc + m_Tw)/m_currfontsize;
    }
    // Else just char spacing
    else
    {
      glyphwidth = j.second + m_Tc * 1000/m_currfontsize;
    }

    // Adjust the pushright in text space by character width
    m_PRstate += glyphwidth;

    // Move user space right by the (converted to user space) width of the char
    textspace[6] =  m_PRstate * scale / 1000 + txtspcinit;

    if (j.first != 0x0020)
    {
      // record width of char taking Th (horizontal scaling) into account
      width = scale * glyphwidth/1000 * m_Th/100;
      right = left + width;
      m_db.push_back(make_shared<text_element>(left, right, bottom + scale,
                                               bottom, m_wfont,
                                               vector<Unicode>{j.first}
                                          )
                    );
    }
  }
}

/*---------------------------------------------------------------------------*/
// The reader takes the instructions generated by the tokenizer and enacts them.
// It does this by reading each token and its type. If it comes across an
// IDENTIFIER it calls the operator function for that symbol. Otherwise,
// it assumes it is reading an operand and places it on the operand stack.
// When an operator function is called, it takes the operands on the stack
// as arguments.

void parser::reader(string& token, TState state)
{
  // if it's an identifier, call the operator
  if (state == IDENTIFIER)
  {
    // Pass any stored operands on the stack
    if (sm_fmap.find(token) != sm_fmap.end())
    {
      (this->*sm_fmap[token])();
    }
    // Clear the stack since an operator has been called
    m_OperandTypes.clear();
    m_Operands.clear();
  }
  else
  {
    // Push operands and their types on stack, awaiting operator
    m_OperandTypes.push_back(state);
    m_Operands.push_back(token);
  }
}

/*---------------------------------------------------------------------------*/
// Matrix mulitplication on two 3 x 3 matrices
// Note there is no matrix class - these are pseudo 3 x 3 matrices formed
// from single length-9 vectors in the format:
//
//                      | x[0]  x[1]  x[2] |
//                      |                  |
//                      | x[3]  x[4]  x[5] |
//                      |                  |
//                      | x[6]  x[7]  x[8] |
//

void parser::matmul(const array<float, 9>& b, array<float, 9>& a)
{
  array<float, 9> newmat;

  // Clever use of indices to allow fill by loop
  for(size_t i = 0; i < 9; ++i)
  {
    newmat[i] = (a[i % 3 + 0] * b[3 * (i / 3) + 0] +
                 a[i % 3 + 3] * b[3 * (i / 3) + 1] +
                 a[i % 3 + 6] * b[3 * (i / 3) + 2] );
  }

  swap(a, newmat);
}

/*---------------------------------------------------------------------------*/
// Allows a length-6 vector of number strings to be converted to 3x3 matrix
// (This is the way transformation matrices are represented in pdfs), since
// the 3rd column of any matrix is fixed at [0, 0, 1]
//
// For example, the entry "11 12 13 14 15 16 Tm" represents the following
// 3x3 matrix:
//
//                      |   11    12    0  |
//                      |                  |
//                      |   13    14    0  |
//                      |                  |
//                      |   15    16    1  |
//

array<float, 9> parser::stringvectomat(const vector<string>& a)
{
  array<float, 9> newmat {stof(a[0]), stof(a[1]), 0,
                          stof(a[2]), stof(a[3]), 0,
                          stof(a[4]), stof(a[5]), 1};
  return newmat;
}


