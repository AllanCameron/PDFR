//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR dictionary header file                                              //
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

#ifndef PDFR_DICT
#define PDFR_DICT

#include<string>
#include<map>
#include<unordered_map>
#include<vector>


enum Types {Boolean, Name, String, Number, Reference, Array, Dictionary};

class dictionary
{
  unordered_map<string, string> DictionaryMap;
  void tokenize_dict(const string&, unsigned);

public:
  dictionary(const string&);
  dictionary(const string&, const int&);
  dictionary(unordered_map<string, string> d) : DictionaryMap(d) {};
  dictionary();

  string get(const string& Key);
  bool has(const string& Key);
  bool hasRefs(const string& Key);
  bool hasInts(const string& Key);
  bool hasDictionary(const string& Key);
  vector<int> getRefs(const string& Key);
  vector<int> getInts(const string& Key);
  vector<float> getNums(const string& Key);
  vector<string> getDictKeys();
  dictionary getDictionary(const string& Key);
  unordered_map<string, string> R_out() {return this->DictionaryMap;}
};


#endif
