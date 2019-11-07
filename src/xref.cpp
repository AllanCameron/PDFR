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
// class, but it has been seperated out to remove clutter and because it
// represents one encapsulated and complex task. Because it is only used to
// help in the construction of an XRef object, it has no public interface. The
// XRef class accesses it as a friend class so no-one needs to worry about it
// (unless it breaks...)

class XRefStream
{
  friend class XRef;  // This class is only constructible via XRef

  shared_ptr<XRef>    xref_;         // Pointer to creating XRef
  vector<vector<int>> raw_matrix_,   // Holds the raw data in (row x col) vecs
                      final_array_,  // Transposed, modulo 256 table
                      result_;       // The main results table for export
  vector<int> array_widths_,         // Bytes per column from /w entry
              object_numbers_;       // vector of object numbers inside stream.
  int         number_of_columns_,    // Number of Columns in table
              predictor_,            // Specifies decoding algorithm
              object_start_;         // Byte offset of XRefStream's container
  Dictionary  dictionary_;           // Dictionary of object containing stream

  XRefStream(shared_ptr<XRef>, int); // Private constructor
  void ReadIndex_();                 // Reads the index entry of main dict
  void ReadParameters_();            // Reads the PNG decoding parameters
  void GetRawMatrix_();              // Reads the stream into data table
  void DiffUp_();                    // Un-diffs the table
  void ModuloTranspose_();           // Transposes table and makes it modulo 256
  void ExpandBytes_();               // Multiply bytes by correct powers of 256
  void MergeColumns_();              // Adds adjacent columns as per parameters
  void NumberRows_();                // Merges object numbers with data table
  vector<vector<int>> Table_();      // Getter for final result
};

/*---------------------------------------------------------------------------*/
// The XRef constructor. It takes the entire file contents as a string
// then sequentially runs the steps in creation of an XRef master map

XRef::XRef(shared_ptr<const string> p_file_string_ptr)
  : file_string_(p_file_string_ptr)
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
  string&& last_50_chars = file_string_->substr(file_string_->size()-50, 50);

  // Use CarveOut() from utilities.h to find the first XRef offset
  string&& xref_string =  CarveOut(last_50_chars, "startxref", "%%EOF");

  // Convert the number string to an int
  vector<int> xref_locations {stoi(xref_string)};

  // If no XRef location is found, then we're stuck. Throw an error.
  if (xref_locations.empty()) throw runtime_error("No XRef entry found");

  // The first dictionary found after any XRef offset is always a trailer
  // dictionary, though sometimes it doubles as an XRefStream dictionary.
  // We make this first one found the canonical trailer dictionary
  trailer_dictionary_ = make_shared<Dictionary>(
                          file_string_, xref_locations[0]);

  // Now we follow the pointers to all xrefs sequentially.
  Dictionary temp_dictionary = *trailer_dictionary_;
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

void XRef::ReadXRefStrings_(int p_start)
{
  // Find the length of XRef in chars
  int length = file_string_->find("startxref", p_start) - p_start;

  // Throw error if no XRef found
  if (length <= 0) throw runtime_error("No object found at location");

  // Extract the XRef string
  string&& fullxref = file_string_->substr(p_start, length);

  // Carve out the actual string
  string xref_string = CarveOut(move(fullxref), "xref", "trailer");

  // If it contains a dictionary, process as a stream, otherwise as a string
  auto finder = xref_string.substr(0, 15).find("<<", 0);
  if (finder != string::npos) ReadXRefFromStream_(p_start);
  else ReadXRefFromString_(xref_string);
}

/*---------------------------------------------------------------------------*/
// Takes an XRef location and if it is a stream, creates an XRefStream object.
// The output of this object is a "table" (vec<vec<int>>) which is parsed
// and added to the main combined XRef table

