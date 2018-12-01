/* Most strings we want to show will be Ascii-based, but for some higher-byte
 * characters, the actual glyphs intended by the author have to encoded
 * somehow. There are different ways to encode these glyphs as numbers.
 * We therefore need to know the encoding used if we want to recover the correct
 * glyphs from the string. We do this by reading the encoding entry of the
 * fonts dictionary. This allows us to convert directly to a pdf-standard
 * name ("/glyphname") for each character. This can be converted as needed
 * for output on the system running the software.
 *
 * We need to start with a base encoding, if specified in the font dictionary.
 * Sometimes, none is specified in which case we use standard encoding.
 * Sometimes, some or all glyph names and their byte values are given; these
 * supercede the base encoding). Sometimes the encoding is given as /Identity-H
 * which means the encoding is specified in a CMAP.
 *
 * Since this library aims to extract usable text rather than beautiful layout,
 * some glyphs need to be converted to pairs of lower-byte glyphs to make
 * text extraction more useful, particularly the ligatures.
 */

#ifndef PDFR_ENCODE
#define PDFR_ENCODE

#include "pdfr.h"
#include "document.h"
#include "stringfunctions.h"
#include "streams.h"
#include "encodings.h"

EncMap getBaseEncode(const std::string& encoding);
std::string parseUnicode(std::string s, std::map<std::string, std::string>& UM);
std::string defaultUnicode(document& d, std::string s);
std::vector<std::string> baseEncoding(const std::string& enc);

#endif
