#include "fl/video.h"


using namespace std;
using namespace fl;


// class VideoIn ----------------------------------------------------------------

VideoIn::VideoIn (const string & fileName, const PixelFormat & hint)
{
  file = 0;
  VideoFileFormat * format = VideoFileFormat::find (fileName);
  if (format)
  {
	file = format->openInput (fileName, hint);
  }
}

VideoIn::~VideoIn ()
{
  if (file)
  {
	delete file;
  }
}

void
VideoIn::seekFrame (int frame)
{
  if (file)
  {
	file->seekFrame (frame);
  }
}

void
VideoIn::seekTime (double timestamp)
{
  if (file)
  {
	file->seekTime (timestamp);
  }
}

VideoIn &
VideoIn::operator >> (Image & image)
{
  if (file)
  {
	file->readNext (image);
  }
  return *this;
}

bool
VideoIn::good () const
{
  return file  &&  file->good ();
}

void
VideoIn::setTimestampMode (bool frames)
{
  if (file)
  {
	file->setTimestampMode (frames);
  }
}


// class VideoOut -------------------------------------------------------------

VideoOut::VideoOut (const std::string & fileName, const std::string & formatName, const std::string & codecName)
{
  file = 0;
  VideoFileFormat * format = VideoFileFormat::find (formatName, codecName);
  if (format)
  {
	file = format->openOutput (fileName, formatName, codecName);
  }
}

VideoOut::~VideoOut ()
{
  if (file)
  {
	delete file;
  }
}

VideoOut &
VideoOut::operator << (const Image & image)
{
  if (file)
  {
	file->writeNext (image);
  }
  return *this;
}

bool
VideoOut::good () const
{
  return file  &&  file->good ();
}

void
VideoOut::set (const string & name, double value)
{
  if (file)
  {
	file->set (name, value);
  }
}


// class VideoInFile ----------------------------------------------------------

VideoInFile::~VideoInFile ()
{
}


// class VideoOutFile ---------------------------------------------------------

VideoOutFile::~VideoOutFile ()
{
}


// class VideoFileFormat ------------------------------------------------------

vector<VideoFileFormat *> VideoFileFormat::formats;

VideoFileFormat::VideoFileFormat ()
{
  formats.push_back (this);
}

VideoFileFormat::~VideoFileFormat ()
{
  vector<VideoFileFormat *>::iterator i;
  for (i = formats.begin (); i < formats.end (); i++)
  {
	if (*i == this)
	{
	  formats.erase (i);
	  break;
	}
  }
}

VideoFileFormat *
VideoFileFormat::find (const std::string & fileName)
{
  VideoFileFormat * result = NULL;
  float bestProbability = 0;
  vector<VideoFileFormat *>::reverse_iterator i;
  for (i = formats.rbegin (); i != formats.rend (); i++)
  {
	float p = (*i)->isIn (fileName);
	if (p > bestProbability)
	{
	  result = *i;
	  bestProbability = p;
	}
  }

  return result;
}

VideoFileFormat *
VideoFileFormat::find (const std::string & formatName, const std::string & codecName)
{
  VideoFileFormat * result = NULL;
  float bestProbability = 0;
  vector<VideoFileFormat *>::reverse_iterator i;
  for (i = formats.rbegin (); i != formats.rend (); i++)
  {
	float p = (*i)->handles (formatName, codecName);
	if (p > bestProbability)
	{
	  result = *i;
	  bestProbability = p;
	}
  }

  return result;
}
