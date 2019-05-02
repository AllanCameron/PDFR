//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR xref implementation file                                            //
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

#include "xref.h"

//---------------------------------------------------------------------------//

using namespace std;

//---------------------------------------------------------------------------//
// The xrefstream class is private a helper class for xref. It contains
// only private members and functions. Its functions could all sit in the xref
// class, but it has been seperated out to remove clutter and because it
// represents one encapsulated and complex task. Because it is only used to
// help in the construction of an xref object, it has no public interface. The
// xref class accesses it as a friend class so no-one needs to worry about it
// (unless it breaks...)

class xrefstream
{
  friend class xref;                // This class is only constructible via xref

  shared_ptr<xref>    m_XR;         // Pointer to creating xref
  vector<vector<int>> m_rawMatrix,  // Holds the raw data in (row x column) vecs
                      m_finalArray, // Transposed, modulo 256 table
                      m_result;     // The main results table for export
  vector<int> m_arrayWidths,        // Bytes per column from /w entry
              m_objectNumbers;      // vector of object numbers inside stream.
  int         m_ncols,              // Number of Columns in table
              m_predictor,          // Specifies decoding algorithm
              m_objstart;           // Byte offset of the xrefstream's container
  dictionary  m_dict;               // Dictionary of object containing stream

  xrefstream(shared_ptr<xref>, int);// private constructor
  void getIndex();                  // Reads the index entry of main dict
  void getParms();                  // Reads the PNG decoding parameters
  void getRawMatrix();              // Reads the stream into data table
  void diffup();                    // Un-diffs the table
  void modulotranspose();           // Transposes table and makes it modulo 256
  void expandbytes();               // Multiply bytes by correct powers of 256
  void mergecolumns();              // Adds adjacent columns as per parameters
  void numberRows();                // Merges object numbers with data table
  vector<vector<int>> table();      // Getter for final result
};

/*---------------------------------------------------------------------------*/
// The xref constructor. It takes the entire file contents as a string
// then sequentially runs the steps in creation of an xref master map

xref::xref(shared_ptr<const std::string> s) :  fs(s), encrypted(false)
{
  locateXrefs();           // Find all xrefs
  xrefstrings();           // Get the strings containing all xrefs
  get_crypto();            // Get the file key, needed for decryption of streams
  Xreflocations.clear();   // Clear the large string vector to save memory
}

/*---------------------------------------------------------------------------*/
// The first job of the creator function (after its initializers) is to find
// out where the xrefs are. It does this by reading the penultimate line
// of the file, which contains the byte offset of the first xref.
//
// However, there can be multiple xrefs in a file, so for each xref this
// function finds the associated dictionary (either a trailer dictionary or an
// Xrefstream dictionary), and finds out if there are more xrefs to parse
// by checking for the "/Previous" entry, which points to the next xref
// location. All the locations are stored as a vector for passing to
// other creators.

void xref::locateXrefs()
{
  // Get last 50 chars of the file
  string&& last_50_chars = fs->substr(fs->size() - 50, 50);

  // Use carveout() from utilities.h to find the first xref offset
  string&& xrefstring =  carveout(last_50_chars, "startxref", "%%EOF");

  // Convert the number string to an int
  Xreflocations.emplace_back(stoi(xrefstring));

  // If no xref location is found, then we're stuck. Throw an error.
  if(Xreflocations.empty()) throw runtime_error("No xref entry found");

  // The first dictionary found after any xref offset is always a trailer
  // dictionary, though sometimes it doubles as an xrefstream dictionary.
  // We make this first one found the canonical trailer dictionary
  TrailerDictionary = dictionary(fs, Xreflocations[0]);

  // Now we follow the pointers to all xrefs sequentially.
  dictionary tempdict = TrailerDictionary;
  while (tempdict.contains_ints("/Prev"))
  {
    Xreflocations.emplace_back(tempdict.get_ints("/Prev")[0]);
    tempdict = dictionary(fs, Xreflocations.back());
  }
}

