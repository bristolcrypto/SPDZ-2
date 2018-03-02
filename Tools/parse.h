// (C) 2018 University of Bristol. See License.txt

/*
 * parse.h
 *
 */

#ifndef TOOLS_PARSE_H_
#define TOOLS_PARSE_H_

#include <iostream>
#include <vector>
using namespace std;

// Read a byte
inline int get_val(istream& s)
{
  char cc;
  s.get(cc);
  int a=cc;
  if (a<0) { a+=256; }
  return a;
}

// Read a 4-byte integer
inline int get_int(istream& s)
{
  int n = 0;
  for (int i=0; i<4; i++)
    { n<<=8;
      int t=get_val(s);
      n+=t;
    }
  return n;
}

// Read several integers
inline void get_ints(int* res, istream& s, int count)
{
  for (int i = 0; i < count; i++)
    res[i] = get_int(s);
}

inline void get_vector(int m, vector<int>& start, istream& s)
{
  start.resize(m);
  for (int i = 0; i < m; i++)
    start[i] = get_int(s);
}

#endif /* TOOLS_PARSE_H_ */
