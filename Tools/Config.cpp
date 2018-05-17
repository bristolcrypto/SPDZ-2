// (C) 2018 University of Bristol. See License.txt

// Client key file format:
//      X25519  Public Key
//      X25519  Secret Key
//      Ed25519 Public Key
//      Ed25519 Secret Key
//      Server 1 X25519 Public Key
//      Server 1 Ed25519 Public Key
//      ... 
//      Server N Public Key
//      Server N Ed25519 Public Key
//
// Player key file format:
//      X25519  Public Key
//      X25519  Secret Key
//      Ed25519 Public Key
//      Ed25519 Secret Key
//      Number of clients [64 bit little endian]
//      Client 1 X25519 Public Key
//      Client 1 Ed25519 Public Key
//      ...
//      Client N X25519 Public Key
//      Client N Ed25519 Public Key
//      Number of servers [64 bit little endian]
//      Server 1 X25519 Public Key
//      Server 1 Ed25519 Public Key
//      ...
//      Server N X25519 Public Key
//      Server N Ed25519 Public Key
#include "Tools/octetStream.h"
#include "Networking/Player.h"
#include "Math/gf2n.h"
#include "Config.h"
#include <sodium.h>
#include <vector>
#include <iomanip>

namespace Config {
    class ConfigError : public std::exception
    {
        std::string s;

        public:
        ConfigError(std::string ss) : s(ss) {}
        ~ConfigError() throw () {}
        const char* what() const throw() { return s.c_str(); }
    };

    static void output(const vector<octet> &vec, ofstream &of)
    {
        copy(vec.begin(), vec.end(), ostreambuf_iterator<char>(of));
    }
    void print_vector(const vector<octet> &vec)
    {
        cerr << hex;
        for(size_t i = 0; i < vec.size(); i ++ ) {
            cerr << setfill('0') << setw(2) << (int)vec[i];
        }
        cerr << dec << endl;
    }

    uint64_t getW64le(ifstream &infile)
    {
        uint8_t buf[8];
        uint64_t res=0;
        infile.read((char*)buf,sizeof buf);

        if (!infile.good())
            throw ConfigError("getW64le: could not read from config file");

        for(size_t i = 0; i < sizeof buf ; i ++ ) {
            res |= ((uint64_t)buf[i]) << i*8;
        }

        return res;
    }

    void putW64le(ofstream &outf, uint64_t nr)
    {
        char buf[8];
        for(int i=0;i<8;i++) {
            char byte = (uint8_t)(nr >> (i*8));
            buf[i] = (char)byte;
        }
        outf.write(buf,sizeof buf);
    }

    const string default_player_config_file_prefix = "Player-SPDZ-Keys-P";
    string player_config_file(int player_number)
    {
        stringstream filename;
        filename << default_player_config_file_prefix << player_number;
        return filename.str();
    }

    void read_player_config(string cfgdir,int my_number,vector<public_signing_key> pubkeys,secret_signing_key mykey, public_signing_key mypubkey)
    {
        string filename;
        filename = cfgdir + player_config_file(my_number);
        ifstream infile(filename.c_str(), ios::in | ios::binary);

        infile.seekg(crypto_box_PUBLICKEYBYTES + crypto_box_SECRETKEYBYTES);
        mypubkey.resize(crypto_sign_PUBLICKEYBYTES);
        infile.read((char*)&mypubkey[0], crypto_sign_PUBLICKEYBYTES);
        mykey.resize(crypto_sign_SECRETKEYBYTES);
        infile.read((char*)&mykey[0], crypto_sign_SECRETKEYBYTES);

        // If we've failed by this point, abort. After this point we'll
        // just try to read optional content.
        if (!infile.good()) {
            throw ConfigError("Could not parse player config file.");
        }

        // Deal gracefully with absence of additional key material
        try {
            uint64_t nrClients = getW64le(infile);
            infile.ignore(nrClients * (crypto_sign_PUBLICKEYBYTES + crypto_box_PUBLICKEYBYTES));
            uint64_t nrPlayers = getW64le(infile);
            pubkeys.resize(nrPlayers);
            for(size_t i=0; i<nrPlayers; i++) {
                pubkeys[i].resize(crypto_sign_PUBLICKEYBYTES);
                infile.read((char*)&pubkeys[i][0],pubkeys[i].size());
            }
        } catch (ConfigError& e) {
            pubkeys.resize(0);
        }

        infile.close();
    }
    void write_player_config_file(string config_dir
                           ,int player_number, public_key my_pub, secret_key my_priv
                                             , public_signing_key my_signing_pub, secret_signing_key my_signing_priv
                                             , vector<public_key> client_pubs, vector<public_signing_key> client_signing_pubs
                                             , vector<public_key> player_pubs, vector<public_signing_key> player_signing_pubs)
    {
        stringstream filename;
        filename << config_dir << "Player-SPDZ-Keys-P" << player_number;
        ofstream outf(filename.str().c_str(), ios::out | ios::binary);
        if (outf.fail())
            throw file_error(filename.str().c_str());
        if(crypto_box_PUBLICKEYBYTES != my_pub.size()  ||
           crypto_box_SECRETKEYBYTES != my_priv.size() ||
           crypto_sign_PUBLICKEYBYTES != my_signing_pub.size() ||
           crypto_sign_SECRETKEYBYTES != my_signing_priv.size()) {
            throw "Invalid key sizes";
        } else if(client_pubs.size() != client_signing_pubs.size()) {
            throw "Incorrect number of client keys";
        } else if(player_pubs.size() != player_signing_pubs.size()) {
            throw "Incorrect number of player keys";
        } else {
            for(size_t i = 0; i < client_pubs.size(); i++) {
                if(crypto_box_PUBLICKEYBYTES != client_pubs[i].size() ||
                   crypto_sign_PUBLICKEYBYTES != client_signing_pubs[i].size()) {
                       throw "Incorrect size of client key.";
                   }
            }
            for(size_t i = 0; i < player_pubs.size(); i++) {
                if(crypto_box_PUBLICKEYBYTES != player_pubs[i].size() ||
                   crypto_sign_PUBLICKEYBYTES != player_signing_pubs[i].size()) {
                       throw "Incorrect size of player key.";
                   }
            }
        }
        // Write public and secret X25519 keys
        output(my_pub, outf);
        output(my_priv, outf);
        output(my_signing_pub, outf);
        output(my_signing_priv, outf);

        putW64le(outf, (uint64_t)client_pubs.size());
        // Write all client public keys
        for (size_t j = 0; j < client_pubs.size(); j++) {
            output(client_pubs[j], outf);
            output(client_signing_pubs[j], outf);
        }
        putW64le(outf, (uint64_t)player_pubs.size());
        for (size_t j = 0; j < player_pubs.size(); j++) {
            output(player_pubs[j], outf);
            output(player_signing_pubs[j], outf);
        }
        outf.flush();
        outf.close();
    }
}
