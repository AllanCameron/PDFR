//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR GraphicsState header file                                           //
//                                                                           //
//  Copyright (C) 2018 - 2021 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#ifndef PDFR_GS

//---------------------------------------------------------------------------//

#define PDFR_GS

#include "matrix.h"
#include "page.h"
#include "graphicobject.h"

/*---------------------------------------------------------------------------*/

class TextState
{
  public:
    float                 tc,     // Character spacing
                          tw,     // Word spacing
                          th,     // Horizontal scaling
                          tl,     // Text leading
                          tfs,    // Font size
                          trise;  // Text rise
    std::string           tf;     // Font name
    int                   tmode;  // Text printing mode
    std::shared_ptr<Font> current_font;

    TextState() : tc(0), tw(0), th(100), tl(0),
                  tfs(0), trise(0), tf(""), tmode(0) {}
};

//---------------------------------------------------------------------------//

class GraphicsState
{
  public:
    Matrix                   CTM;
    Path                     clipping_path;
    std::vector<std::string> colour_space_stroke,
                             colour_space_fill;
    std::vector<float>       colour,
                             fill;
    TextState                text_state;
    Matrix                   tm_state,
                             td_state;
    float                    line_width;
    int                      line_cap,
                             line_join;
    float                    miter_limit;
    std::string              rendering_intent;
    bool                     stroke_adjustment;
    std::vector<int>         dash_array;
    std::vector<std::string> blending_mode;
    std::string              soft_mask;
    float                    alpha_constant;
    bool                     alpha_source;

    GraphicsState(std::shared_ptr<Page> p) :
                      CTM(Matrix()), clipping_path(Path()),
                      colour_space_stroke({"/DeviceGray"}),
                      colour_space_fill({"/DeviceGray"}),
                      colour({0, 0, 0}), fill({0, 0, 0}),
                      text_state(TextState()), tm_state(Matrix()),
                      td_state(Matrix()), line_width(1),
                      line_cap(0), line_join(0), miter_limit(10.0),
                      rendering_intent("/RelativeColorimetric"),
                      stroke_adjustment(false),
                      dash_array({0}),
                      blending_mode({"Normal"}), soft_mask("None"),
                      alpha_constant(1.0), alpha_source(false)
    {
      std::shared_ptr<Box> b = p->GetMinbox();
      clipping_path.SetX({b->GetLeft(),   b->GetLeft(), b->GetRight(),
                          b->GetRight(),  b->GetLeft()});
      clipping_path.SetY({b->GetBottom(), b->GetTop(), b->GetTop(),
                          b->GetBottom(), b->GetBottom()});
    }

};

//---------------------------------------------------------------------------//

#endif


