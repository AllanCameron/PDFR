#ifndef PDFR_CRYPTO
#define PDFR_CRYPTO

typedef unsigned long UL;

std::vector<uint8_t> perm(std::string str);
std::vector<uint8_t> upw();
UL rotateLeft(UL x, int y);
UL md5first(UL a, UL b, UL c, UL d, UL e, UL f, UL g);
UL md5second(UL a, UL b, UL c, UL d, UL e, UL f, UL g);
UL md5third(UL a, UL b, UL c,UL d, UL e, UL f,UL g);
UL md5fourth(UL a, UL b, UL c,UL d, UL e, UL f,UL g);
std::vector<uint8_t> md5(std::vector<uint8_t> input);
std::vector<uint8_t> md5(std::string input);
std::vector<uint8_t> rc4(std::vector<uint8_t> msg, std::vector<uint8_t> key);
std::string decryptStream(std::string streamstr, std::vector<uint8_t> key,
                          int objNum, int objGen);


#endif
