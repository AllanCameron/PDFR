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
#include "truetype.h"


/*---------------------------------------------------------------------------*/

void Contour::smooth()
{
  std::vector<uint8_t>  flags_b;
  std::vector<int16_t>  xcoords_b;
  std::vector<int16_t>  ycoords_b;
  std::vector<uint16_t> shape_b;

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
  shape = shape_c;
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
  ReadHeadTable();
  ReadCMap();
  ReadMaxp();
  ReadLoca();
};

/*---------------------------------------------------------------------------*/

Glyf TTFont::ReadGlyf(uint16_t glyf_num)
{
  GoToTable("glyf");

  it_ += loca_.offset_[glyf_num];
  Glyf result;
  result.numberOfContours_ = GetInt16();
  result.xMin_ =  GetInt16();
  result.yMin_ =  GetInt16();
  result.xMax_ =  GetInt16();
  result.yMax_ =  GetInt16();
  result.contours_.push_back(Contour());

  if(result.numberOfContours_ < 0)
    ReadCompoundGlyph(result);
  else
    ReadSimpleGlyph(result);

  return result;
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
        uint16_t this_compound_index = GetUint16();
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
        uint8_t this_compound_index = GetUint8();
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
    if(std::fabs(std::fabs(a) - std::fabs(c)) < double(33)/double(65536))
    {
      m = 2 * m;
    }
    if(std::fabs(std::fabs(b) - std::fabs(d)) < double(33)/double(65536))
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
  int64_t macTime = GetUint32() * 0x100000000 + GetUint32();
  int64_t utcTime = macTime * 1000 - 2080166400000;
  return utcTime;
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
  	    throw std::runtime_error("Invalid checksum in font file.");
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
  res.checksum_ = GetUint32();
  res.offset_   = GetUint32();
  res.length_   = GetUint32();
  return res;
}

/*---------------------------------------------------------------------------*/

void TTFont::ReadHeadTable()
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

    for (uint16_t i = 0; i < n_tables; ++i)
    {
      std::string encoding;
      uint16_t platform = GetUint16();
      uint16_t id = GetUint16();
      if (platform == 0)
      {
        auto entry = unicode_specific_map.find(id);
        if (entry != unicode_specific_map.end()) encoding = entry->second;
      }
      if (platform == 3)
      {
        auto entry = windows_specific_map.find(id);
        if (entry != windows_specific_map.end()) encoding = entry->second;
      }
      if (platform == 1) encoding = "Mac";
      if (platform == 2 || platform > 3)
      {
        throw std::runtime_error("Unrecognised encoding in cmap directory.");
      }
      uint16_t offset = GetUint32();
      cmap_dir_.emplace_back(CMapDirectory(platform, id, offset, encoding));
    }

    for (auto& entry : cmap_dir_)
    {
      it_ = cmap_begin + entry.offset_;
      entry.format_ = GetUint16();
      if (entry.format_ > 7)
      {
        if (GetUint16() != 0)
          throw std::runtime_error("Unknown cmap table format.");
      }
      entry.length_ = GetUint16();
      uint16_t language = GetUint16();
      if (language) language = 0; // Language is unused variable - removes warning

      switch(entry.format_)
      {
        case 0 : HandleFormat0(entry); break;
        case 2 :  HandleFormat2(entry); break;
        case 4 : HandleFormat4(entry); break;
        case 6 : HandleFormat6(entry); break;
        case 8 : HandleFormat8(entry); break;
        case 10 : HandleFormat10(entry); break;
        case 12 : HandleFormat12(entry); break;
        case 13 : HandleFormat13(entry); break;
        case 14 : HandleFormat14(entry); break;
        default : throw std::runtime_error("Unknown subtable format in cmap.");
      }

    }
  }
  else
  {
    cmap_dir_.emplace_back(CMapDirectory(0, 0, 0, "Unicode Default"));
    for (uint16_t j = 0; j <= 256; ++j)
    {
      cmap_dir_.back().cmap_[j] = j;
    }
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat0(CMapDirectory& entry)
{
  for (uint16_t i = 0; i < 256; ++i)
  {
    entry.cmap_[i] = (uint16_t)(uint8_t) *it_++;
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat2(CMapDirectory& entry)
{
  std::vector<uint16_t> sub_header_keys;
  for (uint16_t i = 0; i < 256; ++i)
  {
    uint16_t k = GetUint16()/8;
    if (k) k = 0; // Unused variable - this kills warnings
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat4(CMapDirectory& entry)
{
  uint16_t seg_count = GetUint16() / 2;
  // search_range, entry_selector, range_shift are all unused here and
  // therefore we just skip on 6 bytes
  it_ += 6;
  std::vector<uint16_t> end_code, start_code, id_delta, range_offset;
  for (uint16_t i = 0; i < seg_count; ++i) end_code.push_back(GetUint16());
  if(GetUint16() != 0) throw std::runtime_error("Reserve pad != 0.");
  for (uint16_t i = 0; i < seg_count; ++i) start_code.push_back(GetUint16());
  for (uint16_t i = 0; i < seg_count; ++i) id_delta.push_back(GetUint16());
  for (uint16_t i = 0; i < seg_count; ++i) range_offset.push_back(GetUint16());

  for (uint16_t i = 0; i < start_code.size(); ++i)
  {
    if (end_code[i] == 0xffff) break;
    for (uint16_t j = start_code[i]; j <= end_code[i]; ++j)
    {
      entry.cmap_[j] = (j + id_delta[i]) % 65536;
    }
  }

}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat6(CMapDirectory& entry)
{
  auto first_entry = GetUint16();
  auto num_entries = GetUint16();
  for (uint16_t i = 0; i < num_entries; ++i )
  {
    entry.cmap_[first_entry + i] = GetUint16();
  }
}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat8(CMapDirectory& entry)
{

}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat10(CMapDirectory& entry)
{

}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat12(CMapDirectory& entry)
{

}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat13(CMapDirectory& entry)
{

}

/*---------------------------------------------------------------------------*/

void TTFont::HandleFormat14(CMapDirectory& entry)
{

}

/*---------------------------------------------------------------------------*/

