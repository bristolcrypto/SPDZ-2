// (C) 2018 University of Bristol. See License.txt


#ifndef _myHNF
#define _myHNF

#include <vector>
using namespace std;

#include "Math/bigint.h"
#include "Math/modp.h"

typedef vector< vector<bigint> > matrix;
typedef vector< vector<modp> > modp_matrix;

class imatrix : public vector< vector<int> >
{
public:
    bool operator!=(const imatrix& other) const;

    void pack(octetStream& o) const;
    void unpack(octetStream& o);
};

/* Uses Algorithm 2.7 from Pohst-Zassenhaus to compute H and U st
		H = HNF(A) = A*U
*/
void HNF(matrix& H,matrix& U,const matrix& A);

/* S = U*A*V
   We dont care about U though
*/
void SNF(matrix& S,const matrix& A,matrix& V);

void print(const matrix& S);
void print(const imatrix& S);

// Special inverse routine, assumes A is unimodular and only
// requires column operations to create the inverse
matrix inv(const matrix& A);

// Another special routine for modp matrices. 
// Solves
// 	Ax=b
// Assumes A is unimodular, square and only requires row operations to
// create the inverse. In put is C=(A||b) and the routines alters A
vector<modp> solve(modp_matrix& C,const Zp_Data& PrD);

// Finds a pseudo-inverse of a matrix A modulo 2
//   - Input matrix is assumed to have more rows than columns
void pinv(imatrix& Ai,const imatrix& A);

ostream& operator<<(ostream& s,const imatrix& A);
istream& operator>>(istream& s,imatrix& A);

#endif
