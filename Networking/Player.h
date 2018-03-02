// (C) 2018 University of Bristol. See License.txt

#ifndef _Player
#define _Player

/* Class to create a player, for KeyGen, Offline and Online phases.
 *
 * Basically handles connection to the server to obtain the names
 * of the other players. Plus sending and receiving of data
 *
 */

#include <vector>
#include <set>
#include <iostream>
#include <fstream>
using namespace std;

#include "Tools/octetStream.h"
#include "Networking/sockets.h"
#include "Networking/ServerSocket.h"
#include "Tools/sha1.h"
#include "Networking/Receiver.h"
#include "Networking/Sender.h"

typedef vector<octet> public_signing_key;
typedef vector<octet> secret_signing_key;
typedef vector<octet> chachakey;
typedef pair< chachakey, uint64_t > keyinfo;

class CommsecKeysPackage {
public:
    vector<public_signing_key> player_public_keys;
    secret_signing_key my_secret_key;
    public_signing_key my_public_key;

    CommsecKeysPackage(vector<public_signing_key> playerpubs,
                       secret_signing_key mypriv,
                       public_signing_key mypub);
    ~CommsecKeysPackage();
};

/* Class to get the names off the server */
class Names
{
  vector<string> names;
  vector<int> ports;
  int nplayers;
  int portnum_base;
  int player_no;

  CommsecKeysPackage *keys;

  int default_port(int playerno) { return portnum_base + playerno; }
  void setup_ports();

  void setup_names(const char *servername, int my_port);

  void setup_server();

  public:

  static const int DEFAULT_PORT = -1;

  mutable ServerSocket* server;

  void init(int player,int pnb,int my_port,const char* servername);
  Names(int player,int pnb,int my_port,const char* servername)
    { init(player,pnb,my_port,servername); }
  // Set up names when we KNOW who we are going to be using before hand
  void init(int player,int pnb,vector<octet*> Nms);
  Names(int player,int pnb,vector<octet*> Nms)
    { init(player,pnb,Nms); }
  void init(int player,int pnb,vector<string> Nms);
  Names(int player,int pnb,vector<string> Nms)
    { init(player,pnb,Nms); }
  // nplayers = 0 for taking it from hostsfile
  void init(int player, int pnb, const string& hostsfile, int players = 0);
  Names(int player, int pnb, const string& hostsfile)
    { init(player, pnb, hostsfile); }
  void set_keys( CommsecKeysPackage *keys );

  Names() : nplayers(-1), portnum_base(-1), player_no(-1), keys(0), server(0) { ; }
  Names(const Names& other);
  ~Names();

  int num_players() const { return nplayers; }
  int my_num() const { return player_no; }
  const string get_name(int i) const { return names[i]; }
  int get_portnum_base() const { return portnum_base; }

  friend class PlayerBase;
  friend class Player;
  friend class TwoPartyPlayer;
};


class PlayerBase
{
protected:
  int player_no;

public:
  mutable size_t sent;
  mutable Timer timer;

  PlayerBase(int player_no) : player_no(player_no), sent(0) {}
  virtual ~PlayerBase() {}

  int my_real_num() const { return player_no; }

  virtual int my_num() const = 0;
  virtual int num_players() const = 0;

  virtual void exchange(int other, octetStream& o) const = 0;
  virtual void pass_around(octetStream& o, int offset = 1) const = 0;
};

class Player : public PlayerBase
{
protected:
  vector<int> sockets;
  int send_to_self_socket;

  void setup_sockets(const vector<string>& names,const vector<int>& ports,int id_base,ServerSocket& server);

  int nplayers;

  mutable blk_SHA_CTX ctx;

  map<int,int> socket_players;

  int socket_to_send(int player) const { return player == player_no ? send_to_self_socket : sockets[player]; }

public:
  // The offset is used for the multi-threaded call, to ensure different
  // portnum bases in each thread
  Player(const Names& Nms,int id_base=0);

