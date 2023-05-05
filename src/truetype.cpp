//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR TrueType implementation file                                        //
//                                                                           //
//  Copyright (C) 2018 - 2021 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include<cmath>
#include <stdexcept>
#include <iostream>
#include "truetype.h"


/*---------------------------------------------------------------------------*/

uint8_t TTFont::GetUint8()
{
  if (it_ == stream_.end())
    throw std::runtime_error("Insufficient bytes for GetUint8()");
  return (uint8_t) *it_++;
}

/*---------------------------------------------------------------------------*/

uint16_t TTFont::GetUint16()
{
  if (stream_.end() - it_ < 2)
    throw std::runtime_error("Insufficient bytes for GetUint16()");
  uint16_t res = 0;
  res += 256 * ((uint16_t) (uint8_t) *it_++);
  res += (uint16_t) (uint8_t) *it_++;
  return res;
};

/*---------------------------------------------------------------------------*/

int16_t TTFont::GetInt16()
{
  if (stream_.end() - it_ < 2)
    throw std::runtime_error("Insufficient bytes for GetInt16()");

  return (int16_t) GetUint16();
};

/*---------------------------------------------------------------------------*/

Fword TTFont::GetFword()
{
  return GetInt16();
}

/*---------------------------------------------------------------------------*/

Fixed TTFont::GetFixed()
{
  return GetInt32() / (1 << 16);
}

/*---------------------------------------------------------------------------*/

double TTFont::GetF2Dot14()
{
  uint16_t num = GetInt16();
  int integer = num >> 14;
  double fraction = ((double) (num & 0x3fff)) / ((double) 0x4000);
  return integer + fraction;
}

/*---------------------------------------------------------------------------*/

Date_type TTFont::GetDate()
{
  int64_t macTime = ((int64_t) GetUint32()) << 32;
  macTime |= GetUint32();
  return macTime;
}

/*---------------------------------------------------------------------------*/

int32_t TTFont::GetInt32()
{
  if (stream_.end() - it_ < 4)
    throw std::runtime_error("Insufficient bytes for GetInt32()");
  int32_t res = 0;
  res = res | *it_++ << 24;
  res = res | *it_++ << 16;
  res = res | *it_++ << 8;
  res = res | *it_++;
  return res;
};

/*---------------------------------------------------------------------------*/

uint32_t TTFont::GetUint32()
{
  if (stream_.end() - it_ < 4)
    throw std::runtime_error("Insufficient bytes for GetUint32()");
  uint32_t res = GetUint16() << 16;
  res += GetUint16();
  return res;
};

/*---------------------------------------------------------------------------*/

std::string TTFont::GetPascalString()
{
  uint8_t end = GetUint8();

  std::string result(it_, it_ + end);
  it_ += end;
  return result;
}

/*---------------------------------------------------------------------------*/

