/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifdef __CYGWIN__
#include <crypt.h>
#else
#define _XOPEN_SOURCE
#include <unistd.h>
#endif

#include <time.h>
#include <stdio.h>


int
main (int argc, char * argv[])
{
  const char codes[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  int seed = time (NULL) % 4096;
  char salt[3];
  salt[0] = codes[seed & 0x3F];
  salt[1] = codes[seed >> 6];
  salt[2] = 0;

  printf ("%s\n", crypt (argv[1], salt));

  return 0;
}
