/*
 * (C) 2018 University of Bristol. See License.txt
 *
 * Demonstrate external client inputing and receiving outputs from a SPDZ process, 
 * following the protocol described in https://eprint.iacr.org/2015/1006.pdf. 
 * Uses SPDZ implemented encryption for external client communication, see bankers-bonus-client.cpp 
 *  for a simpler client with no crypto.
 *
 * Provides a client to bankers_bonus_commsec.mpc program to calculate which banker pays for lunch based on
 * the private value annual bonus. Up to 8 clients can connect to the SPDZ engines running 
 * the bankers_bonus.mpc program.
 *
 * Each connecting client:
 * - runs crypto setup to demonstrate both DH Auth Encryption and STS protocol for comms security.
 * - sends a unique id to identify the client
 * - sends an integer input (bonus value to compare)
 * - sends an integer (0 meaining more players will join this round or 1 meaning stop the round and calc the result).
 *
 * The result is returned authenticated with a share of a random value:
 * - share of winning unique id [y]
 * - share of random value [r]
 * - share of winning unique id * random value [w]
 *   winning unique id is valid if ∑ [y] * ∑ [r] = ∑ [w]
 * 
 * To run with 2 parties (SPDZ engines) and 3 external clients:
 *   ./Scripts/setup-online.sh to create triple shares for each party (spdz engine).
 *   ./client-setup.x 2 -nc 3 to create the crypto key material for both parties and clients.
 *   ./compile.py bankers_bonus_commsec
 *   ./Scripts/run-online bankers_bonus_commsec to run the engines.
 *
 *   ./bankers-bonus-commsec-client.x 0 2 100 0
 *   ./bankers-bonus-commsec-client.x 1 2 200 0
 *   ./bankers-bonus-commsec-client.x 2 2 50 1
 *
 *   Expect winner to be second client with id 1.
 *   Note here client id must match id used in generating client key material, Client-Keys-C<client id>.
 */

#include "Math/gfp.h"
#include "Math/gf2n.h"
#include "Networking/sockets.h"
#include "Networking/STS.h"
#include "Tools/int.h"
#include "Math/Setup.h"
#include "Auth/fake-stuff.h"

#include <sodium.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

typedef pair< vector<octet>, vector<octet> > keypair_t; // A pair of send/recv keys for talking to SPDZ
typedef vector< keypair_t > commsec_t;  // A database of send/recv keys indexed by server
typedef struct {
    unsigned char client_secretkey[crypto_sign_SECRETKEYBYTES];
    unsigned char client_publickey[crypto_sign_PUBLICKEYBYTES];
    vector<int> client_publickey_ints;
    vector< vector<unsigned char> >server_publickey;
} sign_key_container_t;

keypair_t sts_response_role_exceptions(sign_key_container_t keys, vector<int>& sockets, int server_id)
{
    STS ke(&keys.server_publickey[server_id][0], keys.client_publickey, keys.client_secretkey);
    sts_msg1_t m1;
    sts_msg2_t m2;
    sts_msg3_t m3;
    octetStream os;

    os.Receive(sockets[server_id]);
    os.consume(m1.bytes, sizeof m1.bytes);
    m2 = ke.recv_msg1(m1);
    os.reset_write_head();
    os.append(m2.pubkey, sizeof m2.pubkey);
    os.append(m2.sig, sizeof m2.sig);
    os.Send(sockets[server_id]);
    os.Receive(sockets[server_id]);
    os.consume(m3.bytes, sizeof m3.bytes);
    ke.recv_msg3(m3);
    vector<unsigned char> recvKey = ke.derive_secret(crypto_secretbox_KEYBYTES);
    vector<unsigned char> sendKey = ke.derive_secret(crypto_secretbox_KEYBYTES);
    return make_pair(sendKey,recvKey);
}

