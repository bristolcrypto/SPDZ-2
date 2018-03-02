// (C) 2018 University of Bristol. See License.txt


#include "Ring.h"
#include "Exceptions/Exceptions.h"

ostream& operator<<(ostream& s,const Ring& R)
{
  s << R.mm << " " << R.phim << endl;
  for (int i=0; i<R.phim; i++)  { s << R.pi[i] << " "; }
  s << endl;
  for (int i=0; i<R.mm; i++)    { s << R.pi_inv[i] << " "; }
  s << endl;
  for (int i=0; i<=R.phim; i++) { s << R.poly[i] << " "; }
  s << endl;
  return s;
}

istream& operator>>(istream& s,Ring& R)
{
  s >> R.mm >> R.phim;
  if (s.fail())
    throw IO_Error("can't read ring");
  R.pi.resize(R.phim); R.pi_inv.resize(R.mm); R.poly.resize(R.phim+1);
  for (int i=0; i<R.phim; i++)  { s >> R.pi[i]; }
  for (int i=0; i<R.mm; i++)    { s >> R.pi_inv[i]; }
  for (int i=0; i<=R.phim; i++) { s >> R.poly[i]; }
  return s;
}

void Ring::pack(octetStream& o) const
{
  o.store(mm);
  o.store(phim);
  o.store(pi);
  o.store(pi_inv);
  o.store(poly);
}

void Ring::unpack(octetStream& o)
{
  o.get(mm);
  o.get(phim);
  o.get(pi);
  o.get(pi_inv);
  o.get(poly);
}

bool Ring::operator !=(const Ring& other) const
{
  if (mm != other.mm or phim != other.phim or pi != other.pi
      or pi_inv != other.pi_inv or poly != other.poly)
    return true;
  else
    return false;
}
