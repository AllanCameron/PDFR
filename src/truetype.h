
#ifndef PDFR_TTF
/*---------------------------------------------------------------------------*/
#define PDFR_TTF

#include<Rcpp.h>
#include<map>
#include<vector>
#include<numeric>
#include<algorithm>
#include<iostream>
#include <stdint.h>
#include <stdexcept>

/*---------------------------------------------------------------------------*/

typedef int16_t Fword;
typedef float Fixed;
typedef int64_t Date_type;

/*---------------------------------------------------------------------------*/

enum flagbits {
  ON_CURVE_POINT = 0x01,
  X_SHORT_VECTOR = 0x02,
  Y_SHORT_VECTOR = 0x04,
  REPEAT_FLAG    = 0x08,
  X_MODIFIER     = 0x10,
  Y_MODIFIER     = 0x20,
  OVERLAP_SIMPLE = 0x40
};

enum composites {
  ARG_1_AND_2_ARE_WORDS    = 0x01,
  ARGS_ARE_XY_VALUES       = 0x02,
  ROUND_XY_TO_GRID         = 0x04,
  WE_HAVE_A_SCALE          = 0x08,
  OBSOLETE                 = 0x10,
  MORE_COMPONENTS          = 0x20,
  WE_HAVE_AN_X_AND_Y_SCALE = 0x40,
  WE_HAVE_A_TWO_BY_TWO     = 0x80,
  WE_HAVE_INSTRUCTIONS     = 0x100,
  USE_MY_METRICS           = 0x200,
  OVERLAP_COMPOUND         = 0x400
};

/*---------------------------------------------------------------------------*/

struct TTFRow
{
  std::string table_name_;
  uint32_t    checksum_;
  uint32_t    offset_;
  uint32_t    length_;
};

/*---------------------------------------------------------------------------*/

struct HeadTable
{
  Fixed     version;
  Fixed     fontRevision;
  uint32_t  checksumAdjustment;
  uint32_t  magicNumber;
  uint16_t  flags;
  uint16_t  unitsPerEm;
  Date_type created;
  Date_type modified;
  Fword     xMin;
  Fword     yMin;
  Fword     xMax;
  Fword     yMax;
  uint16_t  macStyle;
  uint16_t  lowestRecPPEM;
  int16_t   fontDirectionHint;
  int16_t   indexToLocFormat;
  int16_t   glyphDataFormat;
};

/*---------------------------------------------------------------------------*/

struct CMapDirectory
{
  CMapDirectory(uint16_t    platform_id,
                uint16_t    specific_id,
                uint32_t    offset,
                std::string encoding) :
    platform_id_(platform_id),
    specific_id_(specific_id),
    offset_(offset),
    encoding_(encoding){};

  uint16_t                     platform_id_;
  uint16_t                     specific_id_;
  uint32_t                     offset_;
  std::string                  encoding_;
  uint16_t                     format_;
  uint16_t                     length_;
  std::map<uint16_t, uint16_t> cmap_;
};

/*---------------------------------------------------------------------------*/

struct Maxp
{
  Fixed 	  version_;
  uint16_t 	numGlyphs_; 	// the number of glyphs in the font
  uint16_t 	maxPoints_; 	// points in non-compound glyph
  uint16_t 	maxContours_; 	// contours in non-compound glyph
  uint16_t 	maxComponentPoints_; // 	points in compound glyph
  uint16_t 	maxComponentContours_; // contours in compound glyph
  uint16_t 	maxZones_; // 	set to 2
  uint16_t 	maxTwilightPoints_; 	// points used in Twilight Zone (Z0)
  uint16_t 	maxStorage_; // 	number of Storage Area locations
  uint16_t 	maxFunctionDefs_; //	number of FDEFs
  uint16_t 	maxInstructionDefs_; //	number of IDEFs
  uint16_t 	maxStackElements_;	// maximum stack depth
  uint16_t 	maxSizeOfInstructions_;	// byte count for glyph instructions
  uint16_t 	maxComponentElements_; // number of glyphs referenced at top level
  uint16_t 	maxComponentDepth_; // levels of recursion
};

