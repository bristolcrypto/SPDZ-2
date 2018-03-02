// (C) 2018 University of Bristol. See License.txt

#ifndef _FILE_IO_HEADER
#define _FILE_IO_HEADER

#include "Exceptions/Exceptions.h"
#include "Math/Share.h"

#include <string>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

/* 
 * Provides generalised file read and write methods for arrays of numeric data types.
 * Stateless and not optimised for multiple reads from file.
 * Intended for MPC application specific file IO.
 */

class Binary_File_IO
{
  public:

  /*
   * Append the buffer values as binary to the filename.
   * Throws file_error.   
   */
  template <class T>
  void write_to_file(const string filename, const vector< Share<T> >& buffer);

  /*
   * Read from posn in the filename the binary values until the buffer is full.
   * Assumes file holds binary that maps into the type passed in.
   * Returns the current posn in the file or -1 if at eof.
   * Throws file_error.
   */
  template <class T>
  void read_from_file(const string filename, vector< Share<T> >& buffer, const int start_posn, int &end_posn);
};

#endif
