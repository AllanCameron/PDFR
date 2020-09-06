# Contributing

I am an amateur, with little time to spend on this project and no experience whatsoever in C++. If you are interested in contributing, I would be delighted to hear from you: please [get in touch](Allan.Cameron@nhs.net).

To keep the C++ codebase consistent, I will declare some coding conventions here. This is mostly for my own benefit, but I would ask any contributors to keep to these conventions too where possible.

# Conventions

## Naming conventions
- All variable names are written in `snake_case` with no capitals.
- Aim for descriptive names over saving horizontal space e.g `temporary_byte_vector` is better than `tmpvec`
- Prefer named iterators in a loop rather than `i`; e.g. `for (size_t entry = 0; entry < table.size(); ++entry)` unless there is no meaningful name to apply.
- All private data members are suffixed with a single underscore: `private_member_`
- All function / method names are written in `CamelCase`.
- Class, struct, enum and type names are written in `CamelCase`.
- Suffix private methods with an underscore - `MyPrivateMethod_();`
- Use descriptive variable names in class method declarations, as these will help document the class user interface. Private methods don't always need a variable name, or can be short descriptive names if preferred.

The following code block demonstrates most of these naming conventions:

```cpp
//---------------------------------------------------------------------------//
// Method to make things OK

std::string MakeEverythingOK(std::string input_string)
{
  std::string ok_suffix = " is OK";
  input_string.append(ok_suffix);
  return input_string;
}

//---------------------------------------------------------------------------//
// Make the data member OK

void MyClass::MakeAnOKMember_(const std::string& input_string)
{
  ok_data_member_ = MakeEverythingOK(input_string);
}

//---------------------------------------------------------------------------//
```


## Comments
- Every file begins with the MIT license header
- Most comments should have the single-line `//` format rather than `/* Multi-line */` type. An exception can be made for large introductory comments explaining the rationale for a class at the top of a header file, just below the license.
- Prefer verbose comments, even though the naming rules should make the code largely self-commenting. It takes less time to understand what's going on if things are well commented. Of course, we want to avoid being silly with the likes of
```cpp
return result; // Returns the result
```
but the general rule is, if it is quicker to understand it with a comment, the comment goes in.

## Layout
- Indentation is in [Allman style](https://en.wikipedia.org/wiki/Indentation_style#Allman_style). Yes it wastes vertical space, but I just find it more readable.
- Indentation is with two spaces. No tabs allowed.
- The maximum line width is 80 characters. No exceptions.
- All function definitions are seperated by an 80-character comment line as shown in the snippet above, with a brief description commented below, a line break, then the function, followed by a line break then the next comment line.
- Class definitions are declared public members first, then private members.
- The keywords `public:` and `private:` in a class definition get a single space indentation.
