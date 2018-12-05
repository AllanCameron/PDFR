#ifndef PDFR_GS
#define PDFR_GS

#include "pdfr.h"


class GraphicsState
{
public:
  Rcpp::DataFrame db;
  GraphicsState(page& pag);
  page p;

private:
  font wfont;
  std::vector<std::vector<float>> gs, statehx;
  std::vector<float> xvals, yvals, fontsize, widths, Tmstate, Tdstate, R,
  left, right, bottom, size, width, fontsizestack, initstate;
  std::vector<std::string> fontname, stringres, text, fonts, fontstack;
  std::vector<int> leftmatch, rightmatch;
  int PRstate;
  float Tl, Tw, Th, Tc, currfontsize;
  std::string currentfont;
  Instructionset Instructions;
  Instructionset tokenize(std::string s);
  void tokenize_array(std::vector<std::string> &ttype,
                      std::vector<std::string> &token, std::string &s);
  Instructionset parser(std::vector<std::string> token,
                        std::vector<std::string> ttype);
  void InstructionReader(page& p, Instructionset I);
  void Q(page& p);
  void q();
  void Td(std::string Ins, std::vector<std::string>& Operands);
  void BT();
  void TJ(page& pag, std::vector<std::vector<std::string>>& i);
  void Tf(page& pag, std::vector<std::string>& Operands);
  void MakeGS();
  void clump(int a);
  void Do(std::string& xo);

};

#endif
