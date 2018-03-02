// (C) 2018 University of Bristol. See License.txt

#ifndef _QGroup
#define _QGroup

/* This class holds the data needed to represent
	Gal/G_2
   where G_2 is the decomposition group at 2 and Gal=(Z/mZ)^*

   We hold it as a set of generators g[i] and orders d[i]

   The group is created using a Hafner-McCurley style
   generator/relation algorithm, then we apply the SNF.
   The key thing is that we impose the relation 2=1 so
   as to get the quotient group above
*/

#include <iostream>
#include <vector>
using namespace std;

class QGroup
{
   int m;           // the integer m defines the group (Z/mZ)^*/<2>
   vector<int> g;   // g is an array of generators
   vector<int> d;   /* d is an array of group orders, such that
	             *   (Z/mZ)^* = C_d0 \times C_d1 \times ...
                     * and gi generates an order-di group
	             *******************************************/
   int ngen; // the number of generators
   int Gord;
   mutable vector<int> elems;

   public:
 
   void assign(int mm,int seed); // initialize this instance for m=mm

   QGroup()  { ; }
   ~QGroup() { ; }
   
   int num_gen()      const { return ngen; }
   int gen(int i)     const { return g[i]; }
   int gord(int i)    const { return d[i]; }
   int order()        const { return Gord;  }

   friend ostream& operator<<(ostream& s,const QGroup& QGrp);
   friend istream& operator>>(istream& s,QGroup& QGrp);

   // For 0 <= n < ord returns the nth element
   int nth_element(int n) const;
};

#endif
