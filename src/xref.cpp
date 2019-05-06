//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR XRef implementation File                                            //
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
// The XRefStream class is private a helper class for XRef. It contains
// only private members and functions. Its functions could all sit in the XRef
// class, but it has been seperated out to remove clutter and because it
// represents one encapsulated and complex task. Because it is only used to
// help in the construction of an XRef object, it has no public interface. The
// XRef class accesses it as a friend class so no-one needs to worry about it
// (unless it breaks...)

class XRefStream
{
  friend class XRef;                // This class is only constructible via XRef

  shared_ptr<XRef>    xref_;        // Pointer to creating XRef
  vector<vector<int>> raw_matrix_,  // Holds the raw data in (row x column) vecs
                      final_array_, // Transposed, modulo 256 table
                      result_;      // The main results table for export
  vector<int> array_widths_,        // Bytes per column from /w entry
              object_numbers_;      // vector of object numbers inside stream.
  int         number_of_columns_,   // Number of Columns in table
              predictor_,           // Specifies decoding algorithm
              object_start_;        // Byte offset of the XRefStream's container
  Dictionary  dictionary_;          // Dictionary of object containing stream

  XRefStream(shared_ptr<XRef>, int);// private constructor
  void ReadIndex();                // Reads the index entry of main dict
  void ReadParameters();           // Reads the PNG decoding parameters
  void GetRawMatrix();            // Reads the stream into data table
  void DiffUp();                   // Un-diffs the table
  void ModuloTranspose();          // Transposes table and makes it modulo 256
  void ExpandBytes();              // Multiply bytes by correct powers of 256
  void MergeColumns();             // Adds adjacent columns as per parameters
  void NumberRows();               // Merges object numbers with data table
  vector<vector<int>> table();      // Getter for final result
};

/*---------------------------------------------------------------------------*/
// The XRef constructor. It takes the entire file contents as a string
// then sequentially runs the steps in creation of an XRef master map

XRef::XRef(shared_ptr<const string> s) :
  file_string_(s), encrypted_(false)
{
  LocateXRefs();             // Find all xrefs
  ReadXRefStrings();         // Get the strings containing all xrefs
  CreateCrypto();            // Get file key, needed for decryption of streams
  xref_locations_.clear();   // Clear the large string vector to save memory
}

/*---------------------------------------------------------------------------*/
// The first job of the creator function (after its initializers) is to find
// out where the xrefs are. It does this by reading the penultimate line
// of the file, which contains the byte offset of the first XRef.
//
// However, there can be multiple xrefs in a file, so for each XRef this
// function finds the associated dictionary (either a trailer dictionary or an
// XRefStream dictionary), and finds out if there are more xrefs to parse
// by checking for the "/Previous" entry, which points to the next XRef
// location. All the locations are stored as a vector for passing to
// other creators.

void XRef::LocateXRefs()
{
  // Get last 50 chars of the file
  string&& last_50_chars = file_string_->substr(file_string_->size()-50, 50);

  // Use carve_out() from utilities.h to find the first XRef offset
  string&& xref_string =  carve_out(last_50_chars, "startxref", "%%EOF");

  // Convert the number string to an int
  xref_locations_.emplace_back(stoi(xref_string));

  // If no XRef location is found, then we're stuck. Throw an error.
  if(xref_locations_.empty()) throw runtime_error("No XRef entry found");

  // The first dictionary found after any XRef offset is always a trailer
  // dictionary, though sometimes it doubles as an XRefStream dictionary.
  // We make this first one found the canonical trailer dictionary
  trailer_dictionary_ = Dictionary(file_string_, xref_locations_[0]);

  // Now we follow the pointers to all xrefs sequentially.
  Dictionary temp_dictionary = trailer_dictionary_;
  while (temp_dictionary.ContainsInts("/Prev"))
  {
    xref_locations_.emplace_back(temp_dictionary.GetInts("/Prev")[0]);
    temp_dictionary = Dictionary(file_string_, xref_locations_.back());
  }
}

