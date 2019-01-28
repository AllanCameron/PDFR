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
  friend class xref;  // This class is only constructible from xref...
  xref* XR;           // ...to which it needs a pointer
  dictionary dict,    // The dictionary of the object containing the xrefstream
             subdict; // The decode parameters dictionary, an entry in dict
  std::vector<std::vector<int>> rawMatrix,  // Holds the raw data in r x c vecs
                                finalArray, // transposed, modulo 256 table
                                result;     // The main results table
  std::vector<int> arrayWidths,   // bytes per column from /w
                   objectNumbers, // vector of object numbers.
                   indexEntries;  // the ints telling us the objects in the xref
  int ncols,       // columns in table
      nrows,       // rows in table
      predictor,   // predictor number - specifies decoding algorithm
      objstart;    // byte offset of the xrefstream's containing object
  void getIndex();          // reads the index entry of main dict
  void getParms();          // reads the PNG decoding parameters
  void getRawMatrix();      // reads the stream into appropriately-sized table
  void diffup();            // un-diffs the table
  void modulotranspose();   // transposes the table and makes entries modulo 256
  void expandbytes();       // multiply bytes by correct powers of 256
  void mergecolumns();      // add adjacent colums as guided by parameters
  void numberRows();        // merge object numbers with data table

  // private creator function
  xrefstream(xref*, int objstart);

  // getter of final result
  std::vector<std::vector<int>> table();
};

/*---------------------------------------------------------------------------*/
// the xref creator function. It takes the entire file contents as a string
// then sequentially runs the steps in creation of an xref master map

xref::xref(string* s) : fs(s)
{
  locateXrefs();           // find all xrefs
  xrefstrings();           // get the strings containing all xrefs
  xrefIsstream();          // find which are streams
  buildXRtable();          // create the xrefs from strings and / or streams
  get_crypto();          // get the file key, needed for decryption of streams
  Xreflocations.clear();   //-----//
  Xrefstrings.clear();            //--> clear data used only in construction
  XrefsAreStreams.clear(); //-----//
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
  // get last 50 chars of the file
  std::string&& partial = fs->substr(fs->size() - 50, 50);

  // use carveout() from utilities.h to find the first xref offset
  std::string&& xrefstring =  carveout(partial, "startxref", "%%EOF");

  // convert the number string to an int
  Xreflocations.emplace_back(stoi(xrefstring));
  if(Xreflocations.empty()) throw runtime_error("No xref entry found");

  // the first dictionary found after any xref offset is always a trailer
  // dictionary, though sometimes it doubles as an xrefstream dictionary.
  // We make the first one found the canonical trailer dictionary
  TrailerDictionary = dictionary(fs, Xreflocations[0]);
  dictionary tempdict = TrailerDictionary;

  // Follow pointers to all xrefs sequentially.
  while (true)
    if(tempdict.hasInts("/Prev"))
    {
      Xreflocations.emplace_back(tempdict.getInts("/Prev")[0]);
      tempdict = dictionary(fs, Xreflocations.back());
    }
    else break;
}

/*---------------------------------------------------------------------------*/
// Whatever form the xrefs take (plain or xrefstream), we first get their
// raw contents as strings from the xref locations

void xref::xrefstrings()
{
  // get a string from each xref location or die
  for(auto i : Xreflocations)
  {
    int len = firstmatch(*fs, "startxref", i) - i; // length of xref in chars

    // Throw error if no xref found
    if (len <= 0) throw std::runtime_error("No object found at location");

    // extract the xref string from the file string
    string fullxref = fs->substr(i, len);

    // stick a trimmed version of the xref onto Xrefstrings
    // Note the carveout should leave xrefstreams unaltered
    Xrefstrings.emplace_back(carveout(fullxref, "xref", "trailer"));
  }
}

/*---------------------------------------------------------------------------*/
// Simple check of whether each xref string is a stream or a plaintext

