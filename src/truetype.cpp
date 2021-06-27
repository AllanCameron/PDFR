#include "truetype.h"

using namespace std;
using namespace Rcpp;

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
        if(result.contours_.size() == 0) throw std::runtime_error("I don't understand");
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
        if(result.contours_.size() == 0) throw std::runtime_error("I don't understand");
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

    for (uint16_t i = 0; i < n_tables; ++i)
    {
      std::string encoding;
      uint16_t platform = GetUint16();
      uint16_t id = GetUint16();
      if (platform == 0)
      {
        auto entry = unicode_specific_map_s.find(id);
        if (entry != unicode_specific_map_s.end()) encoding = entry->second;
      }
      if (platform == 3)
      {
        auto entry = windows_specific_map_s.find(id);
        if (entry != windows_specific_map_s.end()) encoding = entry->second;
      }
      if (platform == 1) encoding = "Mac";
      if (platform == 2 || platform > 3)
      {
        throw std::runtime_error("Unrecognised encoding in cmap directory.");
      }
      uint16_t offset = GetUint32();
      cmap_directory_.emplace_back(CMapDirectory(platform, id, offset, encoding));
    }

    for (auto& entry : cmap_directory_)
    {
      it_ = cmap_begin + entry.offset_;
      entry.format_ = GetUint16();
      if (entry.format_ > 7)
      {
        if (GetUint16() != 0) throw std::runtime_error("Unknown cmap table format.");
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
    cmap_directory_.emplace_back(CMapDirectory(0, 0, 0, "Unicode Default"));
    for (uint16_t j = 0; j <= 256; ++j)
    {
      cmap_directory_.back().cmap_[j] = j;
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

void TTFont::HandleFormat8(CMapDirectory& entry)
{

}

void TTFont::HandleFormat10(CMapDirectory& entry)
{

}

void TTFont::HandleFormat12(CMapDirectory& entry)
{

}

void TTFont::HandleFormat13(CMapDirectory& entry)
{

}

void TTFont::HandleFormat14(CMapDirectory& entry)
{

}

/*---------------------------------------------------------------------------*/

DataFrame TTFont::GetTable()
{
    CharacterVector table;
    NumericVector   offset;
    NumericVector   checksum;
    NumericVector   length;

    for(size_t i = 0; i < table_of_tables_.size(); i++)
    {
      table.push_back(table_of_tables_[i].table_name_);
      offset.push_back(table_of_tables_[i].offset_);
      checksum.push_back(table_of_tables_[i].checksum_);
      length.push_back(table_of_tables_[i].length_);
    }

    DataFrame result = Rcpp::DataFrame::create(
                        Named("table")    = table,
                        Named("offset")   = offset,
                        Named("length")   = length,
                        Named("checksum") = checksum);

    return result;
  };

/*---------------------------------------------------------------------------*/

List TTFont::GetHead()
{
    return List::create(Named("checksumAdjustment") = head_.checksumAdjustment,
                        Named("created") = head_.created,
                        Named("flags") = head_.flags,
                        Named("fontDirectionHint") = head_.fontDirectionHint,
                        Named("fontRevision") = head_.fontRevision,
                        Named("glyphDataFormat") = head_.glyphDataFormat,
                        Named("indexToLocFormat") = head_.indexToLocFormat,
                        Named("lowestRecPPEM") = head_.lowestRecPPEM,
                        Named("macStyle") = head_.macStyle,
                        Named("magicNumber") = head_.magicNumber,
                        Named("modified") = head_.modified,
                        Named("unitsPerEm") = head_.unitsPerEm,
                        Named("version") = head_.version,
                        Named("xMax") = head_.xMax,
                        Named("xMin") = head_.xMin,
                        Named("yMax") = head_.yMax,
                        Named("yMin") = head_.yMin);
};

/*---------------------------------------------------------------------------*/

List TTFont::GetCMap()
{
  List result = List::create();
  for(auto j : cmap_directory_)
  {
    std::vector<uint16_t> key, value;
    for(auto i : j.cmap_)
    {
      key.push_back(i.first);
      value.push_back(i.second);
    }
    result.push_back(List::create(Named("format") = j.format_,
                                  Named("first") = key,
                                  Named("second") = value));
  }
  return result;
}

/*---------------------------------------------------------------------------*/

List TTFont::GetMaxp()
{
  return List::create(
    Named("version")               = maxp_.version_,
    Named("numGlyphs")             = maxp_.numGlyphs_,
    Named("maxPoints")             = maxp_.maxPoints_,
    Named("maxContours")           = maxp_.maxContours_,
    Named("maxComponentPoints")    = maxp_.maxComponentPoints_,
    Named("maxComponentContours")  = maxp_.maxComponentContours_,
    Named("maxZones")              = maxp_.maxZones_,
    Named("maxTwilightPoints")     = maxp_.maxTwilightPoints_,
    Named("maxStorage")            = maxp_.maxStorage_,
    Named("maxFunctionDefs")       = maxp_.maxFunctionDefs_,
    Named("maxInstructionDefs")    = maxp_.maxInstructionDefs_,
    Named("maxStackElements")      = maxp_.maxStackElements_,
    Named("maxSizeOfInstructions") = maxp_.maxSizeOfInstructions_,
    Named("maxComponentElements")  = maxp_.maxComponentElements_,
    Named("maxComponentDepth")     = maxp_.maxComponentDepth_);
}

/*---------------------------------------------------------------------------*/

DataFrame TTFont::GetLoca()
{
  return DataFrame::create(Named("glyph")  = loca_.glyph_,
                           Named("offset") = loca_.offset_,
                           Named("length") = loca_.length_);
}

