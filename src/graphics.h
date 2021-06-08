#include<utility>
#include<string>
#include<vector>
#include<memory>

/* This is a header-only implementation of a Graphics class, which is used to
 * store information about shapes extracted from the page description program.
 * Rather than having different subclasses of graphical object, we simply store
 * the type of shape as an enum. This allows shapes to be stored in a standard
 * way with standard data members and methods. If it turns out that there are
 * specific actions which can only apply to particular shapes then we may need
 * to switch this to a hierarchical class structure.
 *
 */


enum GraphicType {RECTANGLE = 0, SEGMENT = 1, POLYGON = 2, LINES = 3, CIRCLE = 4};

class Graphic
{
public:
  Graphic(GraphicType gt) : x_({0}), y_({0}), size_(1),
  colour_("black"), is_closed_(false), is_visible_(false),
  is_filled_(false), fill_colour_("gray"),  type_(gt) {};

  void SetX(std::vector<float> values) {this->x_ = values;}
  void SetY(std::vector<float> values) {this->y_ = values;}
  void SetType(GraphicType value) {this->type_ = value;}
  void SetSize(float size) {this->size_ = size;}
  void SetColour(std::string colour) {this->colour_ = colour;}
  void SetVisibility(bool visible) {this->is_visible_ = visible;}
  void SetClosed(bool is_closed) {this->is_closed_ = is_closed;}

  std::vector<float> GetX() {return this->x_;}
  std::vector<float> GetY() {return this->y_;}
  float GetSize() {return this->size_;}
  std::string GetColour() {return this->colour_;}
  bool IsClosed() {return this->is_closed_;}
  bool IsVisible() {return this->is_visible_;}
  bool IsFilled() {return this->is_filled_;}
  std::string GetFillColour() {return this->fill_colour_;}
  GraphicType GetType() {return this->type_;}

  float Bottom() {return *std::min_element(this->y_.begin(), this->y_.end());}
  float Top()    {return *std::max_element(this->y_.begin(), this->y_.end());}
  float Left()   {return *std::min_element(this->x_.begin(), this->x_.end());}
  float Right()  {return *std::max_element(this->x_.begin(), this->x_.end());}

  float Width()  {return this->Right() - this->Left();}
  float Height() {return this->Top() - this->Bottom();}

private:
  std::vector<float> x_;
  std::vector<float> y_;
  float size_;
  std::string colour_;
  bool is_closed_;
  bool is_visible_;
  bool is_filled_;
  std::string fill_colour_;
  GraphicType type_;

};
