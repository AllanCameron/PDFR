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

enum DState     {PREENTRY,
                 QUERYCLOSE,
                 VALUE,
                 MAYBE,
                 START,
                 KEY,
                 PREVALUE,
                 DSTRING,
                 ARRAYVAL,
                 QUERYDICT,
                 SUBDICT,
                 CLOSE,
                 THE_END};

class dictionary
{
  string* s;
  size_t i;
  int minibuf;
  bool keyPending;
  string buf, pendingKey;
  DState state;
  std::map<std::string, std::string> DictionaryMap;
  void tokenize_dict();
  void sortkey(string, DState);
  void assignValue(string, DState);
  void handleMaybe(char);
  void handleStart(char);
  void handleKey(char);
  void handlePrevalue(char);
  void handleValue(char);
  void handleArrayval(char);
  void handleDstring(char);
  void handleQuerydict(char);
  void handleSubdict(char);
  void handleClose(char);

public:
  dictionary(std::string* s);
  dictionary(std::string* s, size_t);
  dictionary(std::map<std::string, std::string> d) : DictionaryMap(d) {};
  dictionary();
  std::string get(const std::string& Key);
  bool has(const std::string& Key);
  bool hasRefs(const std::string& Key);
  bool hasInts(const std::string& Key);
  bool hasDictionary(const std::string& Key);
  std::vector<int> getRefs(const std::string& Key);
  std::vector<int> getInts(const std::string& Key);
  std::vector<float> getNums(const std::string& Key);
  std::vector<std::string> getDictKeys();
  dictionary getDictionary(const std::string& Key);
  std::map<std::string, std::string> R_out() {return this->DictionaryMap;}
};

#endif