void xref::xrefIsstream()
{
  for(auto i : Xrefstrings) // If first 15 chars contains << it's a stream
    XrefsAreStreams.emplace_back(i.substr(0, 15).find("<<", 0) != string::npos);
}

/*---------------------------------------------------------------------------*/
// Takes an xref location and if it is a stream, creates an xrefstream object.
// The output of this object is a "table" (vec<vec<int>>) which is parsed
// and added to the main combined xref table

void xref::xrefFromStream(int xrefloc)
{
  // Gets the table from the stream
  vector<vector<int>> xreftable = xrefstream(this, xrefloc).table();

  // Throws if something's broken
  if(xreftable.empty()) throw std::runtime_error("xreftable empty");

  // Fills the main data map from the table --------//
  size_t xrts = xreftable[0].size();                // define loop length
  for (size_t j = 0; j < xrts; j++)                 //
  {                                                 //
    xrefrow txr;                                    // maps' values are xrefrows
    txr.object = xreftable[3][j];                   // 4th col of table == obj
    txr.startbyte = txr.in_object = xreftable[1][j];// 2nd col of table == inobj
    if (xreftable[0][j] == 1) txr.in_object = 0;    // 1st col == isinobj
    if (xreftable[0][j] == 2) txr.startbyte = 0;    // if isinobj, no startbyte
    xreftab[txr.object] = txr;                      // write to main map
    objenum.emplace_back(txr.object);               // write to vec of objnums
  }                                                 //
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
  std::vector<int> inuse, byteloc, objnumber;
  std::vector<int> allints = getints(xstr); // use getints() from utilities
  if(allints.size() < 4) return;            // valid xref has >= 4 ints in it
  int startingobj = allints[0];             // the first object == first int

  // The number of ints must be even or the xref is malformed
  if(allints.size() % 2)
    throw runtime_error("Malformed xref");

  // This loop starts on the second row of the table. Even numbers are the
  // byte offsets and odd numbers are the in_use numbers
  int bytestore = 0;
  for(size_t i = 2; i < allints.size(); i++)
  {
    if(i % 2 == 0)
      bytestore = allints[i]; // store byte offsets
    else
    {
      if(allints[i] < 65535) // indicates object in use
      {
        xrefrow txr; // the map is of object numbers to xrefrow
        txr.object = startingobj + (i / 2) - 1; // zero-indexed row + start
        txr.startbyte = bytestore; // use number from last loop
        txr.in_object = 0;         // not in an objectstream
        xreftab[txr.object] = txr;     // write to main data map
        objenum.push_back(txr.object); // write to object enumerator
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// This is the main loop that co-ordinates parsing of the xrefs once they have
// been located and extracted. It checks each location to decide whether it is
// a stream or plaintext and calls the correct parser once it knows

void xref::buildXRtable()
{
  for(size_t i = 0; i < Xreflocations.size(); i++) // for each location
  {
    if(XrefsAreStreams[i])
      xrefFromStream(Xreflocations[i]);   //----------------------------------//
    else                                  // Parse string or stream as needed //
      xrefFromString(Xrefstrings[i]);     //----------------------------------//
  }
}

/*---------------------------------------------------------------------------*/
// Simple determiner of whether an object is present in the built xref

bool xref::objectExists(int objnum)
{
  return xreftab.find(objnum) != xreftab.end();
}

/*---------------------------------------------------------------------------*/
// Returns the byte offset for a pdf object

size_t xref::getStart(int objnum)
{
  if(!objectExists(objnum)) throw std::runtime_error("Object does not exist");
  return (size_t) xreftab.at(objnum).startbyte;
}

/*---------------------------------------------------------------------------*/
// Returns the end byte of an object by finding the first example of the
// word "endobj" after the start of the object

size_t xref::getEnd(int objnum)
{
  // throw an error if objnum isn't a valid object
  if(!objectExists(objnum)) throw std::runtime_error("Object does not exist");

  // If the object is in an object stream, return 0;
  if(xreftab[objnum].in_object) return 0;

  // else find the first match of "endobj" and return
  return (int) firstmatch(*fs, "endobj", xreftab[objnum].startbyte);
}

/*---------------------------------------------------------------------------*/
// Returns whether the requested object is located in another object's stream

bool xref::isInObject(int objnum)
{
  if(!objectExists(objnum)) throw std::runtime_error("Object does not exist");
  return xreftab.at(objnum).in_object != 0;
}

/*---------------------------------------------------------------------------*/
// If an object is part of an objectstream, this tells us which object forms
// the objectstream.

size_t xref::inObject(int objnum)
{
  if(!objectExists(objnum)) throw std::runtime_error("Object does not exist");
  return (size_t) xreftab.at(objnum).in_object;
}

/*---------------------------------------------------------------------------*/
// Returns vector of all objects listed in the xrefs

std::vector<int> xref::getObjects()
{
  return objenum;
}

/*---------------------------------------------------------------------------*/
// returns the offset of the start and stop locations relative to the file
// start, of the stream belonging to the given object

vector<size_t> xref::getStreamLoc(int objstart)
{
  dictionary dict = dictionary(fs, objstart); // get the object dictionary
  if(dict.has("stream"))     // this is created automatically in dictionary
    if(dict.has("/Length"))  // sanity check; should length if there is stream
    {
      int streamlen;
      if(dict.hasRefs("/Length")) // sometimes length is given as another object
      {
        int lengthob = dict.getRefs("/Length")[0]; // finds reference
        size_t firstpos = getStart(lengthob);      // gets start of length obj
        size_t lastpos = firstmatch(*fs, "endobj", firstpos); // and end of it
        size_t len = lastpos - firstpos;  // from which we can get its length
        string objstr = fs->substr(firstpos, len); // from which we get a string
        streamlen = getints(objstr).back(); // from which we get the number
      }
      // thankfully though most lengths are just direct ints
      else streamlen = dict.getInts("/Length")[0];

      // now get stream's start
      int strmstart = dict.getInts("stream")[0];

      // now we can return a length-2 vector of the stream start + end
      vector<size_t> res = {(size_t) strmstart, (size_t) strmstart + streamlen};
      return res;
    }
  // No stream; return a zero-pair.
  vector<size_t> res = {0,0};
  return res;
}

/*---------------------------------------------------------------------------*/
// Wrapper for Encryption object so that xref is the only class that uses it

void xref::decrypt(std::string& s, int obj, int gen)
{
  encryption.decryptStream(s, obj, gen);
}

/*---------------------------------------------------------------------------*/
// Getter for encryption state

bool xref::isEncrypted()
{
  return this->encrypted;
}

/*---------------------------------------------------------------------------*/
// getter function to access the trailer dictionary - a private data member

dictionary xref::trailer()
{
  return TrailerDictionary;
}

/*---------------------------------------------------------------------------*/
// This co-ordinates the creation of the encryption dictionary (if any), and
// the building plus testing of the file decryption key (if any)

void xref::get_crypto()
{
   // if there's no encryption dictionary, there's nothing else to do
  if(!TrailerDictionary.has("/Encrypt")) return;
  int encnum = TrailerDictionary.getRefs("/Encrypt").at(0);
  if(!objectExists(encnum)) return; // No encryption dict - should be exception

  // mark the file as encrypted and read the encryption dictionary
  encrypted = true;
  dictionary encdict = dictionary(fs, getStart(encnum));
  encryption = crypto(encdict, TrailerDictionary);
}

/*---------------------------------------------------------------------------*/
// simple getter for the output of an xrefstream

std::vector<std::vector<int>> xrefstream::table()
{
  return result;
}

/*---------------------------------------------------------------------------*/
// xrefstream constructor. Note that encryption does not apply to xrefstreams.
// The constructor calls several functions which together comprise the
// PNG decompression algorithm. They are seperated out to prevent one large
// hairball function being created that is difficult to debug.

xrefstream::xrefstream(xref* Xref, int starts) :
  XR(Xref), ncols(0), predictor(1), objstart(starts)
{
  dict = dictionary(XR->fs, objstart);         // get the xrefstream dict
  string decodestring = "<<>>";
  if(dict.has("/DecodeParms"))
    decodestring = dict.get("/DecodeParms"); // string for subdict
  subdict = dictionary(&decodestring);            // get parameters (subdict)
  getIndex(); // read Index entry of dict so we know which objects are in stream
  getParms(); // read the PNG decoding parameters

  // If there is no /W entry, we don't know how to interpret the stream. Abort!
  if(!dict.hasInts("/W")) throw std::runtime_error("Malformed xref stream");

  getRawMatrix(); // arrange the raw data in the stream into correct table form
  if(predictor == 12) diffup(); // Undiff the raw data
  // throw an error with unimplemented predictors - this will serve as
  // a reminder to use the document that throws the error to build the
  // correct diff algorithm - pointless doing this if we can't test it yet
  if(predictor != 1 && predictor != 12)
    throw std::runtime_error("predictors other than 1 and 12 not implemented");
  modulotranspose(); // transposes table which ensuring all numbers are <256
  expandbytes(); // multiply numbers to intended size based on their position
  mergecolumns(); // add up adjacent columns that represent large numbers
  numberRows(); // marry the rows to the object numbers from the /Index entry
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
  indexEntries = dict.getInts("/Index");        // Gets the numbers in the entry
  if(indexEntries.empty()) objectNumbers = {0}; // Sanity check - ?exception
  else
  {
    int firstObject = 0;
    // Loop to expand the shorthand list of objects into a vector
    for(size_t i = 0; i < indexEntries.size(); i++)
    {
      // even indices represent the first object being described in a group
      if(i % 2 == 0)
        firstObject = indexEntries[i];
      else
      {
        // odd indices are the number of objects present
        // This subloop just adds sequentially to the first object in the group
        for(auto j = 0; j < indexEntries[i]; j++)
          objectNumbers.emplace_back(firstObject + j);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
// We next read two parameters we need from the /DecodeParms entry of the
// stream dictionary: the number of columns in the table and the predictor
// number, which gives the method used to decode the stream

void xrefstream::getParms()
{
  if(subdict.hasInts("/Columns"))
    ncols = subdict.getInts("/Columns")[0];
  if(subdict.hasInts("/Predictor"))
    predictor = subdict.getInts("/Predictor")[0];
}

/*---------------------------------------------------------------------------*/
// Get the raw data from the stream and arrange it in a table as directed by
// the /W ("widths") entry in the main dictionary

void xrefstream::getRawMatrix()
{
  // finds location of stream beginning and end
  std::vector<size_t> sl = XR->getStreamLoc(objstart);

  // extracts stream from file
  std::string SS = XR->fs->substr(sl[0], sl[1] - sl[0]);

  // gets the containing object's dictionary
  dictionary dict = dictionary(XR->fs, objstart);

  // applies decompression to stream if needed
  if(dict.get("/Filter").find("/FlateDecode", 0) != string::npos)
    SS = FlateDecode(SS);

  // turn the string into an array of bytes
  std::vector<uint8_t> conv(SS.begin(), SS.end());

  // turn the bytes into an array of ints
  std::vector<int> intstrm(conv.begin(), conv.end());

  // read the /W entry to get the width in bytes of each column in the table
  std::vector<int>&& tmparraywidths = dict.getInts("/W");

  // check the widths for any zero values and skip them if present
  for (auto i : tmparraywidths) if (i > 0) arrayWidths.push_back(i);

  // if no record of column numbers, infer from number of /W entries >0
  if (ncols == 0) for (auto i : arrayWidths) ncols += i;

  // the predictor algorithms above 10 require an extra marginal column
  if(predictor > 9) ncols++;

  // sanity check
  if(ncols == 0) throw std::runtime_error("divide by zero error");

  // nrows is just the stream length / ncols (must be no remainder)
  int nrows = intstrm.size() / ncols;
  if ((size_t)(nrows * ncols) != intstrm.size())
    throw runtime_error("Unmatched row and column numbers");

  // now fill the raw matrix with the stream data
  for(int i = 0; i < nrows; i++)
    rawMatrix.emplace_back(intstrm.begin() + ncols * i,
                           intstrm.begin() + ncols * (i + 1));
}

/*---------------------------------------------------------------------------*/
// The PNG algorithm involves finding the difference between adjacent cells
// and storing this instead of the actual number. This function reverses that
// differencing by calculating the cumulative sums of the columns

void xrefstream::diffup()
{
  for(size_t i = 1; i < rawMatrix.size(); i++ )          // for each row...
    for(size_t j = 0; j < rawMatrix.at(i).size(); j++)   // take each entry...
      rawMatrix.at(i).at(j) += rawMatrix.at(i - 1).at(j);// & add the one above
}

/*---------------------------------------------------------------------------*/
// transposes the matrix and makes it modulo 256.

void xrefstream::modulotranspose()
{
  for(size_t i = 0; i < rawMatrix.at(0).size(); i++)  // for each row...
  {
    std::vector<int> tempcol;   // create a new column vector

     // then for each entry in the row make it modulo 256 and push to new column
    for(size_t j = 0; j < rawMatrix.size(); j++)
      tempcol.push_back(rawMatrix.at(j).at(i) % 256);

    // the new column is pushed to the final array unless it is the first
    // column (which is skipped when the predictor is > 9)
    if(predictor < 10 || i > 0) finalArray.push_back(tempcol);
  }
}

/*---------------------------------------------------------------------------*/
// Multiplies the entries in the matrix according to the width in bytes of the
// numbers represented according to the /W entry

void xrefstream::expandbytes()
{
  // The bytes in the matrix represent powers of 256 depending on the number
  // being represented's width in bytes
  std::vector<int> byteVals {16777216, 65536, 256, 1};

  // create the multiplier fot each column with 1:1 correspondence
  std::vector<int> columnConst;
  for(auto i: arrayWidths)
    columnConst.insert(columnConst.end(), byteVals.end() - i, byteVals.end());

  // now multiply the entries cell-by-cell
  for(size_t i = 0; i < finalArray.size(); i++)
    for(auto &j : finalArray.at(i))
      j *= columnConst.at(i);
}

/*---------------------------------------------------------------------------*/
// Now the matrix has the correct values but the columns representing higher
// bytes need to be added to those with lower bytes

void xrefstream::mergecolumns()
{
  int cumsum = 0;
  for(auto i : arrayWidths) // for each of the final columns
  {
    // take the next column from the unmerged array
    std::vector<int> newcolumn = finalArray.at(cumsum);
    if(i > 1) // if the final column is more than 1 byte wide...
    {
      for(int j = 1; j < i; j++) // for each width value above 1
      {
        size_t fas = finalArray.at(cumsum + j).size(); // take another column
        // add it to the previous one
        for(size_t k = 0; k < fas; k++)
          newcolumn.at(k) += finalArray.at(cumsum + j).at(k);
      }
    }
    // this is now one of our final columns, so push it to the result
    result.push_back(newcolumn);

    // move to the next column in the final result
    cumsum += i;
  }
  if(result.size() == 2) // if there are only two columns add a zero filled col
  {
    std::vector<int> zeroArray(result.at(0).size(), 0); // array of 0s
    result.push_back(zeroArray);
  }
}

/*---------------------------------------------------------------------------*/
// The last step in creation of the xrefstream table is matching the rows to
// the object numbers from the /index entry table

void xrefstream::numberRows()
{
  vector<int> objectIndex;
  if(indexEntries.empty()) // if no index entries, assume start at object 0
  {
    for(size_t i = 0; i < result.at(0).size(); i++) // fill sequentially from 0
      objectIndex.push_back(i + objectNumbers.at(0));
    result.push_back(objectIndex);
  }
  else // simply push previously calculated object numbers
    result.push_back(objectNumbers);
}


