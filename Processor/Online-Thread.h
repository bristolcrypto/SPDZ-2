// (C) 2018 University of Bristol. See License.txt

#ifndef _Online_Thread
#define _Online_Thread

#include "Networking/Player.h"
#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/Integer.h"
#include "Processor/Data_Files.h"

#include <vector>
using namespace std;

class Machine;

class thread_info
{
  public: 

  int thread_num;
  int covert;
  Names*  Nms;
  gf2n *alpha2i;
  gfp  *alphapi;
  int prognum;
  bool finished;
  bool ready;

  // rownums for triples, bits, squares, and inverses etc
  DataPositions pos;
  // Integer arg (optional)
  int arg;

  Machine* machine;
};

void* Main_Func(void *ptr);

void purge_preprocessing(Names& N, string prep_dir);

#endif

