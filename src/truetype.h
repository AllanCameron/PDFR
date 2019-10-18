#include<vector>
#include<numeric>
#include<algorithm>
#include<iostream>
#include <stdint.h>
#include <stdexcept>
#include<Rcpp.h>

struct TTFRow
{
  std::string table_name_;
  uint32_t checksum_;
  uint32_t offset_;
  uint32_t length_;
  inline void Print() { std::cout << table_name_ << "\t"
                                  << offset_     << "\t"
                                  << length_     << std::endl;}
};

struct CMapDirectory
{
  CMapDirectory(uint16_t platform_id, uint16_t specific_id, uint32_t offset) :
  platform_id_(platform_id), specific_id_(specific_id), offset_(offset) {};
  uint16_t platform_id_;
  uint16_t specific_id_;
  uint32_t offset_;
};

class TTFont
{
public:
  TTFont(const std::string& p_stream);

  void ShowTables();
  uint16_t GetUint16();
  uint32_t GetUint32();
  TTFRow GetTTRow();

private:
  void ReadTables();
  void ReadCMap();
  std::string stream_;
  std::string::const_iterator it_;
  std::vector<TTFRow> table_of_tables_;
  uint32_t scalar_type_;
  uint16_t num_tables_;
  uint16_t search_range_;
  uint16_t entry_selector_;
  uint16_t range_shift_;
  static const std::map<uint16_t, std::string> windows_specific_map_s;
  static const std::map<uint16_t, std::string> unicode_specific_map_s;
};
