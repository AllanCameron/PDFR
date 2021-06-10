// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// GetXrefFromString
Rcpp::DataFrame GetXrefFromString(const std::string& file_name);
RcppExport SEXP _PDFR_GetXrefFromString(SEXP file_nameSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::string& >::type file_name(file_nameSEXP);
    rcpp_result_gen = Rcpp::wrap(GetXrefFromString(file_name));
    return rcpp_result_gen;
END_RCPP
}
// GetXrefFromRaw
Rcpp::DataFrame GetXrefFromRaw(const std::vector<uint8_t>& raw_file);
RcppExport SEXP _PDFR_GetXrefFromRaw(SEXP raw_fileSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::vector<uint8_t>& >::type raw_file(raw_fileSEXP);
    rcpp_result_gen = Rcpp::wrap(GetXrefFromRaw(raw_file));
    return rcpp_result_gen;
END_RCPP
}
// GetObjectFromString
Rcpp::List GetObjectFromString(const std::string& file_name, int object_number);
RcppExport SEXP _PDFR_GetObjectFromString(SEXP file_nameSEXP, SEXP object_numberSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::string& >::type file_name(file_nameSEXP);
    Rcpp::traits::input_parameter< int >::type object_number(object_numberSEXP);
    rcpp_result_gen = Rcpp::wrap(GetObjectFromString(file_name, object_number));
    return rcpp_result_gen;
END_RCPP
}
// GetObjectFromRaw
Rcpp::List GetObjectFromRaw(const std::vector<uint8_t>& raw_file, int object_number);
RcppExport SEXP _PDFR_GetObjectFromRaw(SEXP raw_fileSEXP, SEXP object_numberSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::vector<uint8_t>& >::type raw_file(raw_fileSEXP);
    Rcpp::traits::input_parameter< int >::type object_number(object_numberSEXP);
    rcpp_result_gen = Rcpp::wrap(GetObjectFromRaw(raw_file, object_number));
    return rcpp_result_gen;
END_RCPP
}
// GetPdfPageFromString
Rcpp::List GetPdfPageFromString(const std::string& file_name, int page_number, bool each_glyph);
RcppExport SEXP _PDFR_GetPdfPageFromString(SEXP file_nameSEXP, SEXP page_numberSEXP, SEXP each_glyphSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::string& >::type file_name(file_nameSEXP);
    Rcpp::traits::input_parameter< int >::type page_number(page_numberSEXP);
    Rcpp::traits::input_parameter< bool >::type each_glyph(each_glyphSEXP);
    rcpp_result_gen = Rcpp::wrap(GetPdfPageFromString(file_name, page_number, each_glyph));
    return rcpp_result_gen;
END_RCPP
}
// GetPdfPageFromRaw
Rcpp::List GetPdfPageFromRaw(const std::vector<uint8_t>& raw_file, int page_number, bool atoms);
RcppExport SEXP _PDFR_GetPdfPageFromRaw(SEXP raw_fileSEXP, SEXP page_numberSEXP, SEXP atomsSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::vector<uint8_t>& >::type raw_file(raw_fileSEXP);
    Rcpp::traits::input_parameter< int >::type page_number(page_numberSEXP);
    Rcpp::traits::input_parameter< bool >::type atoms(atomsSEXP);
    rcpp_result_gen = Rcpp::wrap(GetPdfPageFromRaw(raw_file, page_number, atoms));
    return rcpp_result_gen;
END_RCPP
}
// GetGlyphMap
Rcpp::DataFrame GetGlyphMap(const std::string& file_name, int page_number);
RcppExport SEXP _PDFR_GetGlyphMap(SEXP file_nameSEXP, SEXP page_numberSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::string& >::type file_name(file_nameSEXP);
    Rcpp::traits::input_parameter< int >::type page_number(page_numberSEXP);
    rcpp_result_gen = Rcpp::wrap(GetGlyphMap(file_name, page_number));
    return rcpp_result_gen;
END_RCPP
}
// GetPageStringFromString
std::string GetPageStringFromString(const std::string& file_name, int page_number);
RcppExport SEXP _PDFR_GetPageStringFromString(SEXP file_nameSEXP, SEXP page_numberSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::string& >::type file_name(file_nameSEXP);
    Rcpp::traits::input_parameter< int >::type page_number(page_numberSEXP);
    rcpp_result_gen = Rcpp::wrap(GetPageStringFromString(file_name, page_number));
    return rcpp_result_gen;
END_RCPP
}
// GetPageStringFromRaw
std::string GetPageStringFromRaw(const std::vector<uint8_t>& raw_file, int page_number);
RcppExport SEXP _PDFR_GetPageStringFromRaw(SEXP raw_fileSEXP, SEXP page_numberSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::vector<uint8_t>& >::type raw_file(raw_fileSEXP);
    Rcpp::traits::input_parameter< int >::type page_number(page_numberSEXP);
    rcpp_result_gen = Rcpp::wrap(GetPageStringFromRaw(raw_file, page_number));
    return rcpp_result_gen;
END_RCPP
}
// GetPdfDocumentFromString
Rcpp::DataFrame GetPdfDocumentFromString(const std::string& file_name);
RcppExport SEXP _PDFR_GetPdfDocumentFromString(SEXP file_nameSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::string& >::type file_name(file_nameSEXP);
    rcpp_result_gen = Rcpp::wrap(GetPdfDocumentFromString(file_name));
    return rcpp_result_gen;
END_RCPP
}
// GetPdfDocumentFromRaw
Rcpp::DataFrame GetPdfDocumentFromRaw(const std::vector<uint8_t>& file_name);
RcppExport SEXP _PDFR_GetPdfDocumentFromRaw(SEXP file_nameSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::vector<uint8_t>& >::type file_name(file_nameSEXP);
    rcpp_result_gen = Rcpp::wrap(GetPdfDocumentFromRaw(file_name));
    return rcpp_result_gen;
END_RCPP
}
// GetPdfBoxesFromString
Rcpp::DataFrame GetPdfBoxesFromString(const std::string& file_name, int page_number);
RcppExport SEXP _PDFR_GetPdfBoxesFromString(SEXP file_nameSEXP, SEXP page_numberSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::string& >::type file_name(file_nameSEXP);
    Rcpp::traits::input_parameter< int >::type page_number(page_numberSEXP);
    rcpp_result_gen = Rcpp::wrap(GetPdfBoxesFromString(file_name, page_number));
    return rcpp_result_gen;
END_RCPP
}
// GetPdfBoxesFromRaw
Rcpp::DataFrame GetPdfBoxesFromRaw(const std::vector<uint8_t>& file_name, int page_number);
RcppExport SEXP _PDFR_GetPdfBoxesFromRaw(SEXP file_nameSEXP, SEXP page_numberSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::vector<uint8_t>& >::type file_name(file_nameSEXP);
    Rcpp::traits::input_parameter< int >::type page_number(page_numberSEXP);
    rcpp_result_gen = Rcpp::wrap(GetPdfBoxesFromRaw(file_name, page_number));
    return rcpp_result_gen;
END_RCPP
}
// GetRectangles
Rcpp::List GetRectangles(const std::string& file_name, int page_number);
RcppExport SEXP _PDFR_GetRectangles(SEXP file_nameSEXP, SEXP page_numberSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::string& >::type file_name(file_nameSEXP);
    Rcpp::traits::input_parameter< int >::type page_number(page_numberSEXP);
    rcpp_result_gen = Rcpp::wrap(GetRectangles(file_name, page_number));
    return rcpp_result_gen;
END_RCPP
}
// TestPath
Rcpp::List TestPath();
RcppExport SEXP _PDFR_TestPath() {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(TestPath());
    return rcpp_result_gen;
END_RCPP
}
// stopCpp
void stopCpp();
RcppExport SEXP _PDFR_stopCpp() {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    stopCpp();
    return R_NilValue;
END_RCPP
}

