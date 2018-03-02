// (C) 2018 University of Bristol. See License.txt


#include <iostream>
#include <iomanip>

using namespace std;

inline void pprint_bytes(const char *label, unsigned char *bytes, int len)
{
    cout << label << ": ";
    for (int j = 0; j < len; j++)
        cout << setfill('0') << setw(2) << hex << (int) bytes[j];
    cout << dec << endl;
}
