// (C) 2016 University of Bristol. See License.txt

/*
 * Integer.h
 *
 */

#ifndef INTEGER_H_
#define INTEGER_H_

#include <iostream>
using namespace std;

// Wrapper class for integer, used for Memory

class Integer
{
  long a;

  public:

  Integer()                 { a = 0; }
  Integer(long a) : a(a)    {}

  long get() const          { return a; }

  void assign_zero()        { a = 0; }

  void output(ostream& s,bool human) const;
  void input(istream& s,bool human);

};

#endif /* INTEGER_H_ */