/*---------------------------------------------------------------------------*/
// Whatever form the xrefs take (plain or xrefstream), we first get their
// raw contents as strings from the xref locations

void xref::xrefstrings()
{
  // Get a string from each xref location or throw exception
  for(auto& start : Xreflocations)
  {
    // Find the length of xref in chars
    int len = fs->find("startxref", start) - start;

    // Throw error if no xref found
    if (len <= 0) throw std::runtime_error("No object found at location");

    // Extract the xref string
    string&& fullxref = fs->substr(start, len);

    // Carve out the actual string
    string xrefstring = carveout(move(fullxref), "xref", "trailer");

    // If it contains a dictionary, process as a stream, otherwise as a string
    if(xrefstring.substr(0, 15).find("<<", 0) != string::npos)
    {
      xrefFromStream(start);
    }
    else
    {
      xrefFromString(xrefstring);
    }
  }
}

/*---------------------------------------------------------------------------*/
// Takes an xref location and if it is a stream, creates an xrefstream object.
// The output of this object is a "table" (vec<vec<int>>) which is parsed
// and added to the main combined xref table

void xref::xrefFromStream(int xref_loc)
{
  // Calls xrefstream constructor to make the data table from the stream
  auto xref_table = xrefstream(make_shared<xref>(*this), xref_loc).table();

  // Throws if xrefstream returns an empty table
  if(xref_table.empty()) throw runtime_error("xref table empty");

  // Fill the xref data map from the table -------------------//
  for (size_t j = 0; j < xref_table[0].size(); j++)
  {
    int& object_number = xref_table[3][j], position = xref_table[1][j];

    xreftab[object_number] = xrefrow {position, 0, position};

    if(xref_table[0][j] != 2) xreftab[object_number].in_object = 0;

    else xreftab[object_number].startbyte = 0;
  }
}

/*---------------------------------------------------------------------------*/
// It is easier to parse a plain xref than an xrefstream. It consists of a pair
// of numbers for each object number - the byte offset from the start of the
// file, and an in_use number, which should be 00000 for any objects in use
// and not in a stream. The first object's number is given by a pair of ints
// in the first row representing the first object, and the number of objects
// described, respectively. Thereafter the rows represent sequential objects
// counted from the first.

void xref::xrefFromString(std::string& xstr)
{
  auto allints = parse_ints(xstr);

  // A valid xref has >= 4 ints in it and must have an even number of ints
  auto xrefsize = allints.size();
  if(xrefsize < 4 || xrefsize % 2) throw runtime_error("Malformed xref");

  // This loop starts on the second row of the table. Even numbers are the
  // byte offsets and odd numbers are the in_use numbers
  for(int bytestore = 0, i = 2; i < (int) allints.size(); ++i)
  {
    // If an odd number index and the integer is in use, store to map
    if(i % 2 && allints[i] < 0xffff)
    {
      // Numbers each row by counting pairs of numbers past initial object
      xreftab[allints[0] + (i / 2) - 1] = xrefrow {bytestore, 0, 0};
    }

    // If odd-numbered index, the number gives the byte offset of the object
    else bytestore = allints[i];
  }
}

/*---------------------------------------------------------------------------*/
// Returns the byte offset for a pdf object

size_t xref::getStart(int object_number) const
{
  auto found = xreftab.find(object_number);

  if(found == xreftab.end()) throw runtime_error("Object does not exist");

  return found->second.startbyte;
}

/*---------------------------------------------------------------------------*/
// Returns the end byte of an object by finding the first example of the
// word "endobj" after the start of the object

size_t xref::getEnd(int objnum) const
{
  auto obj_row = xreftab.find(objnum);

  // throw an error if objnum isn't a valid object
  if(obj_row == xreftab.end()) throw runtime_error("Object does not exist");

  // If the object is in an object stream, return 0;
  if(obj_row->second.in_object) return 0;

  // else find the first match of "endobj" and return
  return fs->find("endobj", obj_row->second.startbyte);
}