keypair_t sts_initiator_role_exceptions(sign_key_container_t keys, vector<int>& sockets, int server_id)
{
    STS ke(&keys.server_publickey[server_id][0], keys.client_publickey, keys.client_secretkey);
    sts_msg1_t m1;
    sts_msg2_t m2;
    sts_msg3_t m3;
    octetStream os;

    m1 = ke.send_msg1();
    cout << "m1: ";
    for (unsigned int j = 0; j < 32; j++)
      cout << setfill('0') << setw(2) << hex << (int) m1.bytes[j];
    cout << dec << endl;
    os.reset_write_head();
    os.append(m1.bytes, sizeof m1.bytes);
    os.Send(sockets[server_id]);

    os.reset_write_head();
    os.Receive(sockets[server_id]);
    os.consume(m2.pubkey, sizeof m2.pubkey);
    os.consume(m2.sig, sizeof m2.sig);
    m3 = ke.recv_msg2(m2);

    os.reset_write_head();
    os.append(m3.bytes, sizeof m3.bytes);
    os.Send(sockets[server_id]);

    vector<unsigned char> sendKey = ke.derive_secret(crypto_secretbox_KEYBYTES);
    vector<unsigned char> recvKey = ke.derive_secret(crypto_secretbox_KEYBYTES);
    return make_pair(sendKey,recvKey);
}

pair< vector<octet>, vector<octet> > sts_response_role(sign_key_container_t keys, vector<int>& sockets, int server_id)
{
    pair< vector<octet>, vector<octet> > res;
    try {
       res = sts_response_role_exceptions(keys, sockets, server_id);
    } catch(char const *e) {
        cerr << "Error in STS: " << e << endl;
        exit(1);
    }
    return res;
}

pair< vector<octet>, vector<octet> > sts_initiator_role(sign_key_container_t keys, vector<int>& sockets, int server_id)
{
    pair< vector<octet>, vector<octet> > res;
    try {
       res = sts_initiator_role_exceptions(keys, sockets, server_id);
    } catch(char const *e) {
        cerr << "Error in STS: " << e << endl;
        exit(1);
    }
    return res;
}

// Send the private inputs masked with a random value.
// Receive shares of a preprocessed triple from each SPDZ engine, combine and check the triples are valid.
// Add the private input value to triple[0] and send to each spdz engine.
void send_private_inputs(vector<gfp>& values, vector<int>& sockets, int nparties, 
  commsec_t commsec, vector<octet*>& keys)
{
    int num_inputs = values.size();
    octetStream os;
    vector< vector<gfp> > triples(num_inputs, vector<gfp>(3));
    vector<gfp> triple_shares(3);

    // Receive num_inputs triples from SPDZ
    for (int j = 0; j < nparties; j++)
    {
        os.reset_write_head();
        os.Receive(sockets[j]);
        os.decrypt_sequence(&commsec[j].second[0],0);
        os.decrypt(keys[j]);

        for (int j = 0; j < num_inputs; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                triple_shares[k].unpack(os);
                triples[j][k] += triple_shares[k];
            }
        }
    }
    // Check triple relations
    for (int i = 0; i < num_inputs; i++)
    {
        if (triples[i][0] * triples[i][1] != triples[i][2])
        {
            cerr << "Incorrect triple at " << i << ", aborting\n";
            exit(1);
        }
    }
    // Send inputs + triple[0], so SPDZ can compute shares of each value
    os.reset_write_head();
    for (int i = 0; i < num_inputs; i++)
    {
        gfp y = values[i] + triples[i][0];
        y.pack(os);
    }
    for (int j = 0; j < nparties; j++)
    {
        octetStream temp = os;
        temp.encrypt_sequence(&commsec[j].first[0], 0);
        temp.Send(sockets[j]);
    }
  }

// Send public key in clear to each SPDZ engine.
void send_public_key(vector<int>& pubkey, int socket)
{
    octetStream os;
    os.reset_write_head();

    for (unsigned int i = 0; i < pubkey.size(); i++)
    {
        os.store(pubkey[i]);
    }

    os.Send(socket);
}

