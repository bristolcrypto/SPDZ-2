// (C) 2018 University of Bristol. See License.txt


#include "Player.h"
#include "Exceptions/Exceptions.h"
#include "Networking/STS.h"

#include <sys/select.h>
#include <utility>

using namespace std;

CommsecKeysPackage::CommsecKeysPackage(vector<public_signing_key> playerpubs,
                                             secret_signing_key mypriv,
                                             public_signing_key mypub)
{
    player_public_keys = playerpubs;
    my_secret_key = mypriv;
    my_public_key = mypub;
}

void Names::init(int player,int pnb,int my_port,const char* servername)
{
  player_no=player;
  portnum_base=pnb;
  setup_names(servername, my_port);
  keys = NULL;
  setup_server();
}

void Names::init(int player,int pnb,vector<string> Nms)
{
  vector<octet*> names;
  for (auto& name : Nms)
    names.push_back((octet*)name.c_str());
  init(player, pnb, names);
}

void Names::init(int player,int pnb,vector<octet*> Nms)
{
  player_no=player;
  portnum_base=pnb;
  nplayers=Nms.size();
  names.resize(nplayers);
  setup_ports();
  for (int i=0; i<nplayers; i++) {
      names[i]=(char*)Nms[i];
  }
  keys = NULL;
  setup_server();
}

// initialize names from file, no Server.x coordination.
void Names::init(int player, int pnb, const string& filename, int nplayers_wanted)
{
  ifstream hostsfile(filename.c_str());
  if (hostsfile.fail())
  {
     stringstream ss;
     ss << "Error opening " << filename << ". See HOSTS.example for an example.";
     throw file_error(ss.str().c_str());
  }
  player_no = player;
  nplayers = 0;
  portnum_base = pnb;
  keys = NULL;
  string line;
  while (getline(hostsfile, line))
  {
    if (line.length() > 0 && line.at(0) != '#') {
      names.push_back(line);
      nplayers++;
      if (nplayers_wanted > 0 and nplayers_wanted == nplayers)
        break;
    }
  }
  setup_ports();
  cerr << "Got list of " << nplayers << " players from file: " << endl;
  for (unsigned int i = 0; i < names.size(); i++)
    cerr << "    " << names[i] << endl;
  setup_server();
}

void Names::setup_ports()
{
  ports.resize(nplayers);
  for (int i = 0; i < nplayers; i++)
    ports[i] = default_port(i);
}

void Names::set_keys( CommsecKeysPackage *keys )
{
    this->keys = keys;
}

void Names::setup_names(const char *servername, int my_port)
{
  if (my_port == DEFAULT_PORT)
    my_port = default_port(player_no);

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
  sockaddr_in address;
  socklen_t size = sizeof address;
  getsockname(socket_num, (sockaddr*)&address, &size);
  char* name = inet_ntoa(address.sin_addr);
  // max length of IP address with ending 0
  strncpy((char*)my_name, name, 16);
  fprintf(stderr, "My Name = %s\n",my_name);
  send(socket_num,my_name,512);
  send(socket_num,(octet*)&my_port,4);
  cerr << "My number = " << player_no << endl;

  // Now get the set of names
  int i;
  receive(socket_num,nplayers);
  cerr << nplayers << " players\n";
  names.resize(nplayers);
  ports.resize(nplayers);
  for (i=0; i<nplayers; i++)
    { octet tmp[512];
      receive(socket_num,tmp,512);
      names[i]=(char*)tmp;
      receive(socket_num, (octet*)&ports[i], 4);
      cerr << "Player " << i << " is running on machine " << names[i] << endl;
    }
  close_client_socket(socket_num);
}


void Names::setup_server()
{
  server = new ServerSocket(ports[player_no]);
  server->init();
}


Names::Names(const Names& other)
{
  if (other.server != 0)
      throw runtime_error("Can copy Names only when uninitialized");
  player_no = other.player_no;
  nplayers = other.nplayers;
  portnum_base = other.portnum_base;
  names = other.names;
  ports = other.ports;
  keys = NULL;
  server = 0;
}


Names::~Names()
{
  if (server != 0)
    delete server;
}



