// (C) 2018 University of Bristol. See License.txt

#include "Tools/octetStream.h"
#include "Networking/Player.h"
#include <sodium.h>
namespace Config {
    typedef vector<octet> public_key;
    typedef vector<octet> public_signing_key;
    typedef vector<octet> secret_key;
    typedef vector<octet> secret_signing_key;
    void read_player_config(string cfgdir,int my_number,vector<public_signing_key> pubkeys,secret_signing_key mysecretkey, public_signing_key mypubkey);
    void write_player_config_file(string config_dir
                           ,int player_number, public_key my_pub, secret_key my_priv
                                             , public_signing_key my_signing_pub, secret_signing_key my_signing_priv
                                             , vector<public_key> client_pubs, vector<public_signing_key> client_signing_pubs
                                             , vector<public_key> player_pubs, vector<public_signing_key> player_signing_pubs);
    uint64_t getW64le(ifstream &infile);
    void putW64le(ofstream &outf, uint64_t nr);
    extern const string default_player_config_file_prefix;
    string player_config_file(int player_number);
    void print_vector(const vector<octet> &vec);
}
