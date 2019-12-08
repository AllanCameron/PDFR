#include "truetype.h"

using namespace std;

const std::map<uint16_t, std::string> TTFont::windows_specific_map_s = {
  {0, "Windows Symbol"}, {1, "Windows Unicode (BMP only)"},
  {2, "Windows Shift-JIS"}, {3, "Windows PRC"}, {4, "Windows BigFive"},
  {5, "Windows Johab"}, {10, "Windows Unicode UCS-4"}};

const std::map<uint16_t, std::string> TTFont::unicode_specific_map_s = {
  {0, "Unicode Default"}, {1, "Unicode v1.1" }, {2, "Unicode ISO 10646" },
  {3, "Unicode v2 BMP only"},   {4, "Unicode v2" },   {5, "Unicode Variations"},
  {6, "Unicode Full"}};


TTFont::TTFont(const std::string& p_stream) :
stream_(p_stream),
it_(stream_.begin())
{
  ReadTables();
};



uint16_t TTFont::GetUint16()
{
  if (stream_.end() - it_ < 2)
    throw runtime_error("Insufficient bytes for GetUint16()");
  uint16_t res = 0;
  res += 256 * ((uint16_t) (uint8_t) *it_++);
  res += (uint16_t) (uint8_t) *it_++;
  return res;
};

uint32_t TTFont::GetUint32()
{
  if (stream_.end() - it_ < 4)
    throw runtime_error("Insufficient bytes for GetUint32()");
  uint32_t res = GetUint16() << 16;
  res += GetUint16();
  return res;
};

void TTFont::ReadTables()
{
  scalar_type_ = GetUint32();
  num_tables_ = GetUint16();
  search_range_ = GetUint16();
  entry_selector_ = GetUint16();
  range_shift_ = GetUint16();
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
  ReadCMap();
}

TTFRow TTFont::GetTTRow()
{
  TTFRow res;
  res.table_name_ = std::string(it_, it_ + 4);
  it_ += 4;
  res.checksum_ = GetUint32();
  res.offset_ = GetUint32();
  res.length_ = GetUint32();
  return res;
}

void TTFont::ReadCMap()
{
  uint16_t index = 0;
  for (unsigned i = 0; i < table_of_tables_.size(); ++i)
  {
    if (table_of_tables_[i].table_name_ == "cmap")
    {
      index = i; break;
    }

    if (i == table_of_tables_.size() - 1 && index == 0)
    {
      throw runtime_error("No cmap table in font directory.");
    }
  }
  auto cmap_begin = stream_.begin() + table_of_tables_[index].offset_;
  it_ = cmap_begin;

  if (GetUint16() != 0) throw runtime_error("cmap version not set to zero.");
  uint16_t n_tables = GetUint16();

  std::vector<CMapDirectory> cmap_directory;

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
      throw runtime_error("Unrecognised encoding in cmap directory.");
    }
    uint16_t offset = GetUint32();
    cmap_directory.emplace_back(CMapDirectory(platform, id, offset, encoding));
  }

  for (auto& entry : cmap_directory)
  {
    it_ = cmap_begin + entry.offset_;
    entry.format_ = GetUint16();
    if (entry.format_ > 7)
    {
      if (GetUint16() != 0) throw runtime_error("Unknown cmap table format.");
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
      default : throw runtime_error("Unknown subtable format in cmap.");
    }

  }
}

void TTFont::HandleFormat0(CMapDirectory& p_entry)
{
  for (uint16_t i = 0; i < 256; ++i)
  {
    p_entry.cmap_[i] = (uint16_t)(uint8_t) *it_++;
  }
}

void TTFont::HandleFormat2(CMapDirectory& p_entry)
{
  std::vector<uint16_t> sub_header_keys;
  for (uint16_t i = 0; i < 256; ++i)
  {
    uint16_t k = GetUint16()/8;
    if (k) k = 0; // Unused variable - this kills warnings
  }
}

void TTFont::HandleFormat4(CMapDirectory& p_entry)
{
  uint16_t seg_count = GetUint16() / 2;
  // search_range, entry_selector, range_shift are all unused here and
  // therefore we just skip on 6 bytes
  it_ += 6;
  std::vector<uint16_t> end_code, start_code, id_delta, range_offset;
  for (uint16_t i = 0; i < seg_count; ++i) end_code.push_back(GetUint16());
  if(GetUint16() != 0) throw runtime_error("Reserve pad != 0.");
  for (uint16_t i = 0; i < seg_count; ++i) start_code.push_back(GetUint16());
  for (uint16_t i = 0; i < seg_count; ++i) id_delta.push_back(GetUint16());
  for (uint16_t i = 0; i < seg_count; ++i) range_offset.push_back(GetUint16());
  for (uint16_t i = 0; i < seg_count; ++i)
  {
    if (end_code[i] == 0xffff) break;
    for (uint16_t j = start_code[i]; j <= end_code[i]; ++j)
    {
      p_entry.cmap_[(j + id_delta[i]) % 65536] = j;
    }
  }

}

void TTFont::HandleFormat6(CMapDirectory& p_entry)
{
  auto first_entry = GetUint16();
  auto num_entries = GetUint16();
  for (uint16_t i = 0; i < num_entries; ++i )
  {
    p_entry.cmap_[first_entry + i] = GetUint16();
  }
}

void TTFont::HandleFormat8(CMapDirectory& p_entry)
{

}

void TTFont::HandleFormat10(CMapDirectory& p_entry)
{

}

void TTFont::HandleFormat12(CMapDirectory& p_entry)
{

}

void TTFont::HandleFormat13(CMapDirectory& p_entry)
{

}

void TTFont::HandleFormat14(CMapDirectory& p_entry)
{

}
