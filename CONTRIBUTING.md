# Contributing

The author is very much an amateur, with little time to spend on this project and no experience whatsoever in C++. If you are interested in contributing, I would be delighted to hear from you: please [get in touch](Allan.Cameron@nhs.net).

# Conventions

## Naming conventions
- All variable names are written in snake_case with no capitals.
- Aim for descriptive names over saving horizontal space.
- All variables passed as parameters to a function are prefixed with t_.
- All private data members are suffixed with a single underscore.
- All function / method names are written in CamelCase.
- Private methods are suffixed with a single underscore.
- Class names are written in CamelCase.

e.g.
```cpp
//---------------------------------------------------------------------------//
// Method to make things OK

void MakeEverythingOK(std::string& t_input)
{
  std::string ok_suffix = " is OK";
  t_input.append(ok_suffix);
}

//---------------------------------------------------------------------------//
// Make the data member OK

void MyClass::MakeAnOKMember_(const std::string& t_input)
{
  ok_data_member_ = MakeEverythingOK(t_input);
}

//---------------------------------------------------------------------------//
```


## Formatting
- Indentation is in [Allman style](https://en.wikipedia.org/wiki/Indentation_style#Allman_style). Yes it wastes vertical space, but I just find it more readable.
- Indentation is with two spaces.
- No tabs allowed.
- The maximum line width is 80 characters. No exceptions.
- All function definitions are seperated by an 80-character comment line as shown in the snippet above, with a brief description commented below, a line break, then the function, followed by a line break then the next comment line.
