// (C) 2018 University of Bristol. See License.txt

/*
 * Checking.h
 *
 */

#ifndef FHEOFFLINE_CHECKING_H_
#define FHEOFFLINE_CHECKING_H_

#include "Networking/Player.h"
#include "Auth/MAC_Check.h"
#include "Math/Setup.h"

template <class T>
class TripleSacriFactory
{
public:
    virtual ~TripleSacriFactory() {}
    virtual void get(T& a, T& b, T& c) = 0;
};

template <class T>
class TupleSacriFactory
{
public:
    virtual ~TupleSacriFactory() {}
    virtual void get(T& a, T& b) = 0;
};

template <class T>
class SingleSacriFactory
{
public:
    virtual ~SingleSacriFactory() {}
    virtual void get(T& a) = 0;
};

template <class T>
class FileSacriFactory : public TripleSacriFactory<T>,
        public TupleSacriFactory<T>, public SingleSacriFactory<T>
{
    ifstream inpf;

public:
    FileSacriFactory(const char* type, const Player& P, int output_thread);
    void get(T& a, T& b, T& c);
    void get(T& a, T& b);
    void get(T& a);
};

void Triple_Inverse_Checking(const Player& P, MAC_Check<gfp>& MC, int nm,
    int nr, int output_thread = 0);
template <class T>
void Triple_Checking(const Player& P, MAC_Check<T>& MC, int nm,
        int output_thread, TripleSacriFactory<Share<T> >& factory,
        bool write_output = true, bool clear = true, string dir = PREP_DIR);
template <class T>
void Inverse_Checking(const Player& P, MAC_Check<T>& MC, int nr,
        int output_thread, TripleSacriFactory<Share<T> >& triple_factory,
        TupleSacriFactory<Share<T> >& inverse_factor,
        bool write_output = true, bool clear = true, string dir = PREP_DIR);
void Triple_Checking(const Player& P,MAC_Check<gf2n_short>& MC,int nm);
void Square_Bit_Checking(const Player& P,MAC_Check<gfp>& MC,int ns,int nb);
template <class T>
void Square_Checking(const Player& P, MAC_Check<T>& MC, int ns,
        int output_thread, TupleSacriFactory<Share<T> >& factory,
        bool write_output = true, bool clear = true, string dir = PREP_DIR);
void Bit_Checking(const Player& P, MAC_Check<gfp>& MC, int nb,
        int output_thread, TupleSacriFactory<Share<gfp> >& square_factory,
        SingleSacriFactory<Share<gfp> >& bit_factory, bool write_output = true,
        bool clear = true, string dir = PREP_DIR);
void Square_Checking(const Player& P,MAC_Check<gf2n_short>& MC,int ns);

template <class T>
inline string file_completion(const T& dummy = {})
{
  (void)dummy;
  return { T::type_char() };
}

#endif /* FHEOFFLINE_CHECKING_H_ */
