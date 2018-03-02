// (C) 2018 University of Bristol. See License.txt

#include "Processor/Binary_File_IO.h"
#include "Math/gfp.h"

/* 
 * Provides generalised file read and write methods for arrays of shares.
 * Stateless and not optimised for multiple reads from file.
 * Intended for application specific file IO.
 */

template<class T> 
void Binary_File_IO::write_to_file(const string filename, const vector< Share<T> >& buffer)
{
  ofstream outf;

  outf.open(filename, ios::out | ios::binary | ios::app);
  if (outf.fail()) { throw file_error(filename); }

  for (unsigned int i = 0; i < buffer.size(); i++)
  {
    buffer[i].output(outf, false);
  }

  outf.close();
}

template<class T>
void Binary_File_IO::read_from_file(const string filename, vector< Share<T> >& buffer, const int start_posn, int &end_posn)
{
  ifstream inf;
  inf.open(filename, ios::in | ios::binary);
  if (inf.fail()) { throw file_missing(filename, "Binary_File_IO.read_from_file expects this file to exist."); }

  int size_in_bytes = Share<T>::size() * buffer.size();
  int n_read = 0;
  char * read_buffer = new char[size_in_bytes];
  inf.seekg(start_posn);
  do
  {
      inf.read(read_buffer + n_read, size_in_bytes - n_read);
      n_read += inf.gcount();

      if (inf.eof())
      {
        stringstream ss;
        ss << "Got to EOF when reading from disk (expecting " << size_in_bytes << " bytes).";
        throw file_error(ss.str());
      }
      if (inf.fail())
      {
        stringstream ss;
        ss << "IO problem when reading from disk";
        throw file_error(ss.str());
      }
  }
  while (n_read < size_in_bytes);

  end_posn = inf.tellg();

  //Check if at end of file by getting 1 more char.
  inf.get();
  if (inf.eof())
    end_posn = -1;
  inf.close();

  for (unsigned int i = 0; i < buffer.size(); i++)
    buffer[i].assign(&read_buffer[i*Share<T>::size()]);
}

template void Binary_File_IO::write_to_file(const string filename, const vector< Share<gfp> >& buffer);
template void Binary_File_IO::read_from_file(const string filename, vector< Share<gfp> >& buffer, const int start_posn, int &end_posn);