/*---------------------------------------------------------------------------*/
// If an object is part of an objectstream, this tells us which object forms
// the objectstream.

size_t xref::inObject(int objnum) const
{
  auto obj_row = xreftab.find(objnum);

  if(obj_row == xreftab.end()) throw runtime_error("Object does not exist");

  return obj_row->second.in_object;
}

/*---------------------------------------------------------------------------*/
// Returns vector of all objects listed in the xrefs

std::vector<int> xref::getObjects() const
{
  return getKeys(this->xreftab);
}

/*---------------------------------------------------------------------------*/

int xref::streamLength(const dictionary& dict) const
{
  if(dict.contains_references("/Length"))
  {
    int lengthob = dict.get_references("/Length")[0]; // finds reference
    size_t firstpos = getStart(lengthob);      // gets start of length obj
    size_t len = fs->find("endobj", firstpos) - firstpos; // and its length
    string objstr = fs->substr(firstpos, len); // from which we get a string
    return parse_ints(move(objstr)).back(); // from which we get a number
  }

  // Thankfully though most lengths are just direct ints
  else return dict.get_ints("/Length")[0];
}

/*---------------------------------------------------------------------------*/
// returns the offset of the start and stop locations relative to the file
// start, of the stream belonging to the given object

array<size_t, 2> xref::getStreamLoc(int objstart) const
{
  dictionary dict = dictionary(fs, objstart); // get the object dictionary
  if(dict.has_key("stream") && dict.has_key("/Length")) // only if stream exists
  {
    int streamlen = streamLength(dict);
    int strmstart = dict.get_ints("stream")[0]; // now get stream's start
    return array<size_t, 2>{(size_t) strmstart, (size_t) strmstart + streamlen};
  }
  return array<size_t, 2> {0,0}; // if no length, return empty length-2 array
}

/*---------------------------------------------------------------------------*/
// Wrapper for Encryption object so that xref is the only class that uses it

void xref::decrypt(std::string& s, int obj, int gen) const
{
  encryption->decryptStream(s, obj, gen);
}

/*---------------------------------------------------------------------------*/
// Getter for encryption state

bool xref::isEncrypted() const
{
  return this->encrypted;
}

/*---------------------------------------------------------------------------*/
// getter function to access the trailer dictionary - a private data member

dictionary xref::trailer() const
{
  return TrailerDictionary;
}

/*---------------------------------------------------------------------------*/
// This co-ordinates the creation of the encryption dictionary (if any), and
// the building plus testing of the file decryption key (if any)

void xref::get_crypto()
{
   // if there's no encryption dictionary, there's nothing else to do
  if(!TrailerDictionary.has_key("/Encrypt")) return;

  int encnum = TrailerDictionary.get_references("/Encrypt").at(0);

  // No encryption dict - exception?
  if(xreftab.find(encnum) == xreftab.end()) return;

  // mark file as encrypted and read the encryption dictionary
  encrypted = true;
  dictionary&& encdict = dictionary(fs, getStart(encnum));
  encryption = make_shared<crypto>(move(encdict), TrailerDictionary);
}

/*---------------------------------------------------------------------------*/
// simple getter for the output of an xrefstream

std::vector<std::vector<int>> xrefstream::table()
{
  return m_result;
}

/*---------------------------------------------------------------------------*/
// get a pointer to the original document

std::shared_ptr<const std::string> xref::file() const
{
  return this->fs;
}

/*---------------------------------------------------------------------------*/
// xrefstream constructor. Note that encryption does not apply to xrefstreams.
// The constructor calls several functions which together comprise the
// PNG decompression algorithm. They are seperated out to prevent one large
// hairball function being created that is difficult to debug.

