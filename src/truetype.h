//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR TrueType header file                                                //
//                                                                           //
//  Copyright (C) 2018 - 2021 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_TTF

/*---------------------------------------------------------------------------*/

#define PDFR_TTF

/* The TrueType class allows extraction of glyphs from font files, which are
 * often stored as compressed streams inside pdf files. This allows us to get
 * the actual outlines of the glyphs. Some fontfiles also have internal cmap
 * tables that allow a code point to be translated into a particular glyph, so
 * they may have some role in text parsing if there is no external cmap or
 * other mechanism for looking up code points.
 */

#include<map>
#include<vector>
#include<string>


/*---------------------------------------------------------------------------*/
/* The docs for TrueType use particular names for common data types; it makes
 * writing the code easier to stick to these conventions by defining some
 * type aliases.
 */

typedef int16_t Fword;
typedef float Fixed;
typedef int64_t Date_type;

/*---------------------------------------------------------------------------*/
/* Reading the x and y coordinates of a simple glyph requires the interpretation
 * of a preceding flag byte. These flags are defined directly from the docs.
 */

enum flagbits {
  ON_CURVE_POINT = 0x01,
  X_SHORT_VECTOR = 0x02,
  Y_SHORT_VECTOR = 0x04,
  REPEAT_FLAG    = 0x08,
  X_MODIFIER     = 0x10,
  Y_MODIFIER     = 0x20,
  OVERLAP_SIMPLE = 0x40
};

/*---------------------------------------------------------------------------*/
/* Reading compound glyphs is even more complex than reading simple glyphs. It
 * has a 2-byte long set of flags that must be read. Again, these are taken
 * directly from the docs.
 */

enum compound_flags {
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
/* Each TrueType file has a directory or "contents page" at the start, which
 * tells us where to get each data table in the file. This "table of tables"
 * is specified row-wise and can be defined with the following simple struct:
 * */

struct TTFRow
{
  std::string table_name_; // A four-character long table name
  uint32_t    checksum_;   // Allows us to ensure the table isn't corrupted
  uint32_t    offset_;     // The table's position in the font file
  uint32_t    length_;     // The length of the table in bytes.
};

/*---------------------------------------------------------------------------*/
/* The head table gives various types of data about the font, perhaps most
 * importantly for our purposes the maximum bounding box and the units per Em of
 * the font. It is a list of scalars of different types so we give it its own
 * struct to represent and store it.
 */

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
/* The character map (or maps) is the set of mappings available. It allows the
 * lookup of code points to the actual glyphs in the file, as well as some
 * information that allows the correct set of mappings to be selected.
 */

struct CMapDirectory
{
  CMapDirectory(uint16_t p, uint16_t s, uint32_t o, std::string e) :
                  platform_id_(p), specific_id_(s), offset_(o), encoding_(e) {};

  uint16_t                     platform_id_;
  uint16_t                     specific_id_;
  uint32_t                     offset_;
  std::string                  encoding_;
  uint16_t                     format_;
  uint16_t                     length_;
  std::map<uint16_t, uint16_t> cmap_;        // The actual lookup mapping
};

/*---------------------------------------------------------------------------*/
/* The maxp table gives us the total number of glyphs in the font, as well as
 * information about the maximum value certain data points will have in the
 * font file.
 */

struct Maxp
{
  Fixed 	  version_;
  uint16_t 	numGlyphs_;             // The number of glyphs in the font
  uint16_t 	maxPoints_;             // Points in non-compound glyph
  uint16_t 	maxContours_;           // Contours in non-compound glyph
  uint16_t 	maxComponentPoints_;    // Points in compound glyph
  uint16_t 	maxComponentContours_;  // Contours in compound glyph
  uint16_t 	maxZones_;              // Set to 2
  uint16_t 	maxTwilightPoints_; 	  // Points used in Twilight Zone (Z0)
  uint16_t 	maxStorage_;            // Number of Storage Area locations
  uint16_t 	maxFunctionDefs_;       // Number of FDEFs
  uint16_t 	maxInstructionDefs_;    // Number of IDEFs
  uint16_t 	maxStackElements_;	    // Maximum stack depth
  uint16_t 	maxSizeOfInstructions_;	// Byte count for glyph instructions
  uint16_t 	maxComponentElements_;  // Number of glyphs referenced at top level
  uint16_t 	maxComponentDepth_;     // Levels of recursion
};

/*---------------------------------------------------------------------------*/
/* The loca table is what we use to look up the description of a particular
 * glyph, given its order in the glyf table.
 */

struct Loca
{
  std::vector<uint16_t> glyph_;   // The index of the glyph in the glyf table
  std::vector<uint32_t> offset_;  // The offset in bytes of the start of the
                                  // glyph relative to the start of the glyf
                                  // table
  std::vector<uint32_t> length_;  // Length in bytes of glyph description
};

/*---------------------------------------------------------------------------*/
/* The Contour struct contains 4 equal-length vectors that describe the
 * paths making up a simple (i.e. non-compound) glyph. Compound glyphs will
 * have multiple Contour entries.
 *
 * Contour objects have two functions: one that allows them to be smoothed by
 * calculating the quadratic curves implied by their x and y co-ordinates and
 * their "ON_CURVE_POINT" flag; and another to apply affine transformations.
 */

struct Contour
{
  std::vector<uint8_t>  flags;   // Important only for creating Contours
  std::vector<int16_t>  xcoords; // The actual x coordinates of the paths
  std::vector<int16_t>  ycoords; // The actual y coordinates of the paths
  std::vector<uint16_t> shape;   // Labels each x, y to let us know which
                                 // "piece" of the glyph it is on.

