//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR XRef implementation File                                            //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "utilities.h"
#include "dictionary.h"
#include "deflate.h"
#include "crypto.h"
#include "xref.h"

//---------------------------------------------------------------------------//

using namespace std;

//---------------------------------------------------------------------------//
// The XRefStream class is private a helper class for XRef. It contains
// only private members and functions. Its functions could all sit in the XRef
// class, but it has been separated out to remove clutter and because it
// represents one encapsulated and complex task. Because it is only used to
// help in the construction of an XRef object, it has no public interface. The
// XRef class accesses it as a friend class so no-one needs to worry about it
// (unless it breaks...)

class XRefStream
{
  friend class XRef;  // This class is only constructible via XRef

  XRef&   xref_;         // Pointer to creating XRef
  vector<uint8_t> byte_stream_;
  vector<vector<int>> final_array_,  // Transposed, modulo 256 table
                      result_;       // The main results table for export
  vector<int> array_widths_,         // Bytes per column from /w entry
              object_numbers_;       // vector of object numbers inside stream.
  int         number_of_columns_,    // Number of Columns in table
              predictor_,            // Specifies decoding algorithm
              object_start_;         // Byte offset of XRefStream's container
  Dictionary  dictionary_;           // Dictionary of object containing stream

  XRefStream(XRef&, int); // Private constructor
  void ReadStream_();            // Reads the PNG decoding parameters
  void ProcessStream_();              // Reads the stream into data table
  void ToColumns_(int, int);  // Transposes table and makes it modulo 256
  void ExpandBytes_();               // Multiply bytes by correct powers of 256
  void MergeColumns_();              // Adds adjacent columns as per parameters
  void NumberRows_();                // Merges object numbers with data table
  vector<vector<int>> Table_();      // Getter for final result
};

/*---------------------------------------------------------------------------*/
// The XRef constructor. It takes the entire file contents as a string
// then sequentially runs the steps in creation of an XRef master map

XRef::XRef(shared_ptr<const string> file_string_ptr)
  : file_string_(file_string_ptr)
{
  LocateXRefs_();             // Find all xrefs
  CreateCrypto_();            // Get file key, needed for decryption of streams
}

/*---------------------------------------------------------------------------*/
// The first job of the constructor (after its initializers) is to find
// out where the xrefs are. It does this by reading the penultimate line
// of the file, which contains the byte offset of the first XRef.
//
// However, there can be multiple xrefs in a file, so for each XRef this
// function finds the associated dictionary (either a trailer dictionary or an
// XRefStream dictionary), and finds out if there are more xrefs to parse
// by checking for the "/Previous" entry, which points to the next XRef
// location. All the locations are stored as a vector for passing to
// other creators.

void XRef::LocateXRefs_()
{
  // Get last 50 chars of the file
  CharString file_tail(*file_string_, file_string_->size() - 50);
  auto xref_charstring = file_tail.CarveOut("startxref", "%%EOF");

  // Convert the number string to an int
  vector<int> xref_locations {atoi(xref_charstring.begin())};

  // If no XRef location is found, then we're stuck. Throw an error.
  if (xref_locations.empty()) throw runtime_error("No XRef entry found");

  // The first dictionary found after any XRef offset is always a trailer
  // dictionary, though sometimes it doubles as an XRefStream dictionary.
  // We make this first one found the canonical trailer dictionary
  trailer_dictionary_ = Dictionary(file_string_, xref_locations[0]);
  // Now we follow the pointers to all xrefs sequentially.
  Dictionary temp_dictionary = trailer_dictionary_;
  while (temp_dictionary.ContainsInts("/Prev"))
  {
    xref_locations.emplace_back(temp_dictionary.GetInts("/Prev")[0]);
    temp_dictionary = Dictionary(file_string_, xref_locations.back());
  }

  // Get a string from each XRef location or throw exception
  for (auto& start : xref_locations) ReadXRefStrings_(start);
}

/*---------------------------------------------------------------------------*/
// Whatever form the xrefs take (plain or XRefStream), we first get their
// raw contents as strings from the XRef locations

