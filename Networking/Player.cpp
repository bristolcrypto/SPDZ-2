// (C) 2016 University of Bristol. See License.txt


#include "Player.h"
#include "Exceptions/Exceptions.h"

#include <sys/select.h>

// Use printf rather than cout so valgrind can detect thread issues

void Names::init(int player,int pnb,const char* servername)
{  
  player_no=player;
  portnum_base=pnb;
  setup_names(servername);
  setup_server();
}


void Names::init(int player,int pnb,vector<octet*> Nms)
{
  player_no=player;
  portnum_base=pnb;
  nplayers=Nms.size();
  names.resize(nplayers);
  for (int i=0; i<nplayers; i++)
    { names[i]=(char*)Nms[i]; }
  setup_server();
}


void Names::init(int player,int pnb,vector<string> Nms)
{
  player_no=player;
  portnum_base=pnb;
  nplayers=Nms.size();
  names=Nms;
  setup_server();
}

// initialize hostnames from file
void Names::init(int player, int _nplayers, int pnb, const string& filename)
{
  ifstream hostsfile(filename.c_str());
  if (hostsfile.fail())
  {
     stringstream ss;
     ss << "Error opening " << filename << ". See HOSTS.example for an example.";
     throw file_error(ss.str().c_str());
  }
  player_no = player;
  nplayers = _nplayers;
  portnum_base = pnb;
  string line;
  while (getline(hostsfile, line))
  {
    if (line.length() > 0 && line.at(0) != '#')
      names.push_back(line);
  }
  if ((int)names.size() < nplayers)
    throw invalid_params();
  names.resize(nplayers);
  for (unsigned int i = 0; i < names.size(); i++)
    cerr << "name: " << names[i] << endl;
  setup_server();
}

void Names::setup_names(const char *servername)
{
  int socket_num;
  int pn = portnum_base - 1;
  set_up_client_socket(socket_num, servername, pn);
  send(socket_num, (octet*)&player_no, sizeof(player_no));
  cerr << "Sent " << player_no << " to " << servername << ":" << pn << endl;

  int inst=-1; // wait until instruction to start.
  while (inst != GO) { receive(socket_num, inst); }

  // Send my name
  octet my_name[512];
  memset(my_name,0,512*sizeof(octet));
  gethostname((char*)my_name,512);
  fprintf(stderr, "My Name = %s\n",my_name);
  send(socket_num,my_name,512);
  cerr << "My number = " << player_no << endl;
  
  // Now get the set of names
  int i;
  receive(socket_num,nplayers);
  cerr << nplayers << " players\n";
  names.resize(nplayers);
  for (i=0; i<nplayers; i++)
    { octet tmp[512];
      receive(socket_num,tmp,512);
      names[i]=(char*)tmp;
      cerr << "Player " << i << " is running on machine " << names[i] << endl;
    }
  close_client_socket(socket_num);
}


void Names::setup_server()
{
  server = new ServerSocket(portnum_base + player_no);
}


Names::Names(const Names& other)
{
  if (other.server != 0)
      throw runtime_error("Can copy Names only when uninitialized");
  player_no = other.player_no;
  nplayers = other.nplayers;
  portnum_base = other.portnum_base;
  names = other.names;
  server = 0;
}


Names::~Names()
{
  if (server != 0)
    delete server;
}



Player::Player(const Names& Nms, int id) : PlayerBase(Nms), send_to_self_socket(-1)
{
  nplayers=Nms.nplayers;
  player_no=Nms.player_no;
  setup_sockets(Nms.names, Nms.portnum_base, id, *Nms.server);
  blk_SHA1_Init(&ctx);
}


Player::~Player()
{ 
  /* Close down the sockets */
  for (int i=0; i<nplayers; i++)
    close_client_socket(sockets[i]);
}