/*---------------------------------------------------------------------------*/
// Whatever form the xrefs take (plain or XRefStream), we first get their
// raw contents as strings from the XRef locations

void XRef::ReadXRefStrings()
{
  // Get a string from each XRef location or throw exception
  for (auto& start : xref_locations_)
  {
    // Find the length of XRef in chars
    int len = file_string_->find("startxref", start) - start;

    // Throw error if no XRef found
    if (len <= 0) throw runtime_error("No object found at location");

    // Extract the XRef string
    string&& fullxref = file_string_->substr(start, len);

    // Carve out the actual string
    string xref_string = carve_out(move(fullxref), "xref", "trailer");

    // If it contains a dictionary, process as a stream, otherwise as a string
    if(xref_string.substr(0, 15).find("<<", 0) != string::npos)
    {
      ReadXRefFromStream(start);
    }
    else
    {
      ReadXRefFromString(xref_string);
    }
  }
}

/*---------------------------------------------------------------------------*/
// Takes an XRef location and if it is a stream, creates an XRefStream object.
// The output of this object is a "table" (vec<vec<int>>) which is parsed
// and added to the main combined XRef table

void XRef::ReadXRefFromStream(int xref_location)
{
  // Calls XRefStream constructor to make the data table from the stream
  auto xref_table = XRefStream(make_shared<XRef>(*this), xref_location).table();

  // Throws if XRefStream returns an empty table
  if(xref_table.empty()) throw runtime_error("XRef table empty");

  // Fill the XRef data map from the table -------------------//
  for (size_t j = 0; j < xref_table[0].size(); j++)
  {
    int& object_number = xref_table[3][j], position = xref_table[1][j];

    xref_table_[object_number] = XRefRow {position, 0, position};

    if(xref_table[0][j] != 2) xref_table_[object_number].in_object = 0;

    else xref_table_[object_number].startbyte = 0;
  }
}

/*---------------------------------------------------------------------------*/
// It is easier to parse a plain XRef than an XRefStream. It consists of a pair
// of numbers for each object number - the byte offset from the start of the
// file, and an in_use number, which should be 00000 for any objects in use
// and not in a stream. The first object's number is given by a pair of ints
// in the first row representing the first object, and the number of objects
// described, respectively. Thereafter the rows represent sequential objects
// counted from the first.

void XRef::ReadXRefFromString(string& xref_string)
{
  auto all_ints = parse_ints(xref_string);

  // A valid XRef has >= 4 ints in it and must have an even number of ints
  auto xref_size = all_ints.size();
  if(xref_size < 4 || xref_size % 2) throw runtime_error("Malformed XRef");

  // This loop starts on the second row of the table. Even numbers are the
  // byte offsets and odd numbers are the in_use numbers
  for (int bytestore = 0, i = 2; i < (int) all_ints.size(); ++i)
  {
    // If an odd number index and the integer is in use, store to map
    if(i % 2 && all_ints[i] < 0xffff)
    {
      // Numbers each row by counting pairs of numbers past initial object
      xref_table_[all_ints[0] + (i / 2) - 1] = XRefRow {bytestore, 0, 0};
    }

    // If odd-numbered index, the number gives the byte offset of the object
    else bytestore = all_ints[i];
  }
}

/*---------------------------------------------------------------------------*/
// Returns the byte offset for a pdf object

size_t XRef::GetObjectStartByte(int object_number) const
{
  auto found = xref_table_.find(object_number);

  if(found == xref_table_.end()) throw runtime_error("Object does not exist");

  return found->second.startbyte;
}

/*---------------------------------------------------------------------------*/
// Returns the end byte of an object by finding the first example of the
// word "endobj" after the start of the object

size_t XRef::GetObjectEndByte(int object_number) const
{
  auto row = xref_table_.find(object_number);

  // throw an error if objnum isn't a valid object
  if(row == xref_table_.end()) throw runtime_error("Object doesn't exist");

  // If the object is in an object stream, return 0;
  if(row->second.in_object) return 0;

  // else find the first match of "endobj" and return
  return file_string_->find("endobj", row->second.startbyte);
}

/*---------------------------------------------------------------------------*/
// If an object is part of an objectstream, this tells us which object forms
// the objectstream.

size_t XRef::GetHoldingNumberOf(int object_number) const
{
  auto row = xref_table_.find(object_number);

  if(row == xref_table_.end()) throw runtime_error("Object does not exist");

  return row->second.in_object;
}

/*---------------------------------------------------------------------------*/
// Returns vector of all objects listed in the xrefs

vector<int> XRef::GetAllObjectNumbers() const
{
  return getKeys(this->xref_table_);
}

/*---------------------------------------------------------------------------*/

int XRef::GetStreamLength(const Dictionary& dict) const
{
  if(dict.ContainsReferences("/Length"))
  {
    int lengthob = dict.GetReference("/Length"); // finds reference
    size_t firstpos = GetObjectStartByte(lengthob);
    size_t len = file_string_->find("endobj", firstpos) - firstpos;
    string objstr = file_string_->substr(firstpos, len);
    return parse_ints(move(objstr)).back(); // from which we get a number
  }

  // Thankfully though most lengths are just direct ints
  else return dict.GetInts("/Length")[0];
}

/*---------------------------------------------------------------------------*/
// returns the offset of the start and stop locations relative to the file
// start, of the stream belonging to the given object

array<size_t, 2> XRef::GetStreamLocation(int object_start) const
{
  // Get the object dictionary
  Dictionary dict = Dictionary(file_string_, object_start);

  // If the stream exists, get its start / stop positions as a length-2 array
  if(dict.HasKey("stream") && dict.HasKey("/Length"))
  {
    size_t stream_length = (size_t) GetStreamLength(dict);
    size_t stream_start  = (size_t) dict.GetInts("stream")[0];
    size_t stream_end    = (size_t) stream_start + stream_length;
    return array<size_t, 2>{stream_start, stream_end};
  }
  return array<size_t, 2> {0,0}; // if no length, return empty length-2 array
}

/*---------------------------------------------------------------------------*/
// Wrapper for Encryption object so that XRef is the only class that uses it

void XRef::Decrypt(string& s, int obj, int gen) const
{
  encryption_->decrypt_stream(s, obj, gen);
}

/*---------------------------------------------------------------------------*/
// Getter for encryption state

bool XRef::IsEncrypted() const
{
  return this->encrypted_;
}

/*---------------------------------------------------------------------------*/
// getter function to access the trailer dictionary - a private data member

Dictionary XRef::GetTrailer() const
{
  return trailer_dictionary_;
}

/*---------------------------------------------------------------------------*/
// This co-ordinates the creation of the encryption dictionary (if any), and
// the building plus testing of the file decryption key (if any)

void XRef::CreateCrypto()
{
   // if there's no encryption dictionary, there's nothing else to do
  if(!trailer_dictionary_.HasKey("/Encrypt")) return;

  int encryption_number = trailer_dictionary_.GetReference("/Encrypt");

  // No encryption dict - exception?
  if(xref_table_.find(encryption_number) == xref_table_.end()) return;

  // mark file as encrypted and read the encryption dictionary
  encrypted_ = true;
  size_t starts_at = GetObjectStartByte(encryption_number);
  Dictionary&& dict = Dictionary(file_string_, starts_at);
  encryption_ = make_shared<Crypto>(move(dict), trailer_dictionary_);
}

/*---------------------------------------------------------------------------*/
// simple getter for the output of an XRefStream

vector<vector<int>> XRefStream::table()
{
  return result_;
}

/*---------------------------------------------------------------------------*/
// get a pointer to the original document

shared_ptr<const string> XRef::File() const
{
  return this->file_string_;
}

/*---------------------------------------------------------------------------*/
// XRefStream constructor. Note that encryption does not apply to XRefStreams.
// The constructor calls several functions which together comprise the
// PNG decompression algorithm. They are seperated out to prevent one large
// hairball function being created that is difficult to debug.

