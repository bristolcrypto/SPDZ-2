// (C) 2018 University of Bristol. See License.txt

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
protected:
  long a;

  public:

  static string type_string() { return "integer"; }

  Integer()                 { a = 0; }
  Integer(long a) : a(a)    {}

  long get() const          { return a; }

  void assign_zero()        { a = 0; }

  long operator+(const Integer& other) const { return a + other.a; }
  long operator-(const Integer& other) const { return a - other.a; }
  long operator*(const Integer& other) const { return a * other.a; }
  long operator/(const Integer& other) const { return a / other.a; }

  long operator>>(const Integer& other) const { return a >> other.a; }
  long operator<<(const Integer& other) const { return a << other.a; }

  long operator^(const Integer& other) const { return a ^ other.a; }
  long operator&(const Integer& other) const { return a ^ other.a; }
  long operator|(const Integer& other) const { return a ^ other.a; }

  bool operator==(const Integer& other) const { return a == other.a; }
  bool operator!=(const Integer& other) const { return a != other.a; }
  bool operator<(const Integer& other) const { return a < other.a; }
  bool operator<=(const Integer& other) const { return a <= other.a; }
  bool operator>(const Integer& other) const { return a > other.a; }
  bool operator>=(const Integer& other) const { return a >= other.a; }

  long operator^=(const Integer& other) { return a ^= other.a; }

  friend unsigned int& operator+=(unsigned int& x, const Integer& other) { return x += other.a; }

  friend ostream& operator<<(ostream& s, const Integer& x) { x.output(s, true); return s; }

  void output(ostream& s,bool human) const;
  void input(istream& s,bool human);

};

#endif /* INTEGER_H_ */
