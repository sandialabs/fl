#define BLOCKSIZE 4096
#define BLOCKCOUNT 65536


#include </home/rothgang/i686/include/fl/time.h>

#include <iostream>

using namespace std;
using namespace fl;


int main (int argc, char * argv[])
{
  char block[BLOCKSIZE];
  FILE * file;

  double starttime = getTimestamp ();

  if (argc < 2)
  {
	cerr << "Starting write test" << endl;
	double writetime = getTimestamp ();
	file = fopen ("timing.tmp", "w");
	for (int i = 0; i < BLOCKCOUNT; i++)
	{
	  fwrite (block, BLOCKSIZE, 1, file);
	}
	fsync (fileno (file));
	fclose (file);
	cerr << "Done writing: " << getTimestamp () - writetime << endl;
  }

  cerr << "Starting read test" << endl;
  double readtime = getTimestamp ();
  file = fopen ("timing.tmp", "r");
  for (int i = 0; i < BLOCKCOUNT; i++)
  {
	fread (block, BLOCKSIZE, 1, file);
  }
  fsync (fileno (file));
  fclose (file);
  cerr << "Done reading: " << getTimestamp () - readtime << endl;

  cerr << "Total: " << getTimestamp () - starttime << endl;

  return 0;
}
