/*
Author: Fred Rothganger


Revisions 1.1 and 1.2 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.3  2007/03/23 11:06:58  Fred
Use CVS Log to generate revision history.

Revision 1.2  2005/10/14 16:55:17  Fred
Add Sandia distribution terms.

Revision 1.1  2005/08/28 21:21:58  Fred
Substitute for the non-free xv utility commonly used to view images under X
windows.
-------------------------------------------------------------------------------
*/


#include <fl/slideshow.h>


using namespace fl;
using namespace std;


int
main (int argc, char * argv[])
{
  if (argc < 2)
  {
	cerr << "Usage: " << argv[0] << " {image file names}" << endl;
	return 1;
  }


  new ImageFileFormatPGM;
  new ImageFileFormatJPEG;
  new ImageFileFormatTIFF;


  SlideShow window;

  for (int i = 1; i < argc; i++)
  {
	Image disp (argv[i]);
	window.show (disp);
	window.waitForClick ();
  }

  return 0;
}