void XRef::ReadXRefStrings_(int start)
{
  // Find the length of XRef in chars
  int end = file_string_->find("startxref", start);

  // Throw error if no XRef found
  if (end <= 0) throw runtime_error("No object found at location");

  // Extract the XRef string
  CharString fullxref(file_string_->c_str(), start, end);

  // Carve out the actual string
  CharString xref_string = fullxref.CarveOut("xref", "trailer");

  // If it contains a dictionary, process as a stream, otherwise as a string
  auto finder = xref_string.substr(0, 20);
  if (finder.contains("<<")) ReadXRefFromStream_(start);
  else ReadXRefFromString_(xref_string);
}

/*---------------------------------------------------------------------------*/
// Takes an XRef location and if it is a stream, creates an XRefStream object.
// The output of this object is a "table" (vec<vec<int>>) which is parsed
// and added to the main combined XRef table

void XRef::ReadXRefFromStream_(int location)
{
  // Calls XRefStream constructor to make the data table from the stream
  auto&& xref_table = XRefStream(*this, location).Table_();

  // Throws if XRefStream returns an empty table
  if (xref_table.empty()) throw runtime_error("XRef table empty");

  // Fill the XRef data map from the table -------------------//
  for (size_t j = 0; j < xref_table[0].size(); j++)
  {
    int& object_number = xref_table[3][j], position = xref_table[1][j];
    xref_table_[object_number] = XRefRow {position, 0, position};
    if (xref_table[0][j] != 2) xref_table_[object_number].in_object = 0;
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

void XRef::ReadXRefFromString_(const CharString& xref_string)
{
  auto all_ints = ParseInts(xref_string);

  // A valid XRef has >= 4 ints in it and must have an even number of ints
  auto xref_size = all_ints.size();
  if (xref_size % 2) { throw runtime_error(xref_string.AsString()); }

  // This loop starts on the second row of the table. Even numbers are the
  // byte offsets and odd numbers are the in_use numbers
  for (int bytestore = 0, i = 2; i < (int) all_ints.size(); ++i)
  {
    // If an odd number index and the integer is in use, store to map
    if (i % 2 && all_ints[i] < 0xffff)
    {
      // Numbers each row by counting pairs of numbers past initial object
      xref_table_[all_ints[0] + (i / 2) - 1] = XRefRow {bytestore, 0, 0};
    }

    // If odd-numbered index, the number gives the byte offset of the object
    else bytestore = all_ints[i];
  }
}

const XRefRow& XRef::GetRow_(int object_number) const
{
  auto found = xref_table_.find(object_number);
  if (found == xref_table_.end()) throw runtime_error("Object does not exist");
  return found->second;
}


/*---------------------------------------------------------------------------*/
// Returns the end byte of an object by finding the first example of the
// word "endobj" after the start of the object

size_t XRef::GetObjectEndByte(int object_number) const
{
  auto&& row = GetRow_(object_number);

  // If the object is in an object stream, return 0;
  if (row.in_object) return 0;

  // else find the first match of "endobj" and return
  return file_string_->find("endobj", row.startbyte);
}


/*---------------------------------------------------------------------------*/
// Returns vector of all objects listed in the xrefs

vector<int> XRef::GetAllObjectNumbers() const
{
  return GetKeys(this->xref_table_);
}

/*---------------------------------------------------------------------------*/

int XRef::GetStreamLength_(const Dictionary& dictionary) const
{
  if (dictionary.ContainsReferences("/Length"))
  {
    int length_object_number = dictionary.GetReference("/Length");
    size_t first_position = GetObjectStartByte(length_object_number);
    size_t len = file_string_->find("endobj", first_position) - first_position;
    CharString object_string(file_string_->c_str() + first_position, len);
    return ParseInts(object_string).back();
  }

  // Thankfully though most lengths are just direct ints
  else return dictionary.GetInts("/Length")[0];
}

/*---------------------------------------------------------------------------*/
// Returns the offset of the start location relative to the file start, and the
// length, of the stream belonging to the given object

CharString XRef::GetStreamLocation(int object_start) const
{
  // Get the object dictionary
  Dictionary dictionary = Dictionary(file_string_, object_start);

  // If the stream exists, get its start / stop positions as a CharString
  if (dictionary.HasKey("stream") && dictionary.HasKey("/Length"))
  {
    size_t stream_length = (size_t) GetStreamLength_(dictionary);
    size_t stream_start  = (size_t) dictionary.GetInts("stream")[0];
    stream_length += stream_start;
    return CharString(file_string_->c_str(), stream_start, stream_length);
  }
  return CharString(); // if no length, return empty length-2 array
}

/*---------------------------------------------------------------------------*/
// Wrapper for Encryption object so that XRef is the only class that uses it

string XRef::Decrypt(const CharString& str, int obj, int gen)const
{
  return encryption_->DecryptStream(str, obj, gen);
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

void XRef::CreateCrypto_()
{
   // if there's no encryption dictionary, there's nothing else to do
  if (!trailer_dictionary_.HasKey("/Encrypt")) return;

  int encryption_number = trailer_dictionary_.GetReference("/Encrypt");

  // No encryption dict - exception?
  if (xref_table_.find(encryption_number) == xref_table_.end()) return;

  // mark file as encrypted and read the encryption dictionary
  size_t starts_at = GetObjectStartByte(encryption_number);
  Dictionary&& dictionary = Dictionary(file_string_, starts_at);
  encryption_ = make_shared<Crypto>(move(dictionary), trailer_dictionary_);
}

/*---------------------------------------------------------------------------*/
// simple getter for the output of an XRefStream

vector<vector<int>> XRefStream::Table_()
{
  return result_;
}

/*---------------------------------------------------------------------------*/
// XRefStream constructor. Note that encryption does not apply to XRefStreams.
// The constructor calls several functions which together comprise the
// PNG decompression algorithm. They are seperated out to prevent one large
// hairball function being created that is difficult to debug.

XRefStream::XRefStream(XRef& xref, int starts_at)
  : xref_(xref),
    number_of_columns_(0),
    predictor_(0),
    object_start_(starts_at),
    dictionary_(Dictionary(xref_.File(), object_start_))
{
  ReadStream_();    // Reads the PNG decoding parameters
  ProcessStream_(); // Arranges the raw data in stream into correct table form
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
//
// We next read two parameters we need from the /DecodeParms entry of the
// stream dictionary: the number of columns in the table and the predictor
// number, which gives the method used to decode the stream

void XRefStream::ReadStream_()
{
  // If there is no /W entry, we don't know how to interpret the stream.
  if (!dictionary_.ContainsInts("/W")) throw runtime_error("No /W entry found");

   // Gets the numbers in the /Index entry
  auto index_entries = dictionary_.GetInts("/Index");

  if (!index_entries.empty())
  {
    for (size_t i = 0; i < index_entries.size(); i += 2)
    {
      // Fill object numbers with consecutive ints starting at the even index
      // entries for a length specified by the odd index entries
      object_numbers_.resize(index_entries[i + 1]);
      iota(object_numbers_.begin(), object_numbers_.end(), index_entries[i]);
    }
  }

  Dictionary sub_dictionary(dictionary_.GetDictionary("/DecodeParms"));
  number_of_columns_ = sub_dictionary.GetInts("/Columns").at(0);
  predictor_ = sub_dictionary.GetInts("/Predictor").at(0);

  // Read the /W entry to get the width in bytes of each column in the table
  // check the widths for any zero values and skip them if present
  for (auto i : dictionary_.GetInts("/W")) if (i) array_widths_.push_back(i);
  if(array_widths_.empty()) throw runtime_error("Invalid /W entry in XRefStrm");

  // if no record of column numbers, infer from number of /W entries
  if (number_of_columns_ == 0) number_of_columns_ = array_widths_.size();

  // Predictors above 10 require an extra column
  if (predictor_ > 9) ++number_of_columns_;

  // Obtain the raw stream data
  auto charstream = xref_.GetStreamLocation(object_start_);

  // Applies decompression to stream if needed
  string&& s = dictionary_["/Filter"].find("/FlateDecode", 0) != string::npos?
               FlateDecode(charstream) : charstream.AsString();

  byte_stream_ = vector<uint8_t>(s.begin(), s.end());
}

/*---------------------------------------------------------------------------*/
// Get the raw data from the stream and arrange it in a table as directed by
// the /W ("widths") entry in the main dictionary

void XRefStream::ProcessStream_()
{
  // Gets number of rows
  int number_of_rows = byte_stream_.size() / number_of_columns_;

  // Ensures rectangular table
  if ((size_t)(number_of_rows * number_of_columns_) != byte_stream_.size())
  {
    throw runtime_error("Unmatched row and column numbers");
  }

  if (predictor_ == 12)
  {
    for (auto element = byte_stream_.begin() + number_of_columns_;
              element != byte_stream_.end(); ++element)
    {
      *element += *(element - number_of_columns_);
    }
  }
  ToColumns_(number_of_columns_, number_of_rows);
  ExpandBytes_();
  MergeColumns_();
  NumberRows_();
}


/*---------------------------------------------------------------------------*/
// Changes the byte stream into a group of columns

void XRefStream::ToColumns_(int n_cols, int n_rows)
{
  for (int i = 0; i < n_cols; ++i)
  {
    // Create a new column vector
    vector<int> temp_column{};
    temp_column.reserve(n_rows);
    for(int j = 0; j < n_rows; ++j)
    {
      temp_column.push_back((int) (uint8_t) byte_stream_[i + j * n_cols]);
    }

    // the new column is pushed to the final array unless it is the first
    // column (which is skipped when the predictor is > 9)
    if (predictor_ < 10 || i > 0) final_array_.push_back(move(temp_column));
  }
}

/*---------------------------------------------------------------------------*/
// Multiplies the entries in the matrix according to the width in bytes of the
// numbers represented according to the /W entry

void XRefStream::ExpandBytes_()
{
  // Calculate the byte shift for each column (with 1:1 correspondence);
  int column_number = 0;
  for (auto width : array_widths_)
  {
    while (width)
    {
      int byte_shift = 8 * (width-- - 1);
      for (auto& element : final_array_.at(column_number))
      {
        element <<= byte_shift;
      }
      ++column_number;
    }
  }
}

/*---------------------------------------------------------------------------*/
// Now the matrix has the correct values but the columns representing higher
// bytes need to be added to those with lower bytes

void XRefStream::MergeColumns_()
{
  // For each of the final columns
  for (size_t column_number = 0, cumsum = 0;
       column_number < array_widths_.size();
       ++column_number)
  {
    // Take a zero-filled column the size of those in the unmerged array
    vector<int> new_column(final_array_[0].size(), 0);

    // For each width value
    for (int width_it = 0; width_it < array_widths_[column_number]; ++width_it)
    {
      // Add the column to be merged to the new column
      transform(new_column.begin(), new_column.end(),
                final_array_[cumsum + width_it].begin(),
                new_column.begin(), plus<int>());
    }

    // This is a final column - push it to result
    result_.emplace_back(move(new_column));

    // move to the next column in the final result
    cumsum += array_widths_[column_number];
  }
}

/*---------------------------------------------------------------------------*/
// The last step in creation of the XRefStream table is matching the rows to
// the object numbers from the /index entry table

void XRefStream::NumberRows_()
{
  // If there are only two columns append a zero filled column of the same size
  if (result_.size() == 2)
  {
    result_.emplace_back(vector<int>(result_[0].size(), 0));
  }

  // if no index entries, assume start at object 0
  if (object_numbers_.empty())
  {
    // create extra column numbered sequentially from first of objectNumbers
    object_numbers_.resize(result_[0].size());
    iota(object_numbers_.begin(), object_numbers_.end(), 0);
  }

  // Append to our result
  result_.push_back(object_numbers_);
}
