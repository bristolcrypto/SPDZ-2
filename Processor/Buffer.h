// (C) 2018 University of Bristol. See License.txt

/*
 * Buffer.h
 *
 */

#ifndef PROCESSOR_BUFFER_H_
#define PROCESSOR_BUFFER_H_

#include <fstream>
using namespace std;

#include "Math/Share.h"
#include "Math/field_types.h"
#include "Tools/time-func.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 101
#endif


class BufferBase
{
protected:
    static bool rewind;

    ifstream* file;
    int next;
    const char* data_type;
    const char* field_type;
    Timer timer;
    int tuple_length;
    string filename;

public:
    bool eof;

    BufferBase() : file(0), next(BUFFER_SIZE), data_type(0), field_type(0),
            tuple_length(-1), eof(false) {}
    void setup(ifstream* f, int length, string filename, const char* type = 0,
            const char* field = 0);
    void seekg(int pos);
    bool is_up() { return file != 0; }
    void try_rewind();
    void prune();
    void purge();
};


template <class T, class U>
class Buffer : public BufferBase
{
    T buffer[BUFFER_SIZE];

    void read(char* read_buffer);

public:
    ~Buffer();
    void input(U& a);
    void fill_buffer();
};


template < template<class T> class U, template<class T> class V >
class BufferHelper
{
public:
    Buffer< U<gfp>, V<gfp> > bufferp;
    Buffer< U<gf2n>, V<gf2n> > buffer2;
    ifstream* files[N_DATA_FIELD_TYPE];

    BufferHelper() { memset(files, 0, sizeof(files)); }
    void input(V<gfp>& a) { bufferp.input(a); }
    void input(V<gf2n>& a) { buffer2.input(a); }
    BufferBase& get_buffer(DataFieldType field_type);
    void setup(DataFieldType field_type, string filename, int tuple_length, const char* data_type = 0);
    void close();
    void prune();
    void purge();
};

#endif /* PROCESSOR_BUFFER_H_ */
