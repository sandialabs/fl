#ifndef fl_string_h
#define fl_string_h


#include <ctype.h>
#include <string>


namespace fl
{
  inline void
  split (const std::string & source, char * delimiter, std::string & first, std::string & second)
  {
	int index = source.find (delimiter);
	if (index == std::string::npos)
	{
	  first = source;
	  second.erase ();
	}
	else
	{
	  std::string temp = source;
	  first = temp.substr (0, index);
	  second = temp.substr (index + strlen (delimiter));
	}
  }

  inline void
  trim (std::string & target)
  {
	int begin = target.find_first_not_of (' ');
	int end = target.find_last_not_of (' ');
	if (begin == std::string::npos)
	{
	  // all spaces
	  target.erase ();
	  return;
	}
	target = target.substr (begin, end - begin + 1);
  }

  inline void
  lowercase (std::string & target)
  {
	std::string::iterator i;
	for (i = target.begin (); i < target.end (); i++)
	{
	  *i = tolower (*i);
	}
  }
}


#endif