std::map<uint16_t, std::string> postscript_glyphs = {
  {0, ".notdef"},
  {1, ".null"},
  {2, "nonmarkingreturn"},
  {3, "space"},
  {4, "exclam"},
  {5, "quotedbl"},
  {6, "numbersign"},
  {7, "dollar"},
  {8, "percent"},
  {9, "ampersand"},
  {10, "quotesingle"},
  {11, "parenleft"},
  {12, "parenright"},
  {13, "asterisk"},
  {14, "plus"},
  {15, "comma"},
  {16, "hyphen"},
  {17, "period"},
  {18, "slash"},
  {19, "zero"},
  {20, "one"},
  {21, "two"},
  {22, "three"},
  {23, "four"},
  {24, "five"},
  {25, "six"},
  {26, "seven"},
  {27, "eight"},
  {28, "nine"},
  {29, "colon"},
  {30, "semicolon"},
  {31, "less"},
  {32, "equal"},
  {33, "greater"},
  {34, "question"},
  {35, "at"},
  {36, "A"},
  {37, "B"},
  {38, "C"},
  {39, "D"},
  {40, "E"},
  {41, "F"},
  {42, "G"},
  {43, "H"},
  {44, "I"},
  {45, "J"},
  {46, "K"},
  {47, "L"},
  {48, "M"},
  {49, "N"},
  {50, "O"},
  {51, "P"},
  {52, "Q"},
  {53, "R"},
  {54, "S"},
  {55, "T"},
  {56, "U"},
  {57, "V"},
  {58, "W"},
  {59, "X"},
  {60, "Y"},
  {61, "Z"},
  {62, "bracketleft"},
  {63, "backslash"},
  {64, "bracketright"},
  {65, "asciicircum"},
  {66, "underscore"},
  {67, "grave"},
  {68, "a"},
  {69, "b"},
  {70, "c"},
  {71, "d"},
  {72, "e"},
  {73, "f"},
  {74, "g"},
  {75, "h"},
  {76, "i"},
  {77, "j"},
  {78, "k"},
  {79, "l"},
  {80, "m"},
  {81, "n"},
  {82, "o"},
  {83, "p"},
  {84, "q"},
  {85, "r"},
  {86, "s"},
  {87, "t"},
  {88, "u"},
  {89, "v"},
  {90, "w"},
  {91, "x"},
  {92, "y"},
  {93, "z"},
  {94, "braceleft"},
  {95, "bar"},
  {96, "braceright"},
  {97, "asciitilde"},
  {98, "Adieresis"},
  {99, "Aring"},
  {100, "Ccedilla"},
  {101, "Eacute"},
  {102, "Ntilde"},
  {103, "Odieresis"},
  {104, "Udieresis"},
  {105, "aacute"},
  {106, "agrave"},
  {107, "acircumflex"},
  {108, "adieresis"},
  {109, "atilde"},
  {110, "aring"},
  {111, "ccedilla"},
  {112, "eacute"},
  {113, "egrave"},
  {114, "ecircumflex"},
  {115, "edieresis"},
  {116, "iacute"},
  {117, "igrave"},
  {118, "icircumflex"},
  {119, "idieresis"},
  {120, "ntilde"},
  {121, "oacute"},
  {122, "ograve"},
  {123, "ocircumflex"},
  {124, "odieresis"},
  {125, "otilde"},
  {126, "uacute"},
  {127, "ugrave"},
  {128, "ucircumflex"},
  {129, "udieresis"},
  {130, "dagger"},
  {131, "degree"},
  {132, "cent"},
  {133, "sterling"},
  {134, "section"},
  {135, "bullet"},
  {136, "paragraph"},
  {137, "germandbls"},
  {138, "registered"},
  {139, "copyright"},
  {140, "trademark"},
  {141, "acute"},
  {142, "dieresis"},
  {143, "notequal"},
  {144, "AE"},
  {145, "Oslash"},
  {146, "infinity"},
  {147, "plusminus"},
  {148, "lessequal"},
  {149, "greaterequal"},
  {150, "yen"},
  {151, "mu"},
  {152, "partialdiff"},
  {153, "summation"},
  {154, "product"},
  {155, "pi"},
  {156, "integral"},
  {157, "ordfeminine"},
  {158, "ordmasculine"},
  {159, "Omega"},
  {160, "ae"},
  {161, "oslash"},
  {162, "questiondown"},
  {163, "exclamdown"},
  {164, "logicalnot"},
  {165, "radical"},
  {166, "florin"},
  {167, "approxequal"},
  {168, "Delta"},
  {169, "guillemotleft"},
  {170, "guillemotright"},
  {171, "ellipsis"},
  {172, "nonbreakingspace"},
  {173, "Agrave"},
  {174, "Atilde"},
  {175, "Otilde"},
  {176, "OE"},
  {177, "oe"},
  {178, "endash"},
  {179, "emdash"},
  {180, "quotedblleft"},
  {181, "quotedblright"},
  {182, "quoteleft"},
  {183, "quoteright"},
  {184, "divide"},
  {185, "lozenge"},
  {186, "ydieresis"},
  {187, "Ydieresis"},
  {188, "fraction"},
  {189, "currency"},
  {190, "guilsinglleft"},
  {191, "guilsinglright"},
  {192, "fi"},
  {193, "fl"},
  {194, "daggerdbl"},
  {195, "periodcentered"},
  {196, "quotesinglbase"},
  {197, "quotedblbase"},
  {198, "perthousand"},
  {199, "Acircumflex"},
  {200, "Ecircumflex"},
  {201, "Aacute"},
  {202, "Edieresis"},
  {203, "Egrave"},
  {204, "Iacute"},
  {205, "Icircumflex"},
  {206, "Idieresis"},
  {207, "Igrave"},
  {208, "Oacute"},
  {209, "Ocircumflex"},
  {210, "apple"},
  {211, "Ograve"},
  {212, "Uacute"},
  {213, "Ucircumflex"},
  {214, "Ugrave"},
  {215, "dotlessi"},
  {216, "circumflex"},
  {217, "tilde"},
  {218, "macron"},
  {219, "breve"},
  {220, "dotaccent"},
  {221, "ring"},
  {222, "cedilla"},
  {223, "hungarumlaut"},
  {224, "ogonek"},
  {225, "caron"},
  {226, "Lslash"},
  {227, "lslash"},
  {228, "Scaron"},
  {229, "scaron"},
  {230, "Zcaron"},
  {231, "zcaron"},
  {232, "brokenbar"},
  {233, "Eth"},
  {234, "eth"},
  {235, "Yacute"},
  {236, "yacute"},
  {237, "Thorn"},
  {238, "thorn"},
  {239, "minus"},
  {240, "multiply"},
  {241, "onesuperior"},
  {242, "twosuperior"},
  {243, "threesuperior"},
  {244, "onehalf"},
  {245, "onequarter"},
  {246, "threequarters"},
  {247, "franc"},
  {248, "Gbreve"},
  {249, "gbreve"},
  {250, "Idotaccent"},
  {251, "Scedilla"},
  {252, "scedilla"},
  {253, "Cacute"},
  {254, "cacute"},
  {255, "Ccaron"},
  {256, "ccaron"},
  {257, "dcroat"}};

