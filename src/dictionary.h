#ifndef PDFR_DICT
#define PDFR_DICT

#include<string>
#include<map>
#include<vector>

class dictionary
{
  std::map<std::string, std::string> DictionaryMap;

public:
  dictionary(const std::string& s);
  dictionary(const std::string& s, const int& i);
  dictionary(std::map<std::string, std::string> d) : DictionaryMap(d) {};
  dictionary();

  std::string get(const std::string& Key);
  bool has(const std::string& Key);
  bool hasRefs(const std::string& Key);
  bool hasInts(const std::string& Key);
  bool hasDictionary(const std::string& Key);
  std::vector<int> getRefs(const std::string& Key);
  std::vector<int> getInts(const std::string& Key);
  std::vector<std::string> getDictKeys();
  dictionary getDictionary(const std::string& Key);
  std::map<std::string, std::string> R_out() {return this->DictionaryMap;}
};

#endif