// Set up nmachines client and server sockets to send data back and fro
//   A machine is a server between it and player i if i<=my_number
//   Can also communicate with myself, but only with send_to and receive_from
void Player::setup_sockets(const vector<string>& names,int portnum_base,int id_base,ServerSocket& server)
{
  sockets.resize(nplayers);
  // Set up the client side
  for (int i=player_no; i<nplayers; i++)
    { int pn=id_base+i*nplayers+player_no;
      fprintf(stderr, "Setting up client to %s:%d with id 0x%x\n",names[i].c_str(),portnum_base+i,pn);
      set_up_client_socket(sockets[i],names[i].c_str(),portnum_base+i);
      send(sockets[i], (unsigned char*)&pn, sizeof(pn));
    }
  send_to_self_socket = sockets[player_no];
  // Setting up the server side
  for (int i=0; i<=player_no; i++)
    { int id=id_base+player_no*nplayers+i;
      fprintf(stderr, "Setting up server with id 0x%x\n",id);
      sockets[i] = server.get_connection_socket(id);
    }

  for (int i = 0; i < nplayers; i++)
    {
      // timeout of 5 minutes
      struct timeval tv;
      tv.tv_sec = 300;
      tv.tv_usec = 0;
      int fl = setsockopt(sockets[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
      if (fl<0) { error("set_up_socket:setsockopt");  }
      socket_players[sockets[i]] = i;
    }
}


void Player::send_to(int player,const octetStream& o,bool donthash) const
{ 
  int socket = socket_to_send(player);
  o.Send(socket);
  if (!donthash)
    { blk_SHA1_Update(&ctx,o.get_data(),o.get_length()); }
}


void Player::send_all(const octetStream& o,bool donthash) const
{ for (int i=0; i<nplayers; i++)
     { if (i!=player_no)
         { o.Send(sockets[i]); }
     }
  if (!donthash)
    { blk_SHA1_Update(&ctx,o.get_data(),o.get_length()); }
}


void Player::receive_player(int i,octetStream& o,bool donthash) const
{
  o.reset_write_head();
  o.Receive(sockets[i]);
  if (!donthash)
    { blk_SHA1_Update(&ctx,o.get_data(),o.get_length()); }
}

/* This is deliberately weird to avoid problems with OS max buffer
 * size getting in the way
 */
void Player::Broadcast_Receive(vector<octetStream>& o,bool donthash) const
{ for (int i=0; i<nplayers; i++)
     { if (i>player_no)
	 { o[player_no].Send(sockets[i]); }
       else if (i<player_no)  
         { o[i].reset_write_head();
           o[i].Receive(sockets[i]); 
         }
     }
  for (int i=0; i<nplayers; i++)
     { if (i<player_no)
         { o[player_no].Send(sockets[i]); }
       else if (i>player_no)   
         { o[i].reset_write_head();
           o[i].Receive(sockets[i]); 
         }
     }
  if (!donthash)
    { for (int i=0; i<nplayers; i++)
        { blk_SHA1_Update(&ctx,o[i].get_data(),o[i].get_length()); }
    }
}


void Player::Check_Broadcast() const
{
  octet hashVal[HASH_SIZE];
  vector<octetStream> h(nplayers);
  blk_SHA1_Final(hashVal,&ctx);
  h[player_no].append(hashVal,HASH_SIZE);

  Broadcast_Receive(h,true);
  for (int i=0; i<nplayers; i++)
    { if (i!=player_no) 
        { if (!h[i].equals(h[player_no]))
	    { throw broadcast_invalid(); }
        }
    }
  blk_SHA1_Init(&ctx);
}


void Player::wait_for_available(vector<int>& players, vector<int>& result) const
{
  fd_set rfds;
  FD_ZERO(&rfds);
  int highest = 0;
  vector<int>::iterator it;
  for (it = players.begin(); it != players.end(); it++)
    {
      if (*it >= 0)
        {
          FD_SET(sockets[*it], &rfds);
          highest = max(highest, sockets[*it]);
        }
    }

  int res = select(highest + 1, &rfds, 0, 0, 0);

  if (res < 0)
    error("select()");

  result.clear();
  result.reserve(res);
  for (it = players.begin(); it != players.end(); it++)
    {
      if (res == 0)
        break;

      if (*it >= 0 && FD_ISSET(sockets[*it], &rfds))
        {
          res--;
          result.push_back(*it);
        }
    }
}


ThreadPlayer::ThreadPlayer(const Names& Nms, int id_base) : Player(Nms, id_base)
{
  for (int i = 0; i < Nms.num_players(); i++)
    {
      receivers.push_back(new Receiver(sockets[i]));
      receivers[i]->start();

      senders.push_back(new Sender(socket_to_send(i)));
      senders[i]->start();
    }
}

ThreadPlayer::~ThreadPlayer()
{
  for (unsigned int i = 0; i < receivers.size(); i++)
    {
      receivers[i]->stop();
      if (receivers[i]->timer.elapsed() > 0)
        cerr << "Waiting for receiving from " << i << ": " << receivers[i]->timer.elapsed() << endl;
      delete receivers[i];
    }

  for (unsigned int i = 0; i < senders.size(); i++)
    {
      senders[i]->stop();
      if (senders[i]->timer.elapsed() > 0)
        cerr << "Waiting for sending to " << i << ": " << senders[i]->timer.elapsed() << endl;
      delete senders[i];
    }
}

void ThreadPlayer::request_receive(int i, octetStream& o) const
{
  receivers[i]->request(o);
}

void ThreadPlayer::wait_receive(int i, octetStream& o, bool donthash) const
{
  receivers[i]->wait(o);
  if (!donthash)
    { blk_SHA1_Update(&ctx,o.get_data(),o.get_length()); }
}

void ThreadPlayer::receive_player(int i, octetStream& o, bool donthash) const
{
  request_receive(i, o);
  wait_receive(i, o, donthash);
}

void ThreadPlayer::send_all(const octetStream& o,bool donthash) const
{
  for (int i=0; i<nplayers; i++)
     { if (i!=player_no)
         senders[i]->request(o);
     }

  if (!donthash)
    { blk_SHA1_Update(&ctx,o.get_data(),o.get_length()); }

  for (int i = 0; i < nplayers; i++)
    if (i != player_no)
      senders[i]->wait(o);
}


TwoPartyPlayer::TwoPartyPlayer(const Names& Nms, int other_player, int id) : PlayerBase(Nms), other_player(other_player)
{
  is_server = Nms.my_num() > other_player;
  setup_sockets(Nms.names[other_player].c_str(), *Nms.server, Nms.portnum_base + other_player, id);
}

TwoPartyPlayer::~TwoPartyPlayer()
{ 
  close_client_socket(socket);
}

void TwoPartyPlayer::setup_sockets(const char* hostname, ServerSocket& server, int pn, int id)
{
  if (is_server)
    {
      fprintf(stderr, "Setting up server with id %d\n",id);
      socket = server.get_connection_socket(id);
    }
  else
    {
      fprintf(stderr, "Setting up client to %s:%d with id %d\n", hostname, pn, id);
      set_up_client_socket(socket, hostname, pn);
      ::send(socket, (unsigned char*)&id, sizeof(id));
    }
}

int TwoPartyPlayer::other_player_num() const
{
  return other_player;
}

void TwoPartyPlayer::send(octetStream& o) const
{
  o.Send(socket);
}

void TwoPartyPlayer::receive(octetStream& o) const
{
  o.reset_write_head();
  o.Receive(socket);
}

void TwoPartyPlayer::send_receive_player(vector<octetStream>& o) const
{
  {
    if (is_server)
    {
      o[0].Send(socket);
      o[1].reset_write_head();
      o[1].Receive(socket);
    }
    else
    {
      o[1].reset_write_head();
      o[1].Receive(socket);
      o[0].Send(socket);
    }
  }
}