void XRef::ReadXRefFromStream_(int p_location)
{
  // Calls XRefStream constructor to make the data table from the stream
  auto xref_table = XRefStream(make_shared<XRef>(*this), p_location).Table_();

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

void XRef::ReadXRefFromString_(string& p_xref_string)
{
  auto all_ints = ParseInts(p_xref_string);

  // A valid XRef has >= 4 ints in it and must have an even number of ints
  auto xref_size = all_ints.size();
  if (xref_size % 2) { throw runtime_error(p_xref_string); }

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

/*---------------------------------------------------------------------------*/
// Returns the byte offset for a pdf object

size_t XRef::GetObjectStartByte(int p_object_number) const
{
  auto found = xref_table_.find(p_object_number);

  if (found == xref_table_.end()) throw runtime_error("Object does not exist");

  return found->second.startbyte;
}

/*---------------------------------------------------------------------------*/
// Returns the end byte of an object by finding the first example of the
// word "endobj" after the start of the object

size_t XRef::GetObjectEndByte(int p_object_number) const
{
  auto row = xref_table_.find(p_object_number);

  // throw an error if objnum isn't a valid object
  if (row == xref_table_.end()) throw runtime_error("Object doesn't exist");

  // If the object is in an object stream, return 0;
  if (row->second.in_object) return 0;

  // else find the first match of "endobj" and return
  return file_string_->find("endobj", row->second.startbyte);
}

/*---------------------------------------------------------------------------*/
// If an object is part of an objectstream, this tells us which object forms
// the objectstream.

size_t XRef::GetHoldingNumberOf(int p_object_number) const
{
  auto row = xref_table_.find(p_object_number);

  if (row == xref_table_.end()) throw runtime_error("Object does not exist");

  return row->second.in_object;
}

/*---------------------------------------------------------------------------*/
// Returns vector of all objects listed in the xrefs

vector<int> XRef::GetAllObjectNumbers() const
{
  return GetKeys(this->xref_table_);
}

/*---------------------------------------------------------------------------*/

int XRef::GetStreamLength_(const Dictionary& p_dictionary) const
{
  if (p_dictionary.ContainsReferences("/Length"))
  {
    int length_object_number = p_dictionary.GetReference("/Length");
    size_t first_position = GetObjectStartByte(length_object_number);
    size_t len = file_string_->find("endobj", first_position) - first_position;
    string object_string = file_string_->substr(first_position, len);
    return ParseInts(move(object_string)).back();
  }

  // Thankfully though most lengths are just direct ints
  else return p_dictionary.GetInts("/Length")[0];
}

/*---------------------------------------------------------------------------*/
// Returns the offset of the start location relative to the file start, and the
// length, of the stream belonging to the given object

vector<size_t> XRef::GetStreamLocation(int p_object_start) const
{
  // Get the object dictionary
  Dictionary dictionary = Dictionary(file_string_, p_object_start);

  // If the stream exists, get its start / stop positions as a length-2 array
  if (dictionary.HasKey("stream") && dictionary.HasKey("/Length"))
  {
    size_t stream_length = (size_t) GetStreamLength_(dictionary);
    size_t stream_start  = (size_t) dictionary.GetInts("stream")[0];
    return vector<size_t>  {stream_start, stream_length};
  }
  return vector<size_t> {0,0}; // if no length, return empty length-2 array
}

/*---------------------------------------------------------------------------*/
// Wrapper for Encryption object so that XRef is the only class that uses it

void XRef::Decrypt(string& p_stream, int p_object, int p_generation) const
{
  encryption_->DecryptStream(p_stream, p_object, p_generation);
}

/*---------------------------------------------------------------------------*/
// Getter for encryption state

bool XRef::IsEncrypted() const
{
  if(encryption_) return true; else return false;
}

/*---------------------------------------------------------------------------*/
// getter function to access the trailer dictionary - a private data member

Dictionary XRef::GetTrailer() const
{
  return *trailer_dictionary_;
}

/*---------------------------------------------------------------------------*/
// This co-ordinates the creation of the encryption dictionary (if any), and
// the building plus testing of the file decryption key (if any)

void XRef::CreateCrypto_()
{
   // if there's no encryption dictionary, there's nothing else to do
  if (!trailer_dictionary_->HasKey("/Encrypt")) return;

  int encryption_number = trailer_dictionary_->GetReference("/Encrypt");

  // No encryption dict - exception?
  if (xref_table_.find(encryption_number) == xref_table_.end()) return;

  // mark file as encrypted and read the encryption dictionary
  size_t starts_at = GetObjectStartByte(encryption_number);
  Dictionary&& dictionary = Dictionary(file_string_, starts_at);
  encryption_ = make_shared<Crypto>(move(dictionary), *trailer_dictionary_);
}

/*---------------------------------------------------------------------------*/
// simple getter for the output of an XRefStream

vector<vector<int>> XRefStream::Table_()
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

XRefStream::XRefStream(shared_ptr<XRef> p_xref, int p_starts_at)
  : xref_(p_xref),
    number_of_columns_(0),
    predictor_(0),
    object_start_(p_starts_at),
    dictionary_(Dictionary(xref_->File(), object_start_))
{
  // If there is no /W entry, we don't know how to interpret the stream.
  if (!dictionary_.ContainsInts("/W")) throw runtime_error("No /W entry found.");

  ReadIndex_();       // Reads Index so we know which objects are in stream
  ReadParameters_();  // Reads the PNG decoding parameters
  GetRawMatrix_();    // Arranges the raw data in stream into correct table form
  DiffUp_();          // Undiffs the raw data
  ModuloTranspose_(); // Transposes table which ensuring all numbers are <256
  ExpandBytes_();     // Multiplies bytes according to their position
  MergeColumns_();    // Sums adjacent columns that represent large numbers
  NumberRows_();      // Marries rows to object numbers from the /Index entry
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

void XRefStream::ReadIndex_()
{
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
}

/*---------------------------------------------------------------------------*/
// We next read two parameters we need from the /DecodeParms entry of the
// stream dictionary: the number of columns in the table and the predictor
// number, which gives the method used to decode the stream

void XRefStream::ReadParameters_()
{
  string sub_dictionary_string = "<<>>";

  if (dictionary_.HasKey("/DecodeParms"))
  {
    sub_dictionary_string = dictionary_.GetString("/DecodeParms");
  }

  Dictionary sub_dictionary(make_shared<string>(sub_dictionary_string));

  if (sub_dictionary.ContainsInts("/Columns"))
  {
    number_of_columns_ = sub_dictionary.GetInts("/Columns")[0];
  }

  if (sub_dictionary.ContainsInts("/Predictor"))
  {
    predictor_ = sub_dictionary.GetInts("/Predictor")[0];
  }
}

/*---------------------------------------------------------------------------*/
// Get the raw data from the stream and arrange it in a table as directed by
// the /W ("widths") entry in the main dictionary

void XRefStream::GetRawMatrix_()
{
  // Obtain the raw stream data
  auto stream_location = xref_->GetStreamLocation(object_start_);
  string stream = xref_->File()->substr(stream_location[0], stream_location[1]);

  // Applies decompression to stream if needed
  if (dictionary_.GetString("/Filter").find("/FlateDecode", 0) != string::npos)
  {
    FlateDecode(&stream);
  }

  // Convert stream to bytes then bytes to ints
  vector<uint8_t> conv(stream.begin(), stream.end());
  vector<int> int_stream(conv.begin(), conv.end());

  // Read the /W entry to get the width in bytes of each column in the table
  // check the widths for any zero values and skip them if present
  array_widths_ = dictionary_.GetInts("/W");
  auto new_end_marker = remove(array_widths_.begin(), array_widths_.end(), 0);
  array_widths_.erase(new_end_marker, array_widths_.end());

  // if no record of column numbers, infer from number of /W entries
  if (number_of_columns_ == 0)
  {
    for (auto entry : array_widths_) number_of_columns_ += entry;
  }

  // Predictors above 10 require an extra column
  if (predictor_ > 9) number_of_columns_++;

  if (number_of_columns_ == 0) throw runtime_error("divide by zero error");

  // Gets number of rows
  int number_of_rows = int_stream.size() / number_of_columns_;

  // Ensures rectangular table
  if ((size_t)(number_of_rows * number_of_columns_) != int_stream.size())
  {
    throw runtime_error("Unmatched row and column numbers");
  }

  // Fills the raw matrix with stream data
  for (int i = 0; i < number_of_rows; ++i)
  {
    raw_matrix_.emplace_back(int_stream.begin() + number_of_columns_ * i,
                             int_stream.begin() + number_of_columns_ * (i + 1));
  }
}

/*---------------------------------------------------------------------------*/
// The PNG algorithm involves finding the difference between adjacent cells
// and storing this instead of the actual number. This function reverses that
// differencing by calculating the cumulative sums of the columns

void XRefStream::DiffUp_()
{
  if (predictor_ == 12)
  {
    // For each row
    for (size_t row = 1; row < raw_matrix_.size(); ++row )
    {
      // Create references for this row and the row above it
      auto& this_row = raw_matrix_.at(row);
      auto& row_above = raw_matrix_.at(row - 1);

      // Take each entry & add the entry above
      for (size_t column = 0; column < this_row.size(); ++column)
      {
        this_row.at(column) += row_above.at(column);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// transposes the matrix and makes it modulo 256.

void XRefStream::ModuloTranspose_()
{
  for (size_t i = 0; i < raw_matrix_.at(0).size(); ++i)
  {
    // Create a new column vector
    vector<int> temp_column;

    // Then for each entry in the row make it modulo 256 and push to new column
    for (auto& j : raw_matrix_) temp_column.push_back(j[i] & 0x00ff);

    // the new column is pushed to the final array unless it is the first
    // column (which is skipped when the predictor is > 9)
    if (predictor_ < 10 || i > 0) final_array_.push_back(temp_column);
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