/*---------------------------------------------------------------------------*/

struct Loca
{
  std::vector<uint16_t> glyph_;
  std::vector<uint32_t> offset_;
  std::vector<uint32_t> length_;
};

/*---------------------------------------------------------------------------*/

struct Contour
{
  std::vector<uint8_t>  flags;
  std::vector<int16_t>  xcoords;
  std::vector<int16_t>  ycoords;
  std::vector<uint16_t> shape;

  void smooth()
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

  Rcpp::DataFrame GetContours()
  {
    return Rcpp::DataFrame::create(
      Rcpp::Named("xcoords") = xcoords,
      Rcpp::Named("ycoords") = ycoords,
      Rcpp::Named("shape")   = shape);
  }
};

/*---------------------------------------------------------------------------*/

struct Glyf
{
  int16_t               numberOfContours_;
  Fword                 xMin_;
  Fword                 yMin_;
  Fword                 xMax_;
  Fword                 yMax_;
  std::vector<uint16_t> endPtsOfContours_;
  uint16_t              instructionLength_;
  std::vector<uint8_t>  instructions_;
  Contour               contours_;
  Rcpp::List GetOutlines(){return Rcpp::List::create(contours_.GetContours());}
};

/*---------------------------------------------------------------------------*/

class TTFont
{
 public:
  TTFont(const std::string& input_stream);
  Rcpp::DataFrame GetTable(); // Rcpp
  Rcpp::List GetHead(); // Rcpp
  Rcpp::List GetCMap(); // Rcpp
  Rcpp::List GetMaxp(); // Rcpp
  Rcpp::DataFrame GetLoca(); // Rcpp
  Glyf ReadGlyf(uint16_t);

 private:
  void      GoToTable(std::string table_name);
  void      ReadCMap();
  void      ReadTables();
  void      ReadHeadTable();
  void      ReadMaxp();
  void      ReadLoca();
  uint8_t   GetUint8();
  uint16_t  GetUint16();
  uint32_t  GetUint32();
  int16_t   GetInt16();
  int32_t   GetInt32();
  Fword     GetFword();
  Date_type GetDate();
  Fixed     GetFixed();
  TTFRow    GetTTRow();
  void      ReadSimpleGlyph(Glyf&);
  void      ReadCompoundGlyph(Glyf&);
  void      HandleFormat0(CMapDirectory&);
  void      HandleFormat2(CMapDirectory&);
  void      HandleFormat4(CMapDirectory&);
  void      HandleFormat6(CMapDirectory&);
  void      HandleFormat8(CMapDirectory&);
  void      HandleFormat10(CMapDirectory&);
  void      HandleFormat12(CMapDirectory&);
  void      HandleFormat13(CMapDirectory&);
  void      HandleFormat14(CMapDirectory&);
  bool      TableExists(std::string table_name);

  std::string stream_;
  std::string::const_iterator it_;
  std::vector<TTFRow> table_of_tables_;
  HeadTable head_;
  Maxp maxp_;
  std::vector<CMapDirectory> cmap_directory_;

  uint32_t scalar_type_;
  uint16_t num_tables_;
  uint16_t search_range_;
  uint16_t entry_selector_;
  uint16_t range_shift_;
  Loca loca_;

  const std::map<uint16_t, std::string> windows_specific_map_s = {
  {0, "Windows Symbol"}, {1, "Windows Unicode (BMP only)"},
  {2, "Windows Shift-JIS"}, {3, "Windows PRC"}, {4, "Windows BigFive"},
  {5, "Windows Johab"}, {10, "Windows Unicode UCS-4"}};

  const std::map<uint16_t, std::string> unicode_specific_map_s = {
  {0, "Unicode Default"}, {1, "Unicode v1.1" }, {2, "Unicode ISO 10646" },
  {3, "Unicode v2 BMP only"},   {4, "Unicode v2" },   {5, "Unicode Variations"},
  {6, "Unicode Full"}};
};

/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/

#endif
