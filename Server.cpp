// (C) 2016 University of Bristol. See License.txt


#include "Networking/sockets.h"
#include "Networking/ServerSocket.h"

#include <iostream>
#include <vector>
#include <pthread.h>
using namespace std;

vector<int> socket_num;


vector<octet*> names;

int nmachines;



void get_name(int num)
{
  // Now all machines are set up, send GO to start them.
  send(socket_num[num], GO);
  cerr << "Player " << num << " started." << endl;

  // Receive Name
  names[num]=new octet[512];
  receive(socket_num[num],names[num],512);
  cerr << "Player " << num << " is on machine " << names[num] << endl;
}


void send_names(int num)
{
  /* Now send the machine names back to each client 
   * and the number of machines
   */
  send(socket_num[num],nmachines);
  for (int i=0; i<nmachines; i++)
    { send(socket_num[num],names[i],512); }
}





/* Takes command line arguments of 
       - Number of machines connecting
       - Base PORTNUM address
*/

int main(int argc,char **argv)
{
  if (argc != 3)
    { cerr << "Call using\n\t";
      cerr << "Server.x n PortnumBase\n";
      cerr << "\t\t n           = Number of machines" << endl;
      cerr << "\t\t PortnumBase = Base Portnum\n";
      exit(1);
    }
  nmachines=atoi(argv[1]);
  int PortnumBase=atoi(argv[2]);
  int i;

  names.resize(nmachines);

  /* Set up the sockets */
  socket_num.resize(nmachines);
  for (i=0; i<nmachines; i++) { socket_num[i]=-1; }

  // port number one lower to avoid conflict with players
  ServerSocket server(PortnumBase - 1);

  // set up connections
  for (i=0; i<nmachines; i++)
    {
      cerr << "Waiting for player " << i << endl;
      socket_num[i] = server.get_connection_socket(i);
      cerr << "Connected to player " << i << endl;
    }

  // get names
  for (i=0; i<nmachines; i++)
    get_name(i);

  // send names
  for (i=0; i<nmachines; i++)
    send_names(i);

  for (i=0; i<nmachines; i++) 
    { delete[] names[i]; }

  return 0;
}