RcppExport SEXP run_testthat_tests();

static const R_CallMethodDef CallEntries[] = {
    {"_PDFR_GetXrefFromString", (DL_FUNC) &_PDFR_GetXrefFromString, 1},
    {"_PDFR_GetXrefFromRaw", (DL_FUNC) &_PDFR_GetXrefFromRaw, 1},
    {"_PDFR_GetObjectFromString", (DL_FUNC) &_PDFR_GetObjectFromString, 2},
    {"_PDFR_GetObjectFromRaw", (DL_FUNC) &_PDFR_GetObjectFromRaw, 2},
    {"_PDFR_GetPdfPageFromString", (DL_FUNC) &_PDFR_GetPdfPageFromString, 3},
    {"_PDFR_GetPdfPageFromRaw", (DL_FUNC) &_PDFR_GetPdfPageFromRaw, 3},
    {"_PDFR_GetGlyphMap", (DL_FUNC) &_PDFR_GetGlyphMap, 2},
    {"_PDFR_GetPageStringFromString", (DL_FUNC) &_PDFR_GetPageStringFromString, 2},
    {"_PDFR_GetPageStringFromRaw", (DL_FUNC) &_PDFR_GetPageStringFromRaw, 2},
    {"_PDFR_GetPdfDocumentFromString", (DL_FUNC) &_PDFR_GetPdfDocumentFromString, 1},
    {"_PDFR_GetPdfDocumentFromRaw", (DL_FUNC) &_PDFR_GetPdfDocumentFromRaw, 1},
    {"_PDFR_GetPdfBoxesFromString", (DL_FUNC) &_PDFR_GetPdfBoxesFromString, 2},
    {"_PDFR_GetPdfBoxesFromRaw", (DL_FUNC) &_PDFR_GetPdfBoxesFromRaw, 2},
    {"_PDFR_GetRectangles", (DL_FUNC) &_PDFR_GetRectangles, 2},
    {"_PDFR_TestPath", (DL_FUNC) &_PDFR_TestPath, 0},
    {"_PDFR_stopCpp", (DL_FUNC) &_PDFR_stopCpp, 0},
    {"run_testthat_tests", (DL_FUNC) &run_testthat_tests, 0},
    {NULL, NULL, 0}
};

RcppExport void R_init_PDFR(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