xrefstream::xrefstream(shared_ptr<xref> Xref, int starts) :
  m_XR(Xref),
  m_ncols(0),
  m_predictor(0),
  m_objstart(starts),
  m_dict(dictionary(m_XR->file(), m_objstart))
{
  // If there is no /W entry, we don't know how to interpret the stream.
  if(!m_dict.contains_ints("/W"))
    throw std::runtime_error("No /W entry for stream.");

  getIndex();         // Read Index so we know which objects are in stream
  getParms();         // Read the PNG decoding parameters
  getRawMatrix();     // Arrange the raw data in stream into correct table form
  diffup();           // Undiff the raw data
  modulotranspose();  // Transposes table which ensuring all numbers are <256
  expandbytes();      // Multiply bytes to intended size based on their position
  mergecolumns();     // Sum adjacent columns that represent large numbers
  numberRows();       // Marry rows to the object numbers from the /Index entry
}

/*---------------------------------------------------------------------------*/
// The first job of the xrefstream class is to read and parse the /Index entry
// of the stream's dictionary. This entry is a series of ints which represents
// the object numbers described in the xrefstream. The first number is the
// object number of the first object being described in the first row of the
// table in the stream. The second number is the number of rows in the table
// which describe sequentially numbered objects after the first.
//
// For example, the numbers 3 5 mean that there are five objects described in
// this xrefstream starting from object number 3: {3, 4, 5, 6, 7}.
//
// However, there can be multiple pairs of numbers in the /Index entry, so the
// sequence 3 5 10 1 20 3 equates to {3, 4, 5, 6, 7, 10, 20, 21, 22}.
// The expanded sequence is stored as a private data member of type integer
// vector: objectNumbers

void xrefstream::getIndex()
{
  auto indexEntries = m_dict.get_ints("/Index");// Gets the numbers in the entry

  if(!indexEntries.empty())
  {
    for(size_t i = 0; i < indexEntries.size(); i += 2)
    {
      // Fill object numbers with consecutive ints starting at the even index
      // entries for a length specified by the odd index entries
      m_objectNumbers.resize(indexEntries[i + 1]);
      iota(m_objectNumbers.begin(), m_objectNumbers.end(), indexEntries[i]);
    }
  }
}

/*---------------------------------------------------------------------------*/
// We next read two parameters we need from the /DecodeParms entry of the
// stream dictionary: the number of columns in the table and the predictor
// number, which gives the method used to decode the stream

void xrefstream::getParms()
{
  string decodestring = "<<>>";

  if(m_dict.has_key("/DecodeParms"))
  {
    decodestring = m_dict.get_string("/DecodeParms"); // string for subdict
  }

  dictionary subdict(make_shared<string>(decodestring));

  if(subdict.contains_ints("/Columns"))
  {
    m_ncols = subdict.get_ints("/Columns")[0];
  }

  if(subdict.contains_ints("/Predictor"))
  {
    m_predictor = subdict.get_ints("/Predictor")[0];
  }
}

/*---------------------------------------------------------------------------*/
// Get the raw data from the stream and arrange it in a table as directed by
// the /W ("widths") entry in the main dictionary

void xrefstream::getRawMatrix()
{
  auto sl = m_XR->getStreamLoc(m_objstart);// finds stream location
  std::string SS = m_XR->file()->substr(sl[0], sl[1] - sl[0]); // get stream

  if(m_dict.get_string("/Filter").find("/FlateDecode", 0) != string::npos)
  {
    FlateDecode(SS); // applies decompression to stream if needed
  }

  std::vector<uint8_t> conv(SS.begin(), SS.end());  // convert string to bytes..
  std::vector<int> intstrm(conv.begin(), conv.end()); // and bytes to ints

  // read the /W entry to get the width in bytes of each column in the table
  // check the widths for any zero values and skip them if present
  m_arrayWidths = m_dict.get_ints("/W");
  auto i = remove(m_arrayWidths.begin(), m_arrayWidths.end(), 0);
  m_arrayWidths.erase(i, m_arrayWidths.end());

  // if no record of column numbers, infer from number of /W entries >0
  if(m_ncols == 0) m_ncols = m_arrayWidths.size();

  // Predictors above 10 require an extra column
  if(m_predictor > 9) m_ncols++;

  if(m_ncols == 0) throw runtime_error("divide by zero error");

  // Get number of rows
  int nrows = intstrm.size() / m_ncols;

  if ((size_t)(nrows * m_ncols) != intstrm.size()) // ensure rectangular table
  {
    throw runtime_error("Unmatched row and column numbers");
  }

  for(int i = 0; i < nrows; ++i) // now fill the raw matrix with stream data
  {
    m_rawMatrix.emplace_back(intstrm.begin() + m_ncols * i,
                             intstrm.begin() + m_ncols * (i + 1));
  }
}