Player::Player(const Names& Nms, int id) :
        PlayerBase(Nms.my_num()), send_to_self_socket(-1)
{
  nplayers=Nms.nplayers;
  player_no=Nms.player_no;
  setup_sockets(Nms.names, Nms.ports, id, *Nms.server);
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
void Player::setup_sockets(const vector<string>& names,const vector<int>& ports,int id_base,ServerSocket& server)
{
    sockets.resize(nplayers);
    // Set up the client side
    for (int i=player_no; i<nplayers; i++) {
        int pn=id_base+i*nplayers+player_no;
        if (i==player_no) {
          const char* localhost = "127.0.0.1";
          fprintf(stderr, "Setting up send to self socket to %s:%d with id 0x%x\n",localhost,ports[i],pn);
          set_up_client_socket(sockets[i],localhost,ports[i]);
        } else {
          fprintf(stderr, "Setting up client to %s:%d with id 0x%x\n",names[i].c_str(),ports[i],pn);
          set_up_client_socket(sockets[i],names[i].c_str(),ports[i]);
        }
        send(sockets[i], (unsigned char*)&pn, sizeof(pn));
    }
    send_to_self_socket = sockets[player_no];
    // Setting up the server side
    for (int i=0; i<=player_no; i++) {
        int id=id_base+player_no*nplayers+i;
        fprintf(stderr, "As a server, waiting for client with id 0x%x to connect.\n",id);
        sockets[i] = server.get_connection_socket(id);
    }

    for (int i = 0; i < nplayers; i++) {
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
  TimeScope ts(timer);
  int socket = socket_to_send(player);
  o.Send(socket);
  if (!donthash)
    { blk_SHA1_Update(&ctx,o.get_data(),o.get_length()); }
  sent += o.get_length();
}


void Player::send_all(const octetStream& o,bool donthash) const
{
  TimeScope ts(timer);
  for (int i=0; i<nplayers; i++)
     { if (i!=player_no)
         { o.Send(sockets[i]); }
     }
  if (!donthash)
    { blk_SHA1_Update(&ctx,o.get_data(),o.get_length()); }
  sent += o.get_length() * (num_players() - 1);
}


void Player::receive_player(int i,octetStream& o,bool donthash) const
{
  TimeScope ts(timer);
  o.reset_write_head();
  o.Receive(sockets[i]);
  if (!donthash)
    { blk_SHA1_Update(&ctx,o.get_data(),o.get_length()); }
}


void Player::exchange(int other, octetStream& o) const
{
  TimeScope ts(timer);
  o.exchange(sockets[other], sockets[other]);
  sent += o.get_length();
}


void Player::pass_around(octetStream& o, int offset) const
{
  TimeScope ts(timer);
  o.exchange(sockets.at((my_num() + offset) % num_players()),
      sockets.at((my_num() + num_players() - offset) % num_players()));
  sent += o.get_length();
}


/* This is deliberately weird to avoid problems with OS max buffer
 * size getting in the way
 */
void Player::Broadcast_Receive(vector<octetStream>& o,bool donthash) const
{
  TimeScope ts(timer);
  for (int i=0; i<nplayers; i++)
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
  sent += o[player_no].get_length() * (num_players() - 1);
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


TwoPartyPlayer::TwoPartyPlayer(const Names& Nms, int other_player, int id) :
        PlayerBase(Nms.my_num()), other_player(other_player)
{
  is_server = Nms.my_num() > other_player;
  setup_sockets(other_player, Nms, Nms.ports[other_player], id);
}

TwoPartyPlayer::~TwoPartyPlayer()
{
  for(size_t i=0; i < my_secret_key.size(); i++) {
      my_secret_key[i] = 0;
  }
  close_client_socket(socket);
}

static pair<keyinfo,keyinfo> sts_initiator(int socket, CommsecKeysPackage *keys, int other_player)
{
  sts_msg1_t m1;
  sts_msg2_t m2;
  sts_msg3_t m3;
  octetStream socket_stream;

  // Start Station to Station Protocol
  STS ke(&keys->player_public_keys[other_player][0], &keys->my_public_key[0], &keys->my_secret_key[0]);
  m1 = ke.send_msg1();
  socket_stream.reset_write_head();
  socket_stream.append(m1.bytes, sizeof m1.bytes);
  socket_stream.Send(socket);
  socket_stream.Receive(socket);
  socket_stream.consume(m2.pubkey, sizeof m2.pubkey);
  socket_stream.consume(m2.sig, sizeof m2.sig);
  m3 = ke.recv_msg2(m2);
  socket_stream.reset_write_head();
  socket_stream.append(m3.bytes, sizeof m3.bytes);
  socket_stream.Send(socket);

  // Use results of STS to generate send and receive keys.
  vector<unsigned char> sendKey = ke.derive_secret(crypto_secretbox_KEYBYTES);
  vector<unsigned char> recvKey = ke.derive_secret(crypto_secretbox_KEYBYTES);
  keyinfo sendkeyinfo = make_pair(sendKey,0);
  keyinfo recvkeyinfo = make_pair(recvKey,0);
  return make_pair(sendkeyinfo,recvkeyinfo);
}

static pair<keyinfo,keyinfo> sts_responder(int socket, CommsecKeysPackage *keys, int other_player)
    // secret_signing_key mykey, public_signing_key mypubkey, public_signing_key theirkey)
{
  sts_msg1_t m1;
  sts_msg2_t m2;
  sts_msg3_t m3;
  octetStream socket_stream;

  // Start Station to Station Protocol for the responder
  STS ke(&keys->player_public_keys[other_player][0], &keys->my_public_key[0], &keys->my_secret_key[0]);
  socket_stream.Receive(socket);
  socket_stream.consume(m1.bytes, sizeof m1.bytes);
  m2 = ke.recv_msg1(m1);
  socket_stream.reset_write_head();
  socket_stream.append(m2.pubkey, sizeof m2.pubkey);
  socket_stream.append(m2.sig, sizeof m2.sig);
  socket_stream.Send(socket);
  socket_stream.Receive(socket);
  socket_stream.consume(m3.bytes, sizeof m3.bytes);
  ke.recv_msg3(m3);

  // Use results of STS to generate send and receive keys.
  vector<unsigned char> recvKey = ke.derive_secret(crypto_secretbox_KEYBYTES);
  vector<unsigned char> sendKey = ke.derive_secret(crypto_secretbox_KEYBYTES);
  keyinfo sendkeyinfo = make_pair(sendKey,0);
  keyinfo recvkeyinfo = make_pair(recvKey,0);
  return make_pair(sendkeyinfo,recvkeyinfo);
}

void TwoPartyPlayer::setup_sockets(int other_player, const Names &nms, int portNum, int id)
{
    const char *hostname = nms.names[other_player].c_str();
    ServerSocket *server = nms.server;
    if (is_server) {
        fprintf(stderr, "Setting up server with id %d\n",id);
        socket = server->get_connection_socket(id);
        if(NULL != nms.keys) {
            pair<keyinfo,keyinfo> send_recv_pair = sts_responder(socket, nms.keys, other_player);
            player_send_key = send_recv_pair.first;
            player_recv_key = send_recv_pair.second;
        }
    }
    else {
        fprintf(stderr, "Setting up client to %s:%d with id %d\n", hostname, portNum, id);
        set_up_client_socket(socket, hostname, portNum);
        ::send(socket, (unsigned char*)&id, sizeof(id));
        if(NULL != nms.keys) {
            pair<keyinfo,keyinfo> send_recv_pair = sts_initiator(socket, nms.keys, other_player);
            player_send_key = send_recv_pair.first;
            player_recv_key = send_recv_pair.second;
        }
    }
    p2pcommsec = (0 != nms.keys);
}

int TwoPartyPlayer::other_player_num() const
{
  return other_player;
}

void TwoPartyPlayer::send(octetStream& o)
{
  if(p2pcommsec) {
    o.encrypt_sequence(&player_send_key.first[0], player_send_key.second);
    player_send_key.second++;
  }
  TimeScope ts(timer);
  o.Send(socket);
  sent += o.get_length();
}

void TwoPartyPlayer::receive(octetStream& o)
{
  TimeScope ts(timer);
  o.reset_write_head();
  o.Receive(socket);
  if(p2pcommsec) {
    o.decrypt_sequence(&player_recv_key.first[0], player_recv_key.second);
    player_recv_key.second++;
  }
}

void TwoPartyPlayer::send_receive_player(vector<octetStream>& o)
{
  {
    if (is_server)
    {
      send(o[0]);
      receive(o[1]);
    }
    else
    {
      receive(o[1]);
      send(o[0]);
    }
  }
}

void TwoPartyPlayer::exchange(octetStream& o) const
{
  TimeScope ts(timer);
  sent += o.get_length();
  o.exchange(socket, socket);
}