XRefStream::XRefStream(shared_ptr<XRef> XRef, int starts) :
  xref_(XRef),
  number_of_columns_(0),
  predictor_(0),
  object_start_(starts),
  dictionary_(Dictionary(xref_->File(), object_start_))
{
  // If there is no /W entry, we don't know how to interpret the stream.
  if(!dictionary_.ContainsInts("/W"))
    throw runtime_error("No /W entry for stream.");

  ReadIndex();       // Read Index so we know which objects are in stream
  ReadParameters();  // Read the PNG decoding parameters
  GetRawMatrix();   // Arrange the raw data in stream into correct table form
  DiffUp();          // Undiff the raw data
  ModuloTranspose(); // Transposes table which ensuring all numbers are <256
  ExpandBytes();     // Multiply bytes to intended size based on their position
  MergeColumns();    // Sum adjacent columns that represent large numbers
  NumberRows();      // Marry rows to the object numbers from the /Index entry
}

/*---------------------------------------------------------------------------*/
// The first job of the XRefStream class is to read and parse the /Index entry
// of the stream's dictionary. This entry is a series of ints which represents
// the object numbers described in the XRefStream. The first number is the
// object number of the first object being described in the first row of the
// table in the stream. The second number is the number of rows in the table
// which describe sequentially numbered objects after the first.
//
// For example, the numbers 3 5 mean that there are five objects described in
// this XRefStream starting from object number 3: {3, 4, 5, 6, 7}.
//
// However, there can be multiple pairs of numbers in the /Index entry, so the
// sequence 3 5 10 1 20 3 equates to {3, 4, 5, 6, 7, 10, 20, 21, 22}.
// The expanded sequence is stored as a private data member of type integer
// vector: objectNumbers

void XRefStream::ReadIndex()
{
  auto indexEntries = dictionary_.GetInts("/Index");// Gets the numbers in the entry

  if(!indexEntries.empty())
  {
    for (size_t i = 0; i < indexEntries.size(); i += 2)
    {
      // Fill object numbers with consecutive ints starting at the even index
      // entries for a length specified by the odd index entries
      object_numbers_.resize(indexEntries[i + 1]);
      iota(object_numbers_.begin(), object_numbers_.end(), indexEntries[i]);
    }
  }
}

/*---------------------------------------------------------------------------*/
// We next read two parameters we need from the /DecodeParms entry of the
// stream dictionary: the number of columns in the table and the predictor
// number, which gives the method used to decode the stream

void XRefStream::ReadParameters()
{
  string decodestring = "<<>>";

  if(dictionary_.HasKey("/DecodeParms"))
  {
    decodestring = dictionary_.GetString("/DecodeParms"); // string for subdict
  }

  Dictionary subdict(make_shared<string>(decodestring));

  if(subdict.ContainsInts("/Columns"))
  {
    number_of_columns_ = subdict.GetInts("/Columns")[0];
  }

  if(subdict.ContainsInts("/Predictor"))
  {
    predictor_ = subdict.GetInts("/Predictor")[0];
  }
}

/*---------------------------------------------------------------------------*/
// Get the raw data from the stream and arrange it in a table as directed by
// the /W ("widths") entry in the main dictionary

void XRefStream::GetRawMatrix()
{
  auto sl = xref_->GetStreamLocation(object_start_);
  string SS = xref_->File()->substr(sl[0], sl[1] - sl[0]);

  // Applies decompression to stream if needed
  if(dictionary_.GetString("/Filter").find("/FlateDecode", 0) != string::npos)
  {
    FlateDecode(SS);
  }

  vector<uint8_t> conv(SS.begin(), SS.end());  // convert string to bytes..
  vector<int> intstrm(conv.begin(), conv.end()); // and bytes to ints

  // read the /W entry to get the width in bytes of each column in the table
  // check the widths for any zero values and skip them if present
  array_widths_ = dictionary_.GetInts("/W");
  auto i = remove(array_widths_.begin(), array_widths_.end(), 0);
  array_widths_.erase(i, array_widths_.end());

  // if no record of column numbers, infer from number of /W entries >0
  if(number_of_columns_ == 0) number_of_columns_ = array_widths_.size();

  // Predictors above 10 require an extra column
  if(predictor_ > 9) number_of_columns_++;

  if(number_of_columns_ == 0) throw runtime_error("divide by zero error");

  // Gets number of rows
  int nrows = intstrm.size() / number_of_columns_;

  // Ensures rectangular table
  if ((size_t)(nrows * number_of_columns_) != intstrm.size())
  {
    throw runtime_error("Unmatched row and column numbers");
  }

  // Now fill the raw matrix with stream data
  for (int i = 0; i < nrows; ++i)
  {
    raw_matrix_.emplace_back(intstrm.begin() + number_of_columns_ * i,
                             intstrm.begin() + number_of_columns_ * (i + 1));
  }
}

