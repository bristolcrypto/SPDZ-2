// (C) 2018 University of Bristol. See License.txt

#include "Processor/ExternalClients.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

ExternalClients::ExternalClients(int party_num, const string& prep_data_dir):
   party_num(party_num), prep_data_dir(prep_data_dir), server_connection_count(-1)
{
}

ExternalClients::~ExternalClients() 
{
  // close client sockets
  for (map<int,int>::iterator it = external_client_sockets.begin();
    it != external_client_sockets.end(); it++)
  {
    if (close(it->second))
    {
       error("failed to close external client connection socket)");
    }
  }
  for (map<int,AnonymousServerSocket*>::iterator it = client_connection_servers.begin();
    it != client_connection_servers.end(); it++)
  {
    delete it->second;
  }
  for (map<int,octet*>::iterator it = symmetric_client_keys.begin();
    it != symmetric_client_keys.end(); it++)
  {
    delete[] it->second;
  }
  for (map<int, pair<vector<octet>,uint64_t> >::iterator it_cs = symmetric_client_commsec_send_keys.begin();
    it_cs != symmetric_client_commsec_send_keys.end(); it_cs++)
  {
    memset(&(it_cs->second.first[0]), 0, it_cs->second.first.size());
  }
  for (map<int, pair<vector<octet>,uint64_t> >::iterator it_cs = symmetric_client_commsec_recv_keys.begin();
    it_cs != symmetric_client_commsec_recv_keys.end(); it_cs++)
  {
    memset(&(it_cs->second.first[0]), 0, it_cs->second.first.size());
  }
}

void ExternalClients::start_listening(int portnum_base)
{
  client_connection_servers[portnum_base] = new AnonymousServerSocket(portnum_base + get_party_num());
  client_connection_servers[portnum_base]->init();
  cerr << "Start listening on thread " << this_thread::get_id() << endl;
  cerr << "Party " << get_party_num() << " is listening on port " << (portnum_base + get_party_num()) 
        << " for external client connections." << endl;
}

int ExternalClients::get_client_connection(int portnum_base)
{
  map<int,AnonymousServerSocket*>::iterator it = client_connection_servers.find(portnum_base);
  if (it == client_connection_servers.end())
  {
    cerr << "Thread " << this_thread::get_id() << " didn't find server." << endl; 
    return -1;
  }
  cerr << "Thread " << this_thread::get_id() << " found server." << endl; 
  int client_id, socket;
  socket = client_connection_servers[portnum_base]->get_connection_socket(client_id);
  external_client_sockets[client_id] = socket;
  cerr << "Party " << get_party_num() << " received external client connection from client id: " << dec << client_id << endl;
  return client_id;
}

int ExternalClients::connect_to_server(int portnum_base, int ipv4_address)
{
  struct in_addr addr = { (unsigned int)ipv4_address };
  int csocket;
  const char* address_str = inet_ntoa(addr);
  cerr << "Party " << get_party_num() << " connecting to server at " << address_str << " on port " << portnum_base + get_party_num() << endl;
  set_up_client_socket(csocket, address_str, portnum_base + get_party_num());
  cerr << "Party " << get_party_num() << " connected to server at " << address_str << " on port " << portnum_base + get_party_num() << endl;
  int server_id = server_connection_count;
  // server identifiers are -1, -2, ... to avoid conflict with client identifiers
  server_connection_count--;
  external_client_sockets[server_id] = csocket;
  return server_id;
}

void ExternalClients::curve25519_ints_to_bytes(unsigned char *bytes,  const vector<int>& key_ints)
{
  for(unsigned int j = 0; j < key_ints.size(); j++) {
    for(unsigned int k = 0; k < 4; k++) {
      bytes[j*sizeof(int) + k] = (key_ints[j] >> ((3-k)*8)) & 0xFF;
    }
  }
}

// Generate sesssion key for a newly connected client, store in symmetric_client_keys
// public_key is expected to be size 8 and contain integer values of public key bytes.
// Assumes load_server_keys has been run.
void ExternalClients::generate_session_key_for_client(int client_id, const vector<int>& public_key)
{
  assert(public_key.size() * sizeof(int) == crypto_box_PUBLICKEYBYTES);

  load_server_keys_once();

  unsigned char client_publickey[crypto_box_PUBLICKEYBYTES];

  curve25519_ints_to_bytes(client_publickey, public_key);

  cerr << "Recevied client public key for client " << dec << client_id << " :";
  for (unsigned int j = 0; j < crypto_box_PUBLICKEYBYTES; j++)
    cerr << hex << (int) client_publickey[j] << " ";
  cerr << dec << endl;

  unsigned char scalarmult_q_by_server[crypto_scalarmult_BYTES];
  crypto_generichash_state h;

  symmetric_client_keys[client_id] = new octet[crypto_generichash_BYTES];
    
  // Derive a shared key from this server's secret key and the client's public key
  // shared key = h(q || server_secretkey || client_publickey)
  if (crypto_scalarmult(scalarmult_q_by_server, server_secretkey, client_publickey) != 0) {
      cerr << "Scalar mult failed\n";
      exit(1);
  }
  crypto_generichash_init(&h, NULL, 0U, crypto_generichash_BYTES);
  crypto_generichash_update(&h, scalarmult_q_by_server, sizeof scalarmult_q_by_server);
  crypto_generichash_update(&h, client_publickey, sizeof client_publickey);
  crypto_generichash_update(&h, server_publickey, sizeof server_publickey);
  crypto_generichash_final(&h, symmetric_client_keys[client_id], crypto_generichash_BYTES);
}

// Read pre-computed server keys from client-setup for this SPDZ engine.
// Only needs to be done once per run, but is only necessary if an external connection
// is being requested.
void ExternalClients::load_server_keys_once()
{
  if (server_keys_loaded) {
    return;
  }

  ifstream keyfile;
  stringstream filename;
  filename << prep_data_dir << "Player-SPDZ-Keys-P" << get_party_num();
  keyfile.open(filename.str().c_str());
  if (keyfile.fail())
      throw file_error(filename.str().c_str());

  keyfile.read((char*)server_publickey, sizeof server_publickey);
  if (keyfile.eof())
    throw end_of_file(filename.str(), "server public key" );
  keyfile.read((char*)server_secretkey, sizeof server_secretkey);
  if (keyfile.eof())
    throw end_of_file(filename.str(), "server private key" );

  bool loaded_ed25519 = true;

  keyfile.read((char*)server_publickey_ed25519, sizeof server_publickey_ed25519);
  if (keyfile.eof() || keyfile.bad())
    loaded_ed25519 = false;
  keyfile.read((char*)server_secretkey_ed25519, sizeof server_secretkey_ed25519);
  if (keyfile.eof() || keyfile.bad())
    loaded_ed25519 = false;

  keyfile.close();

  ed25519_keys_loaded = loaded_ed25519;
  server_keys_loaded = true;
}

void ExternalClients::require_ed25519_keys()
{
    if (!ed25519_keys_loaded)
        throw "Ed25519 keys required but not found in player key files";
}

int ExternalClients::get_party_num() 
{
  return party_num;
}

