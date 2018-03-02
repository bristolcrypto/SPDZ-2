// (C) 2018 University of Bristol. See License.txt

// Preprocessing stage to:
// Create the public/private key pairs for each client
// Create the public/private key pairs for each spdz engine
// For each client store the client keys + all spdz engine public keys 
//  in a file named Client-Keys-C<client id>
// For each spdz engine store the spdz engine keys + all client public keys
//  in a file named Player-SPDZ-Keys-P<player id>
//

#include <sodium.h>

#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/Share.h"
#include "Math/Setup.h"
#include "Auth/fake-stuff.h"
#include "Exceptions/Exceptions.h"

#include "Math/Setup.h"
#include "Processor/Data_Files.h"
#include "Tools/mkpath.h"
#include "Tools/ezOptionParser.h"
#include "Tools/Config.h"

#include <sstream>
#include <fstream>
using namespace std;

static void output(const vector<octet> &vec, ofstream &of)
{
    copy(vec.begin(), vec.end(), ostreambuf_iterator<char>(of));
}

int main(int argc, const char** argv)
{
    ez::ezOptionParser opt;

    opt.syntax = "./client-setup.x <nplayers> [OPTIONS]\n";

    opt.add(
          "0", // Default.
          0, // Required?
          1, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Number of external clients (default: nplayers)", // Help description.
          "-nc", // Flag token.
          "--numclients" // Flag token.
    );
    opt.add(
          "128", // Default.
          0, // Required?
          1, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Bit length of GF(p) field (default: 128)", // Help description.
          "-lgp", // Flag token.
          "--lgp" // Flag token.
    );
    opt.add(
          "40", // Default.
          0, // Required?
          1, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Bit length of GF(2^n) field (default: 40)", // Help description.
          "-lg2", // Flag token.
          "--lg2" // Flag token.
    );
    opt.parse(argc, argv);

    string prep_data_prefix;

    string usage;
    
    int nplayers;
    if (opt.firstArgs.size() == 2)
    {
        nplayers = atoi(opt.firstArgs[1]->c_str());
    }
    else if (opt.lastArgs.size() == 1)
    {
        nplayers = atoi(opt.lastArgs[0]->c_str());
    }
    else
    {
        cerr << "ERROR: invalid number of arguments\n";
        opt.getUsage(usage);
        cout << usage;
        return 1;
    }

    int lg2, lgp, nclients;
    opt.get("--numclients")->getInt(nclients);
    if (nclients <= 0) 
        nclients = nplayers;
    opt.get("--lgp")->getInt(lgp);
    opt.get("--lg2")->getInt(lg2);

    cout << "nplayers = " << nplayers << endl;
    cout << "nclients = " << nclients << endl;
    cout << "lgp = " << lgp << endl;
    cout << "lgp2 = " << lg2 << endl;

    prep_data_prefix = get_prep_dir(nplayers, lgp, lg2);
    cout << "prep dir = " << prep_data_prefix << endl;

    vector<Config::public_key> client_publickeys;
    vector<Config::secret_key> client_secretkeys;
    client_publickeys.resize(nclients);
    client_secretkeys.resize(nclients);
    for (int i = 0; i < nclients; i++) {
        client_secretkeys[i].resize(crypto_box_SECRETKEYBYTES);
        client_publickeys[i].resize(crypto_box_PUBLICKEYBYTES);
        randombytes_buf(&client_secretkeys[i][0], client_secretkeys[i].size());
        crypto_scalarmult_base(&client_publickeys[i][0], &client_secretkeys[i][0]);
    }

    vector<Config::public_signing_key> client_signing_publickeys;
    vector<Config::secret_signing_key> client_signing_secretkeys;
    client_signing_publickeys.resize(nclients);
    client_signing_secretkeys.resize(nclients);
    for (int i = 0; i < nclients; i++) {
        client_signing_publickeys[i].resize(crypto_sign_PUBLICKEYBYTES);
        client_signing_secretkeys[i].resize(crypto_sign_SECRETKEYBYTES);
        crypto_sign_keypair(&client_signing_publickeys[i][0], &client_signing_secretkeys[i][0]);
    }

    vector<Config::public_key> server_publickeys;
    vector<Config::secret_key> server_secretkeys;
    server_publickeys.resize(nplayers);
    server_secretkeys.resize(nplayers);
    for (int i = 0; i < nplayers; i++) {
        server_publickeys[i].resize(crypto_box_PUBLICKEYBYTES);
        server_secretkeys[i].resize(crypto_box_SECRETKEYBYTES);
        randombytes_buf(&server_secretkeys[i][0], server_secretkeys[i].size());
        crypto_scalarmult_base(&server_publickeys[i][0], &server_secretkeys[i][0]);
    }
    vector<Config::public_signing_key> server_signing_publickeys;
    vector<Config::secret_signing_key> server_signing_secretkeys;
    server_signing_publickeys.resize(nplayers);
    server_signing_secretkeys.resize(nplayers);
    for (int i = 0; i < nplayers; i++) {
        server_signing_publickeys[i].resize(crypto_sign_PUBLICKEYBYTES);
        server_signing_secretkeys[i].resize(crypto_sign_SECRETKEYBYTES);
        crypto_sign_keypair(&server_signing_publickeys[i][0], &server_signing_secretkeys[i][0]);
    }

    /* Write client files */
    for (int i = 0; i < nclients; i++) {
        stringstream filename;
        filename << prep_data_prefix << "Client-Keys-C" << i;
        ofstream outf(filename.str().c_str());
        if (outf.fail())
            throw file_error(filename.str().c_str());
        // Write public key and secret key
        output(client_publickeys[i],outf);
        output(client_secretkeys[i],outf);
        output(client_signing_publickeys[i],outf);
        output(client_signing_secretkeys[i],outf);
        int keycount = 2;

        // Write all spdz engine public keys
        for (int j = 0; j < nplayers; j++) {
            output(server_publickeys[j], outf);
            output(server_signing_publickeys[j], outf);
            keycount++;
        }
        outf.close();
        cout << "Wrote " << keycount << " keys to " << filename.str() << endl;
    }

    /* Write spdz engine files */
    for (int i = 0; i < nplayers; i++) {
        Config::write_player_config_file( prep_data_prefix, i
                                        , server_publickeys[i], server_secretkeys[i]
                                        , server_signing_publickeys[i], server_signing_secretkeys[i]
                                        , client_publickeys, client_signing_publickeys
                                        , server_publickeys, server_signing_publickeys);
    }
}