/*---------------------------------------------------------------------------*/
// The PNG algorithm involves finding the difference between adjacent cells
// and storing this instead of the actual number. This function reverses that
// differencing by calculating the cumulative sums of the columns

void XRefStream::DiffUp()
{
  if(predictor_ == 12)
  {
    // For each row
    for (size_t i = 1; i < raw_matrix_.size(); ++i )
    {
      // Take each entry & add the one above
      for (size_t j = 0; j < raw_matrix_.at(i).size(); ++j)
      {
        raw_matrix_.at(i).at(j) += raw_matrix_.at(i - 1).at(j);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// transposes the matrix and makes it modulo 256.

void XRefStream::ModuloTranspose()
{
  for (size_t i = 0; i < raw_matrix_.at(0).size(); ++i)
  {
    // Create a new column vector
    vector<int> tempcol;

    // Then for each entry in the row make it modulo 256 and push to new column
    for (auto& j : raw_matrix_) tempcol.push_back(j[i] & 0x00ff);

    // the new column is pushed to the final array unless it is the first
    // column (which is skipped when the predictor is > 9)
    if(predictor_ < 10 || i > 0) final_array_.push_back(tempcol);
  }
}

/*---------------------------------------------------------------------------*/
// Multiplies the entries in the matrix according to the width in bytes of the
// numbers represented according to the /W entry

void XRefStream::ExpandBytes()
{
  // Calculate the byte shift for each column (with 1:1 correspondence);
  int ncol = 0;
  for (auto i : array_widths_)
  {
    while (i)
    {
      int byte_shift = 8 * (i-- - 1);
      for (auto &j : final_array_.at(ncol)) j <<= byte_shift;
      ++ncol;
    }
  }
}

/*---------------------------------------------------------------------------*/
// Now the matrix has the correct values but the columns representing higher
// bytes need to be added to those with lower bytes

void XRefStream::MergeColumns()
{
  // For each of the final columns
  for (int k = 0, cumsum = 0; k < (int) array_widths_.size(); ++k)
  {
    // take a zero-filled column the size of those in the unmerged array
    vector<int> newCol(final_array_[0].size(), 0);

    // for each width value
    for (int j = 0; j < array_widths_[k]; j++)
    {
      // Add the column to be merged to the new column
      transform(newCol.begin(), newCol.end(),
                     final_array_[cumsum + j].begin(),
                     newCol.begin(), plus<int>());
    }

    // This is a final column - push it to result
    result_.emplace_back(move(newCol));

    // move to the next column in the final result
    cumsum += array_widths_[k];
  }
}

/*---------------------------------------------------------------------------*/
// The last step in creation of the XRefStream table is matching the rows to
// the object numbers from the /index entry table

void XRefStream::NumberRows()
{
  // If there are only two columns append a zero filled column of the same size
  if(result_.size() == 2)
  {
    result_.emplace_back(vector<int>(result_[0].size(), 0));
  }

  // if no index entries, assume start at object 0
  if(object_numbers_.empty())
  {
    // create extra column numbered sequentially from first of objectNumbers
    object_numbers_.resize(result_[0].size());
    iota(object_numbers_.begin(), object_numbers_.end(), 0);
  }

  // Append to our result
  result_.push_back(object_numbers_);
}


