//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR graphic_state implementation file                                   //
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

#include "graphic_state.h"

//---------------------------------------------------------------------------//

using namespace std;
using namespace Token;

//---------------------------------------------------------------------------//
// This typedef declares fptr as a function pointer

typedef void (graphic_state::*fptr)();

//---------------------------------------------------------------------------//
// This statically-declared map allows functions to be called based on strings
// passed to it from the tokenizer

std::unordered_map<std::string, fptr> graphic_state::fmap =
{
  {"Q",   &Q}, {"q",   &q}, {"BT", &BT}, {"ET", &ET}, {"cm", &cm}, {"Tm", &Tm},
  {"Tf", &Tf}, {"Td", &Td}, {"Th", &TH}, {"Tw", &TW}, {"Tc", &TC}, {"TL", &TL},
  {"T*", &T_}, {"TD", &TD}, {"'",  &Ap}, {"TJ", &TJ}, {"Tj", &TJ}
};

//---------------------------------------------------------------------------//
// The graphic_state constructor has to initialize many variables that allow
// it to track state once instructions are passed to it. After these are set,
// it does no work unless passed instructions by the tokenizer

graphic_state::graphic_state(shared_ptr<page> pag) : // long initializer list...
  p(pag), // pointer to page of interest
  currfontsize(0), // pointsize specified by page program
  initstate({1, 0, 0, 0, 1, 0, 0, 0, 1}), // 3x3 identity matrix
  fontsizestack({currfontsize}), // history of pointsize
  Tmstate(initstate), // text transformation matrix
  Tdstate(initstate), // Tm modifier
  gs({initstate}), // graphics state history
  currentfont(""), // name of current font
  fontstack({currentfont}), // font history
  PRstate(0), // kerning
  Tl(1), // leading
  Tw(0), // word spacing
  Th(100), // horizontal scaling
  Tc(0) // character spacing
{}

//---------------------------------------------------------------------------//
// To allow recursive parsing of form xobjects, the tokenizer needs to access
// the name of the xobject. At the point when the "Do" identifier is read by
// the tokenizer, the name of the xobject is sitting on the top of the
// operands stack. This public method passes that name on.

std::string graphic_state::getOperand()
{
  if(Operands.empty()) return string("");
  return Operands[0];
}

//---------------------------------------------------------------------------//
// We need to be able to pass the page pointer on to the tokenizer to read
// form xobjects using a public method

std::shared_ptr<page> graphic_state::getPage()
{
  return p;
}

//---------------------------------------------------------------------------//
// The public getter of the main data member

GSoutput* graphic_state::output()
{
  return &db;
}

/*---------------------------------------------------------------------------*/
// q operator - pushes a copy of the current graphics state to the stack

void graphic_state::q()
{
    ////PROFC_NODE("q");
  gs.emplace_back(gs.back());               // push transformation matrix
  fontstack.emplace_back(currentfont);      // push font name
  fontsizestack.emplace_back(currfontsize); // push pointsize
}

/*---------------------------------------------------------------------------*/
// Q operator - pop the graphics state stack

void graphic_state::Q()
{
    //PROFC_NODE("Q");
  if (gs.size() > 1) // Empty graphics state is undefined but gs[0] is identity
    gs.pop_back();
  if (fontstack.size() > 1) // Empty fontstack is undefined
  {
    fontstack.pop_back();
    fontsizestack.pop_back();         // pop the font & fontsize stacks
    currentfont = fontstack.back();
    currfontsize = fontsizestack.back(); // read the top font & size from stack
  }
  wfont = p->getFont(currentfont); // the top of stack is now working font
}

/*---------------------------------------------------------------------------*/
// Td operator - applies tranlational changes only to text matrix (Tm)

void graphic_state::Td()
{
    //PROFC_NODE("Td");
  array<float, 9> Tds = initstate;                  //------------------------
  vector<float> tmpvec = stringtofloat(Operands);   //  create translation
  Tds[6] = tmpvec[0];                               //  matrix (3x3)
  Tds[7] = tmpvec[1];                               //------------------------
  matmul(Tds, Tdstate); // multiply translation and text matrices
  PRstate = 0; // Td resets kerning
}

/*---------------------------------------------------------------------------*/
// TD operator - same as Td except it also sets the 'leading' (Tl) operator

void graphic_state::TD()
{
    //PROFC_NODE("TD");
  array<float, 9> Tds = initstate;                    //------------------------
  vector<float> tmpvec = stringtofloat(Operands);   //  create translation
  Tds[6] = tmpvec[0];                                //  matrix (3x3)
  Tds[7] = tmpvec[1];                               //------------------------
  matmul(Tds, Tdstate); // multiply translation and text matrices
  PRstate = 0; // TD resets kerning
  Tl = -Tds[7]; // set text leading to new value
}