  void smooth();                 // Interpolates and applies quadratic Bezier


  // Performs affine transformation according to 8 entries extracted during
  // the reading of compound glyphs.
  void transform(double a, double b, double c, double d, double e, double f,
                 double m, double n);
};

/*---------------------------------------------------------------------------*/
/* The Glyf struct is a store for one or more set of Contour objects, and also
 * contains information about the number of contours as well as the side of the
 * glyf's bouding box. Simple glyphs also contain a short piece of binary code
 * to allow grid fitting and hinting.
 */

struct Glyf
{
  int16_t               numberOfContours_; // If < 0 this is a compound glyph;
                                           // Otherwise it represents the
                                           // number of path pieces making up
                                           // a simple glyph

  Fword                 xMin_;             //---------------------------------//
  Fword                 yMin_;             // These give the four corners of
  Fword                 xMax_;             // the glyph's bounding box.
  Fword                 yMax_;             //---------------------------------//

  std::vector<uint16_t> endPtsOfContours_;
  uint16_t              instructionLength_;
  std::vector<uint8_t>  instructions_;
  std::vector<Contour>  contours_;
};

/*---------------------------------------------------------------------------*/
/* This is the class that does the job of reading, co-ordinating and storing
 * the various tables in the font file. A TTF object is created by supplying
 * the font file, after which its various tables can be accessed.
 */

class TTFont
{
 public:
  TTFont(const std::string& input_stream);
  std::vector<TTFRow> GetTable() {return this->table_of_tables_;};
  HeadTable GetHead() {return this->head_;};
  std::vector<CMapDirectory> GetCMap() {return this->cmap_dir_;};
  Maxp GetMaxp() {return this->maxp_;}
  Loca GetLoca() {return this->loca_;};
  Glyf ReadGlyf(uint16_t);

 private:

  // Private methods ---------------------------------------------------------//

  // Data reading functions:

  uint8_t   GetUint8();   //--------------------------------------------------//
  uint16_t  GetUint16();  //
  uint32_t  GetUint32();  // These private functions all read the input stream
  int16_t   GetInt16();   // as various different types and then advance the
  int32_t   GetInt32();   // stream iterator appropriately
  double    GetF2Dot14(); //
  Fword     GetFword();   //
  Date_type GetDate();    //
  Fixed     GetFixed();   //--------------------------------------------------//


  // Table reading functions:

  TTFRow    GetTTRow();
  void      ReadTables();

  bool      TableExists(std::string);       // Checks a named table exists
  void      GoToTable(std::string);         // Goes to the named table

  void      ReadHeadTable();                // Reads "head" table
  void      ReadMaxp();                     // Reads "maxp" table
  void      ReadLoca();                     // Reads "loca" table
  void      ReadCMap();                     // Reads "cmap" table

     // Cmap reading helper functions:

      void  HandleFormat0(CMapDirectory&);  //--------------------------------//
      void  HandleFormat2(CMapDirectory&);  // There are different formats of
      void  HandleFormat4(CMapDirectory&);  // cmap table, and each of these is
      void  HandleFormat6(CMapDirectory&);  // read differently. After figuring
      void  HandleFormat8(CMapDirectory&);  // out which format a cmap is stored
      void  HandleFormat10(CMapDirectory&); // in, the cmap builder picks the
      void  HandleFormat12(CMapDirectory&); // correct HandleFormatxx function
      void  HandleFormat13(CMapDirectory&); // to read the cmap table.
      void  HandleFormat14(CMapDirectory&); //--------------------------------//

  // Glyph reading functions:

  void      ReadSimpleGlyph(Glyf&);         // These two helpers are only called
  void      ReadCompoundGlyph(Glyf&);       // when a particular glyph is read


  // Private data members ----------------------------------------------------//

    // Reading the file:

    std::string stream_;                    // The actual font file being read
    std::string::const_iterator it_;        // Iterator reading the font file

    // Font header information:

    uint32_t scalar_type_;                  //--------------------------------//
    uint16_t num_tables_;                   // These fields are included in the
    uint16_t search_range_;                 // font file header to help us
    uint16_t entry_selector_;               // navigate the file
    uint16_t range_shift_;                  //--------------------------------//

    // Storing the contents of the tables:

    std::vector<TTFRow> table_of_tables_;   // The directory of tables
    HeadTable head_;                        // The "head" table's contents
    Maxp maxp_;                             // The "maxp" table's contents
    std::vector<CMapDirectory> cmap_dir_;   // The "cmap" table's contents
    Loca loca_;                             // The "loca" table's contents
};

/*---------------------------------------------------------------------------*/


#endif
