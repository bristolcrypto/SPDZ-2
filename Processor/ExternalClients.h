// (C) 2018 University of Bristol. See License.txt

#ifndef _ExternalClients
#define _ExternalClients

#include "Networking/ServerSocket.h"
#include "Networking/sockets.h"
#include "Exceptions/Exceptions.h"
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sodium.h>
#include <assert.h>

/*
 * Manage the reading and writing of data from/to external clients via Sockets.
 * Generate the session keys for encryption/decryption of secret communication with external clients.
 */

class ExternalClients
{
  map<int,AnonymousServerSocket*> client_connection_servers;
  
  int party_num;
  const string prep_data_dir;
  int server_connection_count;  
  unsigned char server_publickey[crypto_box_PUBLICKEYBYTES];
  unsigned char server_secretkey[crypto_box_SECRETKEYBYTES];
  bool server_keys_loaded = false;
  bool ed25519_keys_loaded = false;

  public:

  unsigned char server_publickey_ed25519[crypto_sign_ed25519_PUBLICKEYBYTES];
  unsigned char server_secretkey_ed25519[crypto_sign_ed25519_SECRETKEYBYTES];

  // Maps holding per client values (indexed by unique 32-bit id)
  std::map<int,int> external_client_sockets;
  std::map<int,octet*> symmetric_client_keys;
  std::map<int,pair<vector<octet>,uint64_t>> symmetric_client_commsec_send_keys;
  std::map<int,pair<vector<octet>,uint64_t>> symmetric_client_commsec_recv_keys;

  ExternalClients(int party_num, const string& prep_data_dir);
  ~ExternalClients();

  void start_listening(int portnum_base);

  int get_client_connection(int portnum_base);

  int connect_to_server(int portnum_base, int ipv4_address);

  // return the socket for a given client or server identifier
  int get_socket(int socket_id);

  void curve25519_ints_to_bytes(unsigned char bytes[crypto_box_PUBLICKEYBYTES],  const vector<int>& key_ints);
  void generate_session_key_for_client(int client_id, const vector<int>& public_key);  

  void load_server_keys_once();

  int get_party_num();
  void require_ed25519_keys();
};

#endif