// Assumes that Scripts/setup-online.sh has been run to compute prime
void initialise_fields(const string& dir_prefix)
{
  int lg2;
  bigint p;

  string filename = dir_prefix + "Params-Data";
  cout << "loading params from: " << filename << endl;

  ifstream inpf(filename.c_str());
  if (inpf.fail()) { throw file_error(filename.c_str()); }
  inpf >> p;
  inpf >> lg2;

  inpf.close();

  gfp::init_field(p);
  gf2n::init_field(lg2);
}

// Assumes that client-setup has been run to create key pairs for clients and parties
void generate_symmetric_keys(vector<octet*>& keys, vector<int>& client_public_key_ints, 
  sign_key_container_t *sts_key, const string& dir_prefix, int client_no)
{
    unsigned char client_publickey[crypto_box_PUBLICKEYBYTES];
    unsigned char client_secretkey[crypto_box_SECRETKEYBYTES];
    unsigned char server_publickey[crypto_box_PUBLICKEYBYTES];
    unsigned char scalarmult_q[crypto_scalarmult_BYTES];
    crypto_generichash_state h;

    // read client public/secret keys + SPDZ server public keys
    ifstream keyfile;
    stringstream client_filename;
    client_filename << dir_prefix << "Client-Keys-C" << client_no;
    keyfile.open(client_filename.str().c_str());
    if (keyfile.fail())
        throw file_error(client_filename.str());
    keyfile.read((char*)client_publickey, sizeof client_publickey);
    if (keyfile.eof()) 
        throw end_of_file(client_filename.str(), "client public key" );

    // Convert client public key unsigned char to int, reverse endianness
    for(unsigned int j = 0; j < client_public_key_ints.size(); j++) {
      int keybyte = 0;
      for(unsigned int k = 0; k < 4; k++) {
        keybyte = keybyte + (((int)client_publickey[j*4+k]) << ((3-k) * 8));
      }
      client_public_key_ints[j] = keybyte;
    }

    keyfile.read((char*)client_secretkey, sizeof client_secretkey);
    if (keyfile.eof()) {
        throw end_of_file(client_filename.str(), "client private key" );
    }

    keyfile.read((char*)sts_key->client_publickey, crypto_sign_PUBLICKEYBYTES);
    keyfile.read((char*)sts_key->client_secretkey, crypto_sign_SECRETKEYBYTES);
    // Convert client public key unsigned char to int, reverse endianness
    sts_key->client_publickey_ints.resize(8);
    for(unsigned int j = 0; j < sts_key->client_publickey_ints.size(); j++) {
      int keybyte = 0;
      for(unsigned int k = 0; k < 4; k++) {
        keybyte = keybyte + (((int)sts_key->client_publickey[j*4+k]) << ((3-k) * 8));
      }
      sts_key->client_publickey_ints[j] = keybyte;
    }

    for (unsigned int i = 0; i < keys.size(); i++)
    {
        keys[i] = new octet[crypto_generichash_BYTES];
        keyfile.read((char*)server_publickey, crypto_box_PUBLICKEYBYTES);
        if (keyfile.eof())
            throw end_of_file(client_filename.str(), "server public key for party " + to_string(i));
        keyfile.read((char*)(&sts_key->server_publickey[i][0]), crypto_sign_PUBLICKEYBYTES);
        if (keyfile.eof())
            throw end_of_file(client_filename.str(), "server public signing key for party " + to_string(i));

        // Derive a shared key from this server's secret key and the client's public key
        // shared key = h(q || client_secretkey || server_publickey)
        if (crypto_scalarmult(scalarmult_q, client_secretkey, server_publickey) != 0) {
            cerr << "Scalar mult failed\n";
            exit(1);
        }
        crypto_generichash_init(&h, NULL, 0U, crypto_generichash_BYTES);
        crypto_generichash_update(&h, scalarmult_q, sizeof scalarmult_q);
        crypto_generichash_update(&h, client_publickey, sizeof client_publickey);
        crypto_generichash_update(&h, server_publickey, sizeof server_publickey);
        crypto_generichash_final(&h, keys[i], crypto_generichash_BYTES);
    }
    keyfile.close();

    cout << "My public key is: ";
    for (unsigned int j = 0; j < 32; j++)
      cout << setfill('0') << setw(2) << hex << (int) client_publickey[j];
    cout << dec << endl;
}


