// (C) 2018 University of Bristol. See License.txt

/*
 * Integer.cpp
 *
 */

#include "Integer.h"

void Integer::output(ostream& s,bool human) const
{
  if (human)
    s << a;
  else
    s.write((char*)&a, sizeof(a));
}

void Integer::input(istream& s,bool human)
{
  if (human)
    s >> a;
  else
    s.read((char*)&a, sizeof(a));
}
