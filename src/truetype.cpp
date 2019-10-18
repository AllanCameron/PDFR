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

void TTFont::ShowTables()
{
  std::cout << "table\toffset\tlength" << std::endl;
  std::cout << "-----\t------\t------" << std::endl;
  for (unsigned i = 0; i < table_of_tables_.size(); ++i)
  {
    (table_of_tables_[i]).Print();
  }
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
  	    throw std::runtime_error("Invalid checksum in font file.");
  	  }
    }
    it_ = it_store;
	}
  ShowTables();
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
  std::cout << "\n\nPlatform\tid\toffset\tname" << endl;
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
    cmap_directory.emplace_back(CMapDirectory(platform, id, offset));
    cout << cmap_directory.back().platform_id_ << "\t\t"
         << cmap_directory.back().specific_id_ << "\t"
         << cmap_directory.back().offset_ << "\t"
         << encoding << std::endl;
  }
  std::cout << "\n\n" << std::endl;

  for (auto& entry : cmap_directory)
  {
    it_ = cmap_begin + entry.offset_;
    uint16_t format = GetUint16();
    if (format > 7)
    {
      if (GetUint16() != 0) throw runtime_error("Unknown cmap table format.");
    }
    uint16_t length = GetUint16();
    uint16_t language = GetUint16();
    std::cout << "Entry with offset " << entry.offset_
              << " has format " << format << ", length " << length
              << ", and language " << language << endl;
  }
}