/*---------------------------------------------------------------------------*/
// BT operator - signifies start of text

void graphic_state::BT()
{
    //PROFC_NODE("BT");
  Tmstate = Tdstate = initstate; // Reset text matrix to identity matrix
  Tw = Tc = 0; // reset word spacing and character spacing
  Th = 100; // reset horizontal spacing
}

/*---------------------------------------------------------------------------*/
// ET operator - signifies end of text

void graphic_state::ET()
{
    //PROFC_NODE("ET");
  Tmstate = Tdstate = initstate; // Reset text matrix to identity matrix
  Tw = Tc = 0; // reset word spacing and character spacing
  Th = 100; // reset horizontal spacing
}

/*---------------------------------------------------------------------------*/
// Tf operator - specifies font and pointsize
void graphic_state::Tf()
{
    //PROFC_NODE("Tf");
  if(Operands.size() > 1) // Should be 2 operators: 1 is not defined
  {
    currentfont = Operands[0];        // read fontID
    wfont = p->getFont(currentfont);  // get font from fontID
    currfontsize = stof(Operands[1]); // get font size
    fontsizestack.back() = currfontsize; // remember changes to current state
    fontstack.back() = currentfont;
  }
}

/*---------------------------------------------------------------------------*/
// TH - sets horizontal spacing

void graphic_state::TH()
{
    //PROFC_NODE("TH");
  Th = stof(Operands.at(0)); // simply reads operand as new Th value
}

/*---------------------------------------------------------------------------*/
// Tc operator - sets character spacing

void graphic_state::TC()
{
    //PROFC_NODE("TC");
  Tc = stof(Operands.at(0)); // simply reads operand as new Tc value
}

/*---------------------------------------------------------------------------*/
// TW operator - sets word spacing

void graphic_state::TW()
{
  Tw = stof(Operands.at(0));  // simply reads operand as new Tw value
}

/*---------------------------------------------------------------------------*/
// TL operator - sets leading (size of vertical jump to new line)

void graphic_state::TL()
{
    //PROFC_NODE("TL");
  Tl = stof(Operands.at(0));  // simply reads operand as new Tl value
}

/*---------------------------------------------------------------------------*/
// T* operator - moves to new line

void graphic_state::T_()
{
    //PROFC_NODE("T*");
  Tdstate.at(7) = Tdstate.at(7) - Tl; // decrease y value of text matrix by Tl
  PRstate = 0; // reset kerning
}

/*---------------------------------------------------------------------------*/
// Tm operator - sets the text matrix (convolve text relative to graphics state)

void graphic_state::Tm()
{
    //PROFC_NODE("Tm");
  Tmstate = stringvectomat(move(Operands)); // Reads operands as a 3x3 matrix
  Tdstate = initstate; // reset the Td modifier matrix
  PRstate = 0; // reset kerning
}

/*---------------------------------------------------------------------------*/
// cm operator - applies transformation matrix to graphics state

void graphic_state::cm()
{
    //PROFC_NODE("cm");
  // read the operands as a matrix, multiply by top of graphics state stack
  // and replace the top of the stack with the result
  matmul(stringvectomat(move(Operands)), gs.back());
}

/*---------------------------------------------------------------------------*/
// The "'" operator is a minor variation of the TJ function. Ap is short for
// apostrophe

void graphic_state::Ap()
{
  //PROFC_NODE("TJ");
  // the "'" operator is the same as Tj except it moves to the next line first
  Tdstate[7] -= Tl;
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

void graphic_state::TJ()
{
  // We create a text space that is the product of Tm and cm matrices
  array<float, 9> textspace = gs.back();
  matmul(Tmstate, textspace);

  // we now use the translation-only Td matrix to get our final text space
  matmul(Tdstate, textspace);

  // now we can set the starting x value of our string
  float txtspcinit = textspace[6];

  // The overall size of text is the font size times the textspace scale
  float scale = currfontsize * textspace[0];

  // We now iterate through our operands, paying attention to their types to
  // perform the correct operations
  size_t otsize = OperandTypes.size();
  for (size_t z = 0; z < otsize; z++)
  {
    if (OperandTypes[z] == NUMBER) // Numbers represent kerning
    {
      PRstate -= stof(Operands[z]); // PR (pushright) state is kerning * -1
      float PRscaled = PRstate * scale / 1000; // scale to user space
      textspace[6] = PRscaled + txtspcinit; // translate user space per PRstate
      continue; // skip to the next operand - important!
    }
    float PRscaled = PRstate * scale / 1000; // scale kerning to user space
    textspace[6] = PRscaled + txtspcinit; // translate user space per kerning
    if (Operands[z] == "") continue; // empty string; ignore & get next operand

    vector<RawChar> raw; // container for rawchar vector (cast from strings)
    // cast "<001F00AA>" style hexstring to vector of RawChar (uint16_t)
    if (OperandTypes[z] == HEXSTRING) raw = HexstringToRawChar(Operands[z]);
    // cast "(cat on mat)" style string to vector of RawChar (uint16_t)
    if (OperandTypes[z] == STRING) raw = StringToRawChar(Operands[z]);
    // Now we can process the string given the current user space and font
    processRawChar(raw, scale, textspace, txtspcinit);
  }
}

/*---------------------------------------------------------------------------*/
// This method is a helper of / extension of Tj which takes the RawChars
// generated, the userspace and initial userspace to calculate the
// glyphs, sizes and positions intended by the string in the page program

void graphic_state::processRawChar(vector<RawChar>& raw, float& scale,
                             array<float, 9>& textspace, float& txtspcinit)
{
  // look up the RawChars in the font to get their Unicode values and widths
  vector<pair<Unicode, int>>&& glyphpairs = wfont->mapRawChar(raw);

  for (auto& j : glyphpairs) // Now, for each character...
  {
    db.left.emplace_back( textspace[6]);                  //
    db.bottom.emplace_back( textspace[7]);                //
    float glyphwidth;
    if (j.first == 0x0020) // if this is a space factor in word & char spacing
      glyphwidth = j.second + 1000 * (Tc + Tw)/currfontsize;
    else
      glyphwidth = j.second + Tc * 1000/currfontsize; // else just char spacing
    PRstate += glyphwidth; // adjust the pushright in text space by char width
    // move user space right by the (converted to user space) width of the char
    textspace[6] =  PRstate * scale / 1000 + txtspcinit;

    // record width of char taking Th (horizontal scaling) into account
    db.width.emplace_back(scale * glyphwidth/1000 * Th/100);
    db.right.emplace_back(db.left.back() + db.width.back());
    // Store Unicode point
    db.text.emplace_back(j.first);

    // store fontsize for glyph
    db.size.emplace_back(scale);

    // store font name of glyph
    db.fonts.emplace_back(wfont->fontname());
  }
}

/*---------------------------------------------------------------------------*/
// The parser takes the instruction set generated by the lexer and enacts it.
// It does this by reading each token and its type. If it comes across an
// IDENTIFIER it calls the operator function for that symbol. Otherwise,
// it assumes it is reading an operand and places it on the operand stack.
// When an operator function is called, it takes the operands on the stack
// as arguments. The "inloop" parameter is there to prevent infinite loops -
// when the parser is called on an xobject it can get stuck if the xobject's
// contents call itself (it does happen!). To prevent this, an xobject
// identifies itself with "inloop" if it calls the parser, and the Do operator
// will not be called if its operand stack contains the same string as inloop.

void
graphic_state::parser(string& token, TState state)
{
  if (state == IDENTIFIER) // if it's an identifier, call the operator,
  {                           // passing any stored operands on the stack
    if (fmap.find(token) != fmap.end()) (this->*fmap[token])();
    OperandTypes.clear(); // clear the stack since an operator has been called
    Operands.clear(); // clear the stack since an operator has been called
  }
  else
  {
    // push operands and their types on stack, awaiting operator
    OperandTypes.push_back(state);
    Operands.push_back(token);
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

void graphic_state::matmul(const array<float, 9>& b, array<float, 9>& a)
{
  array<float, 9> newmat;
  for(size_t i = 0; i < 9; i++) //clever use of indices to allow fill by loop
    newmat[i] = (a[i % 3 + 0] * b[3 * (i / 3) + 0] +
                 a[i % 3 + 3] * b[3 * (i / 3) + 1] +
                 a[i % 3 + 6] * b[3 * (i / 3) + 2] );
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

array<float, 9> graphic_state::stringvectomat(const vector<string>& a)
{
  array<float, 9> newmat {stof(a[0]), stof(a[1]), 0,
                          stof(a[2]), stof(a[3]), 0,
                          stof(a[4]), stof(a[5]), 1};
  return newmat;
}

/*---------------------------------------------------------------------------*/
// Retrieve the minbox around the graphic_state

std::vector<float> graphic_state::getminbox()
{
  return p->getminbox();
}