/*---------------------------------------------------------------------------*/

void Contour::smooth()
{
  std::vector<uint8_t>  flags_b;
  std::vector<int16_t>  xcoords_b;
  std::vector<int16_t>  ycoords_b;
  std::vector<uint16_t> shape_b;

  if(shape.size() > 2)
  {
    if(shape.back() != shape[shape.size() - 2])
    {
      flags.pop_back();
      shape.pop_back();
      xcoords.pop_back();
      ycoords.pop_back();
    }
  }

  for(size_t i = 0; i < flags.size(); i++)
  {
    if((flags[i] & 0x01) == 0x01)
    {
      flags_b.push_back(1);
      xcoords_b.push_back(xcoords[i]);
      ycoords_b.push_back(ycoords[i]);
      shape_b.push_back(shape[i]);
    }
    else
    {
      if(i == 0 || flags_b.back() == 1)
      {
        flags_b.push_back(0);
        xcoords_b.push_back(xcoords[i]);
        ycoords_b.push_back(ycoords[i]);
        shape_b.push_back(shape[i]);
      }
      else
      {
        flags_b.push_back(1);
        xcoords_b.push_back((xcoords[i] + xcoords[i-1])/2);
        ycoords_b.push_back((ycoords[i] + ycoords[i-1])/2);
        shape_b.push_back(shape[i]);

        flags_b.push_back(0);
        xcoords_b.push_back(xcoords[i]);
        ycoords_b.push_back(ycoords[i]);
        shape_b.push_back(shape[i]);
      }
    }

    if(i == flags.size() - 1 || shape[i] != shape[i + 1])
    {
      size_t shape_index = 0;
      while(shape_index < i)
      {
        if(shape[shape_index] == shape_b.back()) break;
        shape_index++;
      }

      flags_b.push_back(1);
      xcoords_b.push_back(xcoords[shape_index]);
      ycoords_b.push_back(ycoords[shape_index]);
      shape_b.push_back(shape[shape_index]);
    }
  }

  std::vector<int16_t>  xcoords_c;
  std::vector<int16_t>  ycoords_c;
  std::vector<uint16_t> shape_c;

  for(size_t i = 0; i < (flags_b.size() - 1); i++)
  {
    if(flags_b[i] == 1 && flags_b[i + 1] == 1)
    {
      xcoords_c.push_back(xcoords_b[i]);
      ycoords_c.push_back(ycoords_b[i]);
      shape_c.push_back(shape_b[i]);
    }

    if(flags_b[i] == 1 && flags_b[i + 1] == 0)
    {
      int p1x = xcoords_b[i];
      int p2x = xcoords_b[i + 1];
      int p3x = xcoords_b[i + 2];
      int p1y = ycoords_b[i];
      int p2y = ycoords_b[i + 1];
      int p3y = ycoords_b[i + 2];

      std::vector<float> frac;

      float filler = 0.1;

      while(filler < 1)
      {
        frac.push_back(filler);
        filler += 0.1;
      }

      for(size_t j = 0; j < frac.size(); j++)
      {
        float t = frac[j];
        xcoords_c.push_back((int) ((1 - t) * (1 - t) * p1x +
          2 * t * (1 - t) * p2x +
          t * t * p3x));
        ycoords_c.push_back((int) ((1 - t) * (1 - t) * p1y +
          2 * t * (1 - t) * p2y +
          t * t * p3y));
        shape_c.push_back(shape_b[i]);
      }
      i++;
    }
  }

  xcoords = xcoords_c;
  ycoords = ycoords_c;
  shape   = shape_c;
}

/*---------------------------------------------------------------------------*/

void Contour::transform(double a, double b, double c, double d, double e,
                        double f, double m, double n)
{
  for(size_t i = 0; i < xcoords.size(); i++)
  {
    xcoords[i] = m * (xcoords[i] * a/m + ycoords[i] * c/m + e);
    ycoords[i] = n * (xcoords[i] * b/n + ycoords[i] * d/n + f);
  }
}

/*---------------------------------------------------------------------------*/

TTFont::TTFont(const std::string& input_stream) :
  stream_(input_stream),
  it_(stream_.begin())
{
  ReadTables();

  ReadHead();
  ReadCMap();
  ReadMaxp();
  ReadLoca();
  ReadPost();
  ReadName();
  ReadOS2();
};

/*---------------------------------------------------------------------------*/

