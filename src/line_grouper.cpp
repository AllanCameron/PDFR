//---------------------------------------------------------------------------//
//                                                                           //
//  PDFR LineGrouper implementation file                                     //
//                                                                           //
//  Copyright (C) 2018 - 2019 by Allan Cameron                               //
//                                                                           //
//  Licensed under the MIT license - see https://mit-license.org             //
//  or the LICENSE file in the project root directory                        //
//                                                                           //
//---------------------------------------------------------------------------//

#include "line_grouper.h"

//---------------------------------------------------------------------------//

using namespace std;

//---------------------------------------------------------------------------//
//

LineGrouper::LineGrouper(vector<TextBox> t_text_boxes)
  : text_boxes_(t_text_boxes)
{
  size_t i = 0;
  if(text_boxes_.empty()) throw runtime_error("No textboxes on page");

  while (i < text_boxes_.size())
  {
    if (text_boxes_[i].size() < 2){++i; continue;}
    sort(text_boxes_[i].begin(), text_boxes_[i].end(), ReadingOrder());
    FindBreaks(text_boxes_[i]);
    if (text_boxes_[i].size() < 2){++i; continue;}
    LineEndings(text_boxes_[i]);
    PasteLines(text_boxes_[i++]);
  }
};

//---------------------------------------------------------------------------//
//

void LineGrouper::FindBreaks(TextBox& t_text_box)
{
  for(size_t i = 1; i < t_text_box.size(); ++i)
  {
    if(t_text_box[i]->GetLeft() - t_text_box[i - 1]->GetLeft() > 0.1
         &&
       t_text_box[i]->GetBottom() < t_text_box[i - 1]->GetBottom())
    {
      SplitBox(t_text_box, t_text_box[i - 1]->GetBottom());
      break;
    }

  }
}

//---------------------------------------------------------------------------//
//

void LineGrouper::LineEndings(TextBox& t_text_box)
{
  for(size_t i = 0; i < t_text_box.size() - 1; ++i)
  {
    auto& element = t_text_box[i];
    switch(element->GetGlyph().back())
    {
      case 0x0020:                          break;
      case 0x00A0:                          break;
      case 0x002d: element->PopLastGlyph(); break;
      case 0x2010: element->PopLastGlyph(); break;
      case 0x2011: element->PopLastGlyph(); break;
      case 0x2012: element->PopLastGlyph(); break;
      case 0x2013: element->PopLastGlyph(); break;
      case 0x2014: element->PopLastGlyph(); break;
      case 0x2015: element->PopLastGlyph(); break;
      default:     element->AddSpace();
    }
  }
}

//---------------------------------------------------------------------------//
//

void LineGrouper::PasteLines(TextBox& t_text_box)
{
  for(size_t i = 1; i < t_text_box.size(); ++i)
  {
    t_text_box[0]->ConcatenateUnicode(t_text_box[i]->GetGlyph());
  }
  t_text_box.resize(1);
}

//---------------------------------------------------------------------------//
//

void LineGrouper::SplitBox(TextBox& t_old_one, float t_top_edge)
{
  if(t_old_one.empty()) return;
  TextBox new_one = t_old_one;
  new_one.clear();
  size_t breakpoint = 0;

  for(size_t i = 0; i < t_old_one.size(); ++i)
  {
    if(t_old_one[i]->GetBottom() < t_top_edge)
    {
      if(breakpoint != 0) breakpoint = i;
      new_one.push_back(t_old_one[i]);
    }
  }
  if(breakpoint > 0) t_old_one.resize(breakpoint - 1);

  t_old_one.SetBottom(t_old_one.back()->GetBottom());
  new_one.SetTop(t_old_one.front()->GetTop());
  text_boxes_.push_back(new_one);
}


//---------------------------------------------------------------------------//

vector<TextBox>& LineGrouper::Output()
{
  return text_boxes_;
}

