#include <stdlib.h>
#include <iostream>

using namespace std;


int
main (int argc, char * argv[])
{
  for (int i = 1; i < argc; i++)
  {
	string base = argv[i];
	int j = base.find_last_of ('.');
	base = base.substr (0, j);

	char line[1024];
	sprintf (line, "jpeg2ps %s > %s.eps", argv[i], base.c_str ());
	cerr << line << endl;
	system (line);
  }
}