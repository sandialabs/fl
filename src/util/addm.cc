/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include <stdlib.h>
#include <stdio.h>


void
addm ()
{
  int character;
  while ((character = getchar ()) != EOF)
  {
	if (character == 10)
	{
	  putchar (13);
	}
	putchar (character);
  }
}

int
main (int argc, char * argv[])
{
  if (argc == 1)
  {
	addm ();
  }
  else
  {
	for (int i = 1; i < argc; i++)
	{
	  char outname[1024];
	  sprintf (outname, "%s_temp", argv[i]);

	  freopen (argv[i], "r", stdin);
	  freopen (outname, "w", stdout);

	  addm ();

	  fclose (stdin);
	  fclose (stdout);

	  char command[1024];
	  sprintf (command, "mv %s %s", outname, argv[i]);
	  system (command);
	}
  }

  return 0;
}