Glyf TTFont::ReadGlyf(uint16_t glyf_num)
{
  if(glyf_cache_.find(glyf_num) != glyf_cache_.end())
    return *(glyf_cache_[glyf_num]);

  GoToTable("glyf");

  it_ += loca_.offset_[glyf_num];
  glyf_cache_[glyf_num] = std::make_shared<Glyf>();

  if(loca_.length_[glyf_num] == 0)
  {
    glyf_cache_[glyf_num]->numberOfContours_ = 0;
    glyf_cache_[glyf_num]->xMin_             = 0;
    glyf_cache_[glyf_num]->yMin_             = 0;
    glyf_cache_[glyf_num]->xMax_             = 0;
    glyf_cache_[glyf_num]->yMax_             = 0;
    glyf_cache_[glyf_num]->contours_         = {Contour()};
  }
  else
  {
    glyf_cache_[glyf_num]->numberOfContours_ = GetInt16();
    glyf_cache_[glyf_num]->xMin_             = GetInt16();
    glyf_cache_[glyf_num]->yMin_             = GetInt16();
    glyf_cache_[glyf_num]->xMax_             = GetInt16();
    glyf_cache_[glyf_num]->yMax_             = GetInt16();
    glyf_cache_[glyf_num]->contours_.push_back(Contour());
  }

  if(glyf_cache_[glyf_num]->numberOfContours_ < 0)
    ReadCompoundGlyph(*glyf_cache_[glyf_num]);

  else if(glyf_cache_[glyf_num]->numberOfContours_ > 0)
    ReadSimpleGlyph(*glyf_cache_[glyf_num]);

  return *(glyf_cache_[glyf_num]);
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadCompoundGlyph(Glyf& result)
{
  uint16_t flags = MORE_COMPONENTS;

  while((flags & MORE_COMPONENTS) == MORE_COMPONENTS)
  {

    flags = GetUint16();
    uint16_t index = GetUint16();
    auto store = it_;

    Glyf component = ReadGlyf(index);

    it_ = store;

    double a = 1.0;
    double d = 1.0;
    double b = 0.0;
    double c = 0.0;
    double e = 0.0;
    double f = 0.0;

    if((flags & ARG_1_AND_2_ARE_WORDS) == ARG_1_AND_2_ARE_WORDS)
    {
      if((flags & ARGS_ARE_XY_VALUES) == ARGS_ARE_XY_VALUES)
      {
        e = GetInt16();
        f = GetInt16();
      }
      else
      {
        if(result.contours_.size() == 0)
          throw std::runtime_error("I don't understand");
        uint16_t comp_index = GetUint16();
        e = component.contours_[0].xcoords[comp_index];
        f = component.contours_[0].ycoords[comp_index];
      }
    }
    else
    {
      if((flags & ARGS_ARE_XY_VALUES) == ARGS_ARE_XY_VALUES)
      {
        uint16_t arg1and2 = GetInt16();
        e = ((int8_t)(arg1and2 >> 8));
        f = (int8_t) (arg1and2 & 0xff);
      }
      else
      {
        if(result.contours_.size() == 0)
          throw std::runtime_error("I don't understand");
        uint8_t comp_index = GetUint8();
        e = component.contours_[0].xcoords[comp_index];
        f = component.contours_[0].ycoords[comp_index];
      }
    }

    if((flags & WE_HAVE_A_SCALE) == WE_HAVE_A_SCALE)
    {
      a = GetF2Dot14();
      d = a;
    }
    else if((flags & WE_HAVE_AN_X_AND_Y_SCALE) ==  WE_HAVE_AN_X_AND_Y_SCALE)
    {
      a =  GetF2Dot14();
      d =  GetF2Dot14();
    }
    else if((flags & WE_HAVE_A_TWO_BY_TWO) == WE_HAVE_A_TWO_BY_TWO)
    {
      a =  GetF2Dot14();
      b =  GetF2Dot14();
      c =  GetF2Dot14();
      d =  GetF2Dot14();
    }

    double m = std::max(std::fabs(a), std::fabs(b));
    double n = std::max(std::fabs(c), std::fabs(d));

    if(std::fabs(std::fabs(a) - std::fabs(c)) < double(33) / double(65536))
    {
      m = 2 * m;
    }

    if(std::fabs(std::fabs(b) - std::fabs(d)) < double(33) / double(65536))
    {
      n = 2 * n;
    }

    component.contours_[0].transform(a, b, c, d, e, f, m, n);
    result.contours_.push_back(component.contours_[0]);
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadSimpleGlyph(Glyf& result)
{
  for(uint16_t i = 0; i < result.numberOfContours_; i++)
  {
    result.endPtsOfContours_.push_back(GetUint16());
  }

  result.instructionLength_ = GetUint16();

  for(uint16_t i = 0; i < result.instructionLength_; i++)
  {
    result.instructions_.push_back(GetUint8());
  }
  uint16_t shape_no = 1;

  for(uint16_t i = 0; i < result.endPtsOfContours_.size(); i++)
  {
    while(result.contours_[0].shape.size() <
          ((size_t) result.endPtsOfContours_[i] + 1))
    {
      result.contours_[0].shape.push_back(shape_no);
    }
    shape_no++;
  }

  while(result.contours_[0].flags.size() < result.contours_[0].shape.size())
  {
    uint8_t flag = GetUint8();
    result.contours_[0].flags.push_back(flag);

    if((flag & REPEAT_FLAG) == REPEAT_FLAG)
    {
      uint8_t n_repeats = GetUint8();
      while(n_repeats-- != 0) result.contours_[0].flags.push_back(flag);
    }
  }

  int16_t new_x = 0;
  int16_t new_y = 0;

  while(result.contours_[0].xcoords.size() <  result.contours_[0].shape.size())
  {
    uint8_t flag = result.contours_[0].flags[result.contours_[0].xcoords.size()];

    if((flag & X_SHORT_VECTOR) == X_SHORT_VECTOR)
    {
      if((flag & X_MODIFIER) == X_MODIFIER) new_x += (int16_t) GetUint8();
      else new_x -= (int16_t) GetUint8();
    }
    else
    {
      if((flag & X_MODIFIER) != X_MODIFIER) new_x += GetInt16();
    }
    result.contours_[0].xcoords.push_back(new_x);
  }

  while(result.contours_[0].ycoords.size() <  result.contours_[0].shape.size())
  {
    uint8_t flag = result.contours_[0].flags[result.contours_[0].ycoords.size()];

    if((flag & Y_SHORT_VECTOR) == Y_SHORT_VECTOR)
    {
      if((flag & Y_MODIFIER) == Y_MODIFIER) new_y += (int16_t) GetUint8();
      else new_y -= (int16_t) GetUint8();
    }
    else
    {
      if((flag & Y_MODIFIER) != Y_MODIFIER) new_y += GetInt16();
    }
    result.contours_[0].ycoords.push_back(new_y);
  }

  result.contours_[0].smooth();
}

/*---------------------------------------------------------------------------*/

void TTFont::GoToTable(std::string table_name) {
  int index = -1;
  for(size_t i = 0; i < table_of_tables_.size(); ++i)
  {
    if(table_of_tables_[i].table_name_ == table_name)
    {
      index = table_of_tables_[i].offset_;
    }
  }
  if(index == -1)
    throw std::runtime_error("Could not find table \"" + table_name +
                             "\" in font directory.");
  it_ = stream_.begin() + index;
}

/*---------------------------------------------------------------------------*/

bool TTFont::TableExists(std::string table_name) {
  int index = -1;
  for(size_t i = 0; i < table_of_tables_.size(); ++i)
  {
    if(table_of_tables_[i].table_name_ == table_name)
    {
      index = table_of_tables_[i].offset_;
    }
  }
  if(index == -1) return false;
  return true;
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadTables()
{
  scalar_type_    = GetUint32();
  num_tables_     = GetUint16();
  search_range_   = GetUint16();
  entry_selector_ = GetUint16();
  range_shift_    = GetUint16();

  for(unsigned i = 0; i < num_tables_; ++i)
    table_of_tables_.push_back(GetTTRow());

  std::string::const_iterator it_store = it_;

  for(unsigned i = 0; i < table_of_tables_.size(); ++i)
  {
    TTFRow this_table = table_of_tables_[i];
    if (this_table.table_name_ != "head")
    {
      uint32_t sum = 0;
  	  uint32_t nLongs = (this_table.length_ + 3) / 4;
  	  it_ = stream_.begin() + this_table.offset_;
  	  while (nLongs-- > 0) sum += GetUint32();
  	  if (sum != this_table.checksum_)
  	  {
  	    //throw std::runtime_error("Invalid checksum in font file.");
  	  }
    }
    it_ = it_store;
	}
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadMaxp()
{
  GoToTable("maxp");
  maxp_.version_               = GetFixed();
  maxp_.numGlyphs_             = GetUint16();
  maxp_.maxPoints_             = GetUint16();
  maxp_.maxContours_           = GetUint16();
  maxp_.maxComponentPoints_    = GetUint16();
  maxp_.maxComponentContours_  = GetUint16();
  maxp_.maxZones_              = GetUint16();
  maxp_.maxTwilightPoints_     = GetUint16();
  maxp_.maxStorage_            = GetUint16();
  maxp_.maxFunctionDefs_       = GetUint16();
  maxp_.maxInstructionDefs_    = GetUint16();
  maxp_.maxStackElements_      = GetUint16();
  maxp_.maxSizeOfInstructions_ = GetUint16();
  maxp_.maxComponentElements_  = GetUint16();
  maxp_.maxComponentDepth_     = GetUint16();
};

/*---------------------------------------------------------------------------*/

TTFRow TTFont::GetTTRow()
{
  TTFRow res;
  res.table_name_ = std::string(it_, it_ + 4);

  it_ += 4;
  res.checksum_   = GetUint32();
  res.offset_     = GetUint32();
  res.length_     = GetUint32();

  return res;
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadHead()
{
    GoToTable("head");

    head_.version            = GetFixed();
    head_.fontRevision       = GetFixed();
    head_.checksumAdjustment = GetUint32();
    head_.magicNumber        = GetUint32();
    head_.flags              = GetUint16();
    head_.unitsPerEm         = GetUint16();
    head_.created            = GetDate();
    head_.modified           = GetDate();
    head_.xMin               = GetFword();
    head_.yMin               = GetFword();
    head_.xMax               = GetFword();
    head_.yMax               = GetFword();
    head_.macStyle           = GetUint16();
    head_.lowestRecPPEM      = GetUint16();
    head_.fontDirectionHint  = GetInt16();
    head_.indexToLocFormat   = GetInt16();
    head_.glyphDataFormat    = GetInt16();

    if(head_.magicNumber != 0x5f0f3cf5)
      throw std::runtime_error("Incorrect magic number in font header");
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadLoca()
{
  GoToTable("loca");
  uint16_t n_entries = maxp_.numGlyphs_ + 1;
  uint16_t format = head_.indexToLocFormat;

  if(format == 0)
  {
    for(uint16_t i = 0; i < n_entries; i++)
    {
      loca_.glyph_.push_back(i);
      uint32_t next_offset = GetUint16() * 2;
      if(i == 1) loca_.length_.push_back(next_offset);
      if(i > 1) loca_.length_.push_back(next_offset - loca_.offset_.back());
      loca_.offset_.push_back(next_offset);
    }
  }

  if(format == 1)
  {
    for(uint16_t i = 0; i < n_entries; i++)
    {
      loca_.glyph_.push_back(i);
      uint32_t next_offset = GetUint32();
      if(i == 1) loca_.length_.push_back(next_offset);
      if(i > 1) loca_.length_.push_back(next_offset - loca_.offset_.back());
      loca_.offset_.push_back(next_offset);
    }
  }

  if(format > 1)
  {
    throw std::runtime_error("Invalid format number in loca table");
  }

  loca_.length_.push_back(0);
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadCMap()
{
  if(TableExists("cmap"))
  {
    GoToTable("cmap");

    auto cmap_begin = it_;

    if (GetUint16() != 0)
      throw std::runtime_error("cmap version not set to zero.");

    uint16_t n_tables = GetUint16();

    const std::map<uint16_t, std::string> windows_specific_map = {
      {0, "Windows Symbol"}, {1, "Windows Unicode (BMP only)"},
      {2, "Windows Shift-JIS"}, {3, "Windows PRC"}, {4, "Windows BigFive"},
      {5, "Windows Johab"}, {10, "Windows Unicode UCS-4"}};

    const std::map<uint16_t, std::string> unicode_specific_map = {
      {0, "Unicode Default"}, {1, "Unicode v1.1" }, {2, "Unicode ISO 10646" },
      {3, "Unicode v2 BMP only"},   {4, "Unicode v2" },
      {5, "Unicode Variations"}, {6, "Unicode Full"}};

    auto left_off = it_;

    for (uint16_t i = 0; i < n_tables; ++i)
    {
      it_ = left_off;
      CMap entry;
      uint16_t platform = GetUint16();
      uint16_t id       = GetUint16();
      if (platform == 0)
      {
        auto found = unicode_specific_map.find(id);
        if (found != unicode_specific_map.end()) entry.encoding_ = found->second;
      }
      if (platform == 3)
      {
        auto found = windows_specific_map.find(id);
        if (found != windows_specific_map.end()) entry.encoding_ = found->second;
      }
      if (platform == 1) entry.encoding_ = "Mac";
      if (platform == 2 || platform > 3)
      {
        throw std::runtime_error("Unrecognised encoding in cmap directory.");
      }

      uint16_t offset = GetUint32();
      left_off = it_;

      it_ = cmap_begin + offset;
      entry.format_   = GetUint16();


      switch(entry.format_)
      {
        case 0  : HandleFormat0(entry);  break;
        case 2  : HandleFormat2(entry);  break;
        case 4  : HandleFormat4(entry);  break;
        case 6  : HandleFormat6(entry);  break;
        case 8  : HandleFormat8(entry);  break;
        case 10 : HandleFormat10(entry); break;
        case 12 : HandleFormat12(entry); break;
        case 13 : HandleFormat13(entry); break;
        case 14 : HandleFormat14(entry); break;

        default : throw std::runtime_error("Unknown subtable format in cmap.");
      }
      cmap_dir_.push_back(entry);
    }
  }
  else
  {
    cmap_dir_.emplace_back(CMap());

    cmap_dir_.back().encoding_ = "Unicode Default";
    cmap_dir_.back().format_   = 0;
    for (uint16_t j = 0; j <= 256; ++j)
    {
      cmap_dir_.back().cmap_[j] = j;
    }
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat0(CMap& entry)
{
  it_ += 4; // skip unused length and language variables

  for (uint16_t i = 0; i < 256; ++i)
  {
    entry.cmap_[i] = (uint16_t)(uint8_t) *it_++;
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat2(CMap& entry)
{
  it_ += 4; // Skip 2 unused variables

  std::vector<uint16_t> subheaderKeys;

  for (uint16_t i = 0; i < 256; ++i) subheaderKeys.push_back(GetUint16());

  auto subheaders_start = it_;

  for (uint16_t i = 0; i < 256; ++i)
  {
    it_ = subheaders_start + subheaderKeys[i];

    uint16_t firstCode     = GetUint16();
	  uint16_t entryCount    = GetUint16();
	  int16_t  idDelta       = GetInt16();
	  uint16_t idRangeOffset = GetUint16();

	  for(uint16_t j = 0; j < entryCount - firstCode; j++)
	  {
	    uint8_t p = (uint8_t)*(it_ + idRangeOffset + j);
	    uint16_t glyphid = (p + idDelta) % 65536;
	    entry.cmap_[i << 8 | (firstCode + j)] = glyphid;
	  }
  }

}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat4(CMap& entry)
{
  it_ += 4; // Skip 2 unused variables

  uint16_t seg_count = GetUint16() / 2;

  // search_range, entry_selector, range_shift are all unused here and
  // therefore we just skip on 6 bytes
  it_ += 6;

  std::vector<uint16_t> end_code, start_code;
  std::vector<int16_t> id_delta;
  for (uint16_t i = 0; i < seg_count; ++i) end_code.push_back(GetUint16());
  if(GetUint16() != 0) throw std::runtime_error("Reserve pad != 0.");
  for (uint16_t i = 0; i < seg_count; ++i) start_code.push_back(GetUint16());
  for (uint16_t i = 0; i < seg_count; ++i) id_delta.push_back(GetInt16());

  for (uint16_t i = 0; i < seg_count; ++i)
  {
    if(end_code[i] == 0xffff) break;
    uint16_t range_offset = GetUint16();

    if(range_offset == 0)
    {
      for (uint16_t j = start_code[i]; j <= end_code[i]; ++j)
      {
        entry.cmap_[j] = (j + id_delta[i]) % 65536;
      }
    }
    else
    {
      auto it_store = it_;

      it_ += (range_offset - 2);

      for (uint16_t j = start_code[i]; j <= end_code[i]; ++j)
      {
        entry.cmap_[j] = GetUint16();
      }
      it_ = it_store;
    }

  }

}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat6(CMap& entry)
{
  it_ += 4; // Skip 2 unused variables

  auto first_entry = GetUint16();
  auto num_entries = GetUint16();

  for (uint16_t i = 0; i < num_entries; ++i )
  {
    entry.cmap_[first_entry + i] = GetUint16();
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat8(CMap& entry)
{
  it_ += 10; // two reserved bytes plus 2 unused uint32_t

  // **Unimplemented**
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat10(CMap& entry)
{
  it_ += 10; // two reserved bytes plus 2 unused uint32_t

  uint32_t start_char = GetUint32();
  uint32_t num_chars  = GetUint32();

  for (uint32_t i = 0; i < num_chars; ++i )
  {
    entry.cmap_[start_char + i] = GetUint32();
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat12(CMap& entry)
{
  it_ += 10; // two reserved bytes plus 2 unused uint32_t

  uint32_t nGroups  = GetUint32();

  for(uint32_t i = 0; i < nGroups; i++)
  {
    uint32_t startCode  = GetUint32();
    uint32_t endCode    = GetUint32();
    uint32_t startGlyph = GetUint32();

    for(uint32_t j = 0; j <= (endCode - startCode); j++)
    {
      entry.cmap_[startCode + j] = startGlyph + j;
    }

  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat13(CMap& entry)
{
  it_ += 10; // two reserved bytes plus 2 unused uint32_t

  uint32_t nGroups  = GetUint32();

  for(uint32_t i = 0; i < nGroups; i++)
  {
    uint32_t startCode  = GetUint32();
    uint32_t endCode    = GetUint32();
    uint32_t startGlyph = GetUint32();

    for(uint32_t j = 0; j <= (endCode - startCode); j++)
    {
      entry.cmap_[startCode + j] = startGlyph;
    }
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat14(CMap& entry)
{

}

/*---------------------------------------------------------------------------*/

void TTFont::ReadPost()
{
  if(TableExists("post"))
  {
    GoToTable("post");
    uint32_t version = GetUint32();
    post_.italic_angle = GetFixed();
    post_.UnderlinePosition = GetFword();
    post_.UnderlineThickness = GetFword();
    post_.IsFixedPitch = GetUint32();
    post_.MinMemType42 = GetUint32();
    post_.MaxMemType42 = GetUint32();
    post_.MinMemType1  = GetUint32();
    post_.MaxMemType1  = GetUint32();

    if(version == 0x00010000)
    {
      post_.version = 1.0;
      post_.mapping = postscript_glyphs;
    }

    if(version == 0x00020000)
    {
      post_.version = 2.0;
      uint16_t n_glyphs = GetUint16();
      std::vector<uint16_t> indexes;

      for(uint16_t i = 0; i < n_glyphs; i++)
      {
        indexes.push_back(GetUint16());
      }
      for(uint16_t i = 0; i < n_glyphs; i++)
      {
        if(indexes[i] > 257)
          post_.mapping[indexes[i] - 258] = GetPascalString();
        else post_.mapping[indexes[i]] = postscript_glyphs[indexes[i]];
      }
    }

    if(version == 0x00025000)
    {
      post_.version = 2.5;
      uint16_t n_glyphs = GetUint16();
      for(uint16_t i = 0; i < n_glyphs; i++)
      {
        post_.mapping[i] = postscript_glyphs[i + (int8_t) GetUint8()];
      }
    }

    if(version == 0x00030000)
    {
      post_.version = 3.0;
      post_.mapping[0] = ".notdef";
    }

    if(version == 0x00040000)
    {
      post_.version = 4.0;
      throw std::runtime_error("Format 4 post table fonts are not supported.");
    }
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadName()
{
  if(TableExists("name"))
  {
    GoToTable("name");
    auto table_start = it_;

    it_ += 2; // skip format which isn't used (2 bytes)
    uint16_t count  = GetUint16();
    uint16_t string_offset = GetUint16();

    for(uint16_t i = 0; i < count; i++)
    {
      name_.platformID.push_back(GetUint16());
      name_.platformSpecificID.push_back(GetUint16());
      name_.languageID.push_back(GetUint16());
      name_.nameID.push_back(GetUint16());
      uint16_t length = GetUint16();
      uint16_t offset = GetUint16();
      auto begin = table_start + string_offset + offset;
      name_.text.push_back(std::string(begin, begin + length));
    }
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadOS2()
{
  if(TableExists("OS/2"))
  {
    GoToTable("OS/2");

    OS2_.version = GetUint16();
    OS2_.xAvgCharWidth = GetInt16();
    OS2_.usWeightClass = GetUint16();
    uint16_t width_index = GetUint16();

    std::vector<std::string> widths = { "Ultra-condensed",
                                        "Extra-condensed",
                                        "Condensed",
                                        "Semi-condensed",
                                        "Medium (normal)",
                                        "Semi-expanded",
                                        "Expanded",
                                        "Extra-expanded",
                                        "Ultra-expanded"
                                      };

    std::string width = "Unknown";

    if(width_index > 0 && width_index < 10)
    {
      OS2_.usWidthClass = widths[width_index - 1];
    }

    switch(GetUint16())
    {
      case 0x0000 : OS2_.fsType = "Installable embedding";        break;
      case 0x0002 : OS2_.fsType = "Restricted licence embedding"; break;
      case 0x0004 : OS2_.fsType = "Preview & print embedding";    break;
      case 0x0008 : OS2_.fsType = "Editable embedding";           break;
      case 0x0100 : OS2_.fsType = "No subset embedding";          break;
      case 0x0200 : OS2_.fsType = "Bitmap embedding only";        break;
      default     : OS2_.fsType = "Unknown";                      break;
    }

    OS2_.ySubscriptXSize     = GetInt16();
    OS2_.ySubscriptYSize     = GetInt16();
    OS2_.ySubscriptXOffset   = GetInt16();
    OS2_.ySubscriptYOffset   = GetInt16();
    OS2_.ySuperscriptXSize   = GetInt16();
    OS2_.ySuperscriptYSize   = GetInt16();
    OS2_.ySuperscriptXOffset = GetInt16();
    OS2_.ySuperscriptYOffset = GetInt16();
    OS2_.yStrikeoutSize      = GetInt16();
    OS2_.yStrikeoutPosition  = GetInt16();
    OS2_.sFamilyClass        = GetInt16();

    for(int i = 0; i < 10; i++) OS2_.panose.push_back(GetUint8());
    for(int i = 0; i < 4;  i++) OS2_.ulUnicodeRange.push_back(GetUint32());

    OS2_.achVendID = std::string(it_, it_ + 4);
    it_ += 4;

     std::vector<std::string> formats = {  "italic",
                                           "underscore",
                                           "negative",
                                           "outlined",
                                           "strikeout",
                                           "bold",
                                           "regular",
                                           "use typography metrics",
                                           "wws",
                                           "oblique"
                                        };

     OS2_.fsSelection = "Unknown";

     uint16_t format = GetUint16();

     for(int i = 0; i < 9; i++)
     {
       if((format >> i) == 1) OS2_.fsSelection = formats[i];
     }

    OS2_.fsFirstCharIndex = GetUint16();
    OS2_.fsLastCharIndex  = GetUint16();
  }
}