/*---------------------------------------------------------------------------*/
// The PNG algorithm involves finding the difference between adjacent cells
// and storing this instead of the actual number. This function reverses that
// differencing by calculating the cumulative sums of the columns

void xrefstream::diffup()
{
  if(m_predictor == 12)
  {
    // For each row
    for(size_t i = 1; i < m_rawMatrix.size(); ++i )
    {
      // Take each entry & add the one above
      for(size_t j = 0; j < m_rawMatrix.at(i).size(); ++j)
      {
        m_rawMatrix.at(i).at(j) += m_rawMatrix.at(i - 1).at(j);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// transposes the matrix and makes it modulo 256.

void xrefstream::modulotranspose()
{
  for(size_t i = 0; i < m_rawMatrix.at(0).size(); ++i)
  {
    // Create a new column vector
    std::vector<int> tempcol;

    // Then for each entry in the row make it modulo 256 and push to new column
    for(auto& j : m_rawMatrix) tempcol.push_back(j[i] & 0x00ff);

    // the new column is pushed to the final array unless it is the first
    // column (which is skipped when the predictor is > 9)
    if(m_predictor < 10 || i > 0) m_finalArray.push_back(tempcol);
  }
}

/*---------------------------------------------------------------------------*/
// Multiplies the entries in the matrix according to the width in bytes of the
// numbers represented according to the /W entry

void xrefstream::expandbytes()
{
  // Calculate the byte shift for each column (with 1:1 correspondence);
  int ncol = 0;
  for(auto i : m_arrayWidths)
  {
    while(i)
    {
      int byte_shift = 8 * (i-- - 1);
      for(auto &j : m_finalArray.at(ncol)) j <<= byte_shift;
      ++ncol;
    }
  }
}

/*---------------------------------------------------------------------------*/
// Now the matrix has the correct values but the columns representing higher
// bytes need to be added to those with lower bytes

void xrefstream::mergecolumns()
{
  // For each of the final columns
  for(int k = 0, cumsum = 0; k < (int) m_arrayWidths.size(); ++k)
  {
    // take a zero-filled column the size of those in the unmerged array
    std::vector<int> newCol(m_finalArray[0].size(), 0);

    // for each width value
    for(int j = 0; j < m_arrayWidths[k]; j++)
    {
      // Add the column to be merged to the new column
      std::transform(newCol.begin(), newCol.end(),
                     m_finalArray[cumsum + j].begin(),
                     newCol.begin(), std::plus<int>());
    }

    // This is a final column - push it to result
    m_result.emplace_back(std::move(newCol));

    // move to the next column in the final result
    cumsum += m_arrayWidths[k];
  }
}

/*---------------------------------------------------------------------------*/
// The last step in creation of the xrefstream table is matching the rows to
// the object numbers from the /index entry table

void xrefstream::numberRows()
{
  // If there are only two columns append a zero filled column of the same size
  if(m_result.size() == 2)
  {
    m_result.emplace_back(vector<int>(m_result[0].size(), 0));
  }

  // if no index entries, assume start at object 0
  if(m_objectNumbers.empty())
  {
    // create extra column numbered sequentially from first of objectNumbers
    m_objectNumbers.resize(m_result[0].size());
    iota(m_objectNumbers.begin(), m_objectNumbers.end(), 0);
  }

  // Append to our result
  m_result.push_back(m_objectNumbers);
}


