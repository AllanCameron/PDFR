//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR Streams implementation file                                         //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_STREAMS

//---------------------------------------------------------------------------//

#define PDFR_STREAMS

/* Streams in pdf files are usually made up of a sequence of non-ascii bytes
 * intended to represent raw data. When they occur they are always part of a pdf
 * object, which will always start with a <<dictionary>>. At the end of the
 * dictionary, after the closing brackets, comes the keyword 'stream', usually
 * (?always) followed by two whitespace bytes: \r and \n. The data then begins.
 * The end of the stream is declared by the sequence (\r\nendstream).
 *
 * The data can represent many different things including pictures, fonts,
 * annotations and postscript-type page descriptions. For the purposes of text
 * extraction, it is mainly the latter we are interested in.
 *
 * The raw data in the stream is almost always compressed, so needs to be
 * decompressed before being processed. That is the purpose of the stream class.
 *
 * At present, only the flatedecode decompression algorithm is implemented. That
 * is simply because it is the most common algorithm used in pdfs. Ultimately,
 * we will need to turn "streams" into its own class, which implements stream
 * decompression for a variety of algorithms on request. I have yet to find a
 * pdf file that uses anything else for page description to allow testing
 *
 * This header is required by the xref class, as it needs to be able to deflate
 * xrefstreams.
 */
#include<string>

//---------------------------------------------------------------------------//
// Inflates the given string

void FlateDecode(std::string& deflated_string);

//---------------------------------------------------------------------------//

#endif