  virtual ~Player();

  int num_players() const { return nplayers; }
  int my_num() const { return player_no; }
  int socket(int i) const { return sockets[i]; }

  // Send/Receive data to/from player i 
  // 8-bit ints only (mainly for testing)
  void send_int(int i,int a)  const    { send(sockets[i],a);    }
  void receive_int(int i,int& a) const { receive(sockets[i],a); }

  // Send an octetStream to all other players 
  //   -- And corresponding receive
  virtual void send_all(const octetStream& o,bool donthash=false) const;
  void send_to(int player,const octetStream& o,bool donthash=false) const;
  virtual void receive_player(int i,octetStream& o,bool donthash=false) const;

  // exchange data with minimal memory usage
  void exchange(int other, octetStream& o) const;

  // send to next and receive from previous player
  void pass_around(octetStream& o, int offset = 1) const;

  // Receive one from player i

  /* Broadcast and Receive data to/from all players 
   *  - Assumes o[player_no] contains the thing broadcast by me
   */
  void Broadcast_Receive(vector<octetStream>& o,bool donthash=false) const;

  /* Run Protocol To Verify Broadcast Is Correct
   *     - Resets the blk_SHA_CTX at the same time
   */
  void Check_Broadcast() const;

  // wait for available inputs
  void wait_for_available(vector<int>& players, vector<int>& result) const;

  // dummy functions for compatibility
  virtual void request_receive(int i, octetStream& o) const { sockets[i]; o.get_length(); }
  virtual void wait_receive(int i, octetStream& o, bool donthash=false) const { receive_player(i, o, donthash); }
};


class ThreadPlayer : public Player
{
public:
  mutable vector<Receiver*> receivers;
  mutable vector<Sender*>   senders;

  ThreadPlayer(const Names& Nms,int id_base=0);
  virtual ~ThreadPlayer();

  void request_receive(int i, octetStream& o) const;
  void wait_receive(int i, octetStream& o, bool donthash=false) const;
  void receive_player(int i,octetStream& o,bool donthash=false) const;

  void send_all(const octetStream& o,bool donthash=false) const;
};


class TwoPartyPlayer : public PlayerBase
{
private:
  // setup sockets for comm. with only one other player
  void setup_sockets(int other_player, const Names &nms, int portNum, int id);

  int socket;
  bool is_server;
  int other_player;
  bool p2pcommsec;

  secret_signing_key my_secret_key;
  map<int,public_signing_key> player_public_keys;
  keyinfo player_send_key;
  keyinfo player_recv_key;

public:
  TwoPartyPlayer(const Names& Nms, int other_player, int pn_offset=0);
  ~TwoPartyPlayer();

  void send(octetStream& o);
  void receive(octetStream& o);

  int other_player_num() const;
  int my_num() const { return is_server; }
  int num_players() const { return 2; }

  /* Send and receive to/from the other player
   *  - o[0] contains my data, received data put in o[1]
   */
  void send_receive_player(vector<octetStream>& o);

  void exchange(octetStream& o) const;
  void exchange(int other, octetStream& o) const { (void)other; exchange(o); }
  void pass_around(octetStream& o, int offset = 1) const { (void)offset; exchange(o); }
};


class OffsetPlayer : public PlayerBase
{
private:
  Player& P;
  int offset;

public:
  OffsetPlayer(Player& P, int offset) : PlayerBase(P.my_num()), P(P), offset(offset) {}

  int my_num() const { return 0; }
  int num_players() const { return 2; }
  int get_offset() const { return offset; }

  void reverse_exchange(octetStream& o) const { P.pass_around(o, P.num_players() - offset); }
  void exchange(int other, octetStream& o) const { (void)other; P.pass_around(o, offset); }
  void pass_around(octetStream& o, int _ = 1) const { (void)_; P.pass_around(o, offset); }
  void Broadcast_Receive(vector<octetStream>& o,bool donthash=false) const;
};

#endif
