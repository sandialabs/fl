#ifndef fl_factory_h
#define fl_factory_h


#include <string>
#include <map>


namespace fl
{
  template<class B>
  class Factory
  {
  public:
	typedef B * productFunction (std::istream & stream);
	typedef std::map<std::string, productFunction *> productMap;

	static B * read (std::istream & stream)
	{
	  std::string name;
	  getline (stream, name);
	  typename productMap::iterator entry = products.find (name);
	  if (entry == products.end ())
	  {
		throw "Unknown class name in stream";
	  }
	  else
	  {
		return (*entry->second) (stream);
	  }
	}

	static productMap products;
  };

  template<class B, class C>
  class Product
  {
  public:
	static B * read (std::istream & stream)
	{
	  return new C (stream);
	}
	static void add ()
	{
	  Factory<B>::products.insert (make_pair (typeid (C).name (), &read));
	}
  };
}


#endif