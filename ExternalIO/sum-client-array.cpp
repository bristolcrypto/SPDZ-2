/*
 * Demonstrate external client inputing and receiving outputs from a SPDZ process, 
 * following the protocol described in https://eprint.iacr.org/2015/1006.pdf.
 *
 * Provides a client to sum.mpc program to calculate the sum of clients inputs.
 *
 * Each connecting client:
 * - sends a unique id to identify the client
 * - sends an integer input (value to add)
 * - sends an integer (0 meaining more players will join this round or 1 meaning stop the round and calc the result).
 *
 * The result is returned authenticated with a share of a random value:
 * - share of winning unique id [y]
 * - share of random value [r]
 * - share of winning unique id * random value [w]
 *   winning unique id is valid if ∑ [y] * ∑ [r] = ∑ [w]
 * 
 * No communications security is used. 
 *
 * To run with 2 parties / SPDZ engines:
 *   ./Scripts/setup-online.sh to create triple shares for each party (spdz engine).
 *   ./compile.py sum-array
 *   ./Scripts/run-online.sh sum-array to run the engines.
 *
 *   ./sum-client-array.x 123 2 ./ExternalIO/data1.txt 0
 *   ./sum-client-array.x 456 2 ./ExternalIO/data2.txt 0
 *   ./sum-client-array.x 789 2 ./ExternalIO/data1.txt 1
 *
 *   Expect the sum to be 800.
 */

#include "Math/gfp.h"
#include "Math/gf2n.h"
#include "Networking/sockets.h"
#include "Tools/int.h"
#include "Math/Setup.h"
#include "Auth/fake-stuff.h"

#include <sodium.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <fstream>

#define ARRAY_UPPER_BOUND 100

// Send the private inputs masked with a random value.
// Receive shares of a preprocessed triple from each SPDZ engine, combine and check the triples are valid.
// Add the private input value to triple[0] and send to each spdz engine.
void send_private_inputs(vector<gfp>& values, vector<int>& sockets, int nparties)
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

        for (int j = 0; j < num_inputs; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                triple_shares[k].unpack(os);
                triples[j][k] += triple_shares[k];
            }
        }
    }

    // Check triple relations (is a party cheating?)
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
        os.Send(sockets[j]);
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


// Receive shares of the result and sum together.
// Also receive authenticating values.
gfp receive_result(vector<int>& sockets, int nparties)
{
    vector<gfp> output_values(3);
    octetStream os;
    for (int i = 0; i < nparties; i++)
    {
        os.reset_write_head();
        os.Receive(sockets[i]);
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
    int finish;
    int port_base = 14000;
    string host_name = "localhost";

    if (argc < 5) {
        cout << "Usage is sum-client <client identifier> <number of spdz parties> "
           << "<number to add> <finish (0 false, 1 true)> <optional host name, default localhost> "
           << "<optional spdz party port base number, default 14000>" << endl;
        exit(0);
    }

    my_client_id = atoi(argv[1]);
    nparties = atoi(argv[2]);
    string fileName(argv[3]);
    finish = atoi(argv[4]);
    if (argc > 5)
        host_name = argv[5];
    if (argc > 6)
        port_base = atoi(argv[6]);
        
    std::vector<int> array_to_add;
    std::cout << "Reading " << fileName << std::endl;
    ifstream inputFile(fileName);

    if (inputFile) { // check if file is open
        int value;
        while (inputFile >> value) {
            array_to_add.push_back(value);
        }
    }
    inputFile.close();
    std::cout << "Read " << array_to_add.size() << " numbers." << std::endl;

    // init static gfp
    string prep_data_prefix = get_prep_dir(nparties, 128, 40);
    initialise_fields(prep_data_prefix);

    // Setup connections from this client to each party socket
    vector<int> sockets(nparties);
    for (int i = 0; i < nparties; i++)
    {
        set_up_client_socket(sockets[i], host_name.c_str(), port_base + i);
    }
    cout << "Finish setup socket connections to SPDZ engines." << endl;

    // Map inputs into gfp 
    int vec_size = array_to_add.size();
    vector<gfp> vector_size_gfp(2);
    vector_size_gfp[0].assign(my_client_id);
    vector_size_gfp[1].assign(vec_size);
    send_private_inputs(vector_size_gfp, sockets, nparties); // first send the client_id and the array-size
        
    // Create gfp vector for the array and send one int at a time
    vector<gfp> input_values_gfp(1);
    input_values_gfp[0].assign(my_client_id); // send client_id one more time
    send_private_inputs(input_values_gfp, sockets, nparties);
    for (int i = 0 ; i < vec_size ; ++i) {
        input_values_gfp[0].assign(array_to_add[i]);
        send_private_inputs(input_values_gfp, sockets, nparties);
    }
    input_values_gfp[0].assign(finish);
    send_private_inputs(input_values_gfp, sockets, nparties);
    cout << "Sent private inputs to each SPDZ engine, waiting for result..." << endl;
    
    // Run the commputation & get the result back
    vector<gfp> results(ARRAY_UPPER_BOUND);
    for (int i = 0 ; i < ARRAY_UPPER_BOUND ; ++i) {
        results[i] = receive_result(sockets, nparties);
    }
    
    cout << "\nArray with sum of clients arrays is :\n";
    for (int i = 1 ; i <= vec_size ; ++i) { // starting from 1 because the first is the client_id
        cout << results[i]  << " ";
    }
    cout << endl;
    
    for (unsigned int i = 0; i < sockets.size(); i++) {
        close_client_socket(sockets[i]);
    }

    return 0;
}