// Receive shares of the result and sum together.
// Also receive authenticating values.
gfp receive_result(vector<int>& sockets, int nparties, commsec_t commsec, vector<octet*>& keys)
{
    vector<gfp> output_values(3);
    octetStream os;
    for (int i = 0; i < nparties; i++)
    {
        os.reset_write_head();
        os.Receive(sockets[i]);

        os.decrypt_sequence(&commsec[i].second[0],1);
        os.decrypt(keys[i]);

        for (unsigned int j = 0; j < 3; j++)
        {
            gfp value;
            value.unpack(os);
            output_values[j] += value;            
        }
    }

    if (output_values[0] * output_values[1] != output_values[2])
    {
        cerr << "Unable to authenticate output value as correct, aborting." << endl;
        exit(1);
    }
    return output_values[0];
}


int main(int argc, char** argv)
{
    int my_client_id;
    int nparties;
    int salary_value;
    int finish;
    int port_base = 14000;
    sign_key_container_t sts_key;
    string host_name = "localhost";

    if (argc < 5) {
        cout << "Usage is external-client <client identifier> <number of spdz parties> "
           << "<salary to compare> <finish (0 false, 1 true)> <optional host name, default localhost> "
           << "<optional spdz party port base number, default 14000>" << endl;
        exit(0);
    }

    my_client_id = atoi(argv[1]);
    nparties = atoi(argv[2]);
    salary_value = atoi(argv[3]);
    finish = atoi(argv[4]);
    if (argc > 5)
        host_name = argv[5];
    if (argc > 6)
        port_base = atoi(argv[6]);

    sts_key.server_publickey.resize(nparties);
    for(int i = 0 ; i < nparties; i++) {
        sts_key.server_publickey[i].resize(crypto_sign_PUBLICKEYBYTES);
    }

    // init static gfp
    string prep_data_prefix = get_prep_dir(nparties, 128, 40);
    initialise_fields(prep_data_prefix);

    // Generate session keys to decrypt data sent from each spdz engine (party)
    vector<octet*> session_keys(nparties);
    vector<int> client_public_key_ints(8);

    generate_symmetric_keys(session_keys, client_public_key_ints, &sts_key, prep_data_prefix, my_client_id);

    // Setup connections from this client to each party socket and send the client public keys
    vector<int> sockets(nparties);
    // vector< pair <vector<octet>, vector <octet> > > commseckey(nparties); 
    commsec_t commseckey(nparties);  
    for (int i = 0; i < nparties; i++)
    {
        set_up_client_socket(sockets[i], host_name.c_str(), port_base + i);
        send_public_key(sts_key.client_publickey_ints, sockets[i]);
        send_public_key(client_public_key_ints, sockets[i]);
        commseckey[i] = sts_initiator_role(sts_key, sockets, i);        
    }
    cout << "Finish setup socket connections to SPDZ engines." << endl;

    // Map inputs into gfp 
    vector<gfp> input_values_gfp(3);
    input_values_gfp[0].assign(my_client_id);
    input_values_gfp[1].assign(salary_value);
    input_values_gfp[2].assign(finish);    

    // Send the inputs to the SPDZ Engines
    send_private_inputs(input_values_gfp, sockets, nparties, commseckey, session_keys);
    cout << "Sent private inputs to each SPDZ engine, waiting for result..." << endl;

    // Get the result back
    gfp result = receive_result(sockets, nparties, commseckey, session_keys);

    cout << "Winning client id is : " << result << endl;
    
    for (unsigned int i = 0; i < sockets.size(); i++)
        close_client_socket(sockets[i]);

    return 0;
}
