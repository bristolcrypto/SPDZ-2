// (C) 2017 University of Bristol. See License.txt

#include "Processor/Machine.h"
#include "Math/Setup.h"
#include "Tools/ezOptionParser.h"
#include "Tools/Config.h"

#include <iostream>
#include <map>
#include <string>
#include <stdio.h>
using namespace std;

int main(int argc, const char** argv)
{
    ez::ezOptionParser opt;

    opt.syntax = "./Player-Online.x [OPTIONS] <playernum> <progname>\n";
    opt.example = "./Player-Online.x -lgp 64 -lg2 128 -m new 0 sample-prog\n./Player-Online.x -pn 13000 -h localhost 1 sample-prog\n";

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
    opt.add(
          "5000", // Default.
          0, // Required?
          1, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Port number base to attempt to start connections from (default: 5000)", // Help description.
          "-pn", // Flag token.
          "--portnumbase" // Flag token.
    );
    opt.add(
          "localhost", // Default.
          0, // Required?
          1, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Host where Server.x is running (default: localhost)", // Help description.
          "-h", // Flag token.
          "--hostname" // Flag token.
    );
    opt.add(
          "empty", // Default.
          0, // Required?
          1, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Where to obtain memory, new|old|empty (default: empty)\n\t"
            "new: copy from Player-Memory-P<i> file\n\t"
            "old: reuse previous memory in Memory-P<i>\n\t"
            "empty: create new empty memory", // Help description.
          "-m", // Flag token.
          "--memory" // Flag token.
    );
    opt.add(
          "", // Default.
          0, // Required?
          0, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Direct communication instead of star-shaped", // Help description.
          "-d", // Flag token.
          "--direct" // Flag token.
    );
    opt.add(
          "", // Default.
          0, // Required?
          0, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Star-shaped communication handled by background threads", // Help description.
          "-P", // Flag token.
          "--parallel" // Flag token.
    );
    opt.add(
          "0", // Default.
          0, // Required?
          1, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Sum at most n shares at once when using indirect communication", // Help description.
          "-s", // Flag token.
          "--opening-sum" // Flag token.
    );
    opt.add(
          "", // Default.
          0, // Required?
          0, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Use player-specific threads for communication", // Help description.
          "-t", // Flag token.
          "--threads" // Flag token.
    );
    opt.add(
          "0", // Default.
          0, // Required?
          1, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Maximum number of parties to send to at once", // Help description.
          "-b", // Flag token.
          "--max-broadcast" // Flag token.
    );
    opt.add(
          "0", // Default.
          0, // Required?
          0, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Use communications security between SPDZ players", // Help description.
          "-c", // Flag token.
          "--player-to-player-commsec" // Flag token.
    );

    opt.parse(argc, argv);

    vector<string*> allArgs(opt.firstArgs);
    allArgs.insert(allArgs.end(), opt.lastArgs.begin(), opt.lastArgs.end());
    string progname;
    int playerno;
    string usage;
    vector<string> badOptions;
    unsigned int i;

    if (allArgs.size() != 3)
    {
        cerr << "ERROR: incorrect number of arguments to Player-Online.x\n";
        cerr << "Arguments given were:\n";
        for (unsigned int j = 1; j < allArgs.size(); j++)
            cout << "'" << *allArgs[j] << "'" << endl;
        opt.getUsage(usage);
        cout << usage;
        return 1;
    }
    else
    {
        playerno = atoi(allArgs[1]->c_str());
        progname = *allArgs[2];

    }

    if(!opt.gotRequired(badOptions))
    {
      for (i=0; i < badOptions.size(); ++i)
        cerr << "ERROR: Missing required option " << badOptions[i] << ".";
      opt.getUsage(usage);
      cout << usage;
      return 1;
    }

    if(!opt.gotExpected(badOptions))
    {
      for(i=0; i < badOptions.size(); ++i)
        cerr << "ERROR: Got unexpected number of arguments for option " << badOptions[i] << ".";
      opt.getUsage(usage);
      cout << usage;
      return 1;
    }

    string memtype, hostname;
    int lg2, lgp, pnbase, opening_sum, max_broadcast;
    int p2pcommsec;

    opt.get("--portnumbase")->getInt(pnbase);
    opt.get("--lgp")->getInt(lgp);
    opt.get("--lg2")->getInt(lg2);
    opt.get("--memory")->getString(memtype);
    opt.get("--hostname")->getString(hostname);
    opt.get("--opening-sum")->getInt(opening_sum);
    opt.get("--max-broadcast")->getInt(max_broadcast);
    opt.get("--player-to-player-commsec")->getInt(p2pcommsec);


    int mynum;
    sscanf((*allArgs[1]).c_str(), "%d", &mynum);

    CommsecKeysPackage *keys = NULL;
    if(p2pcommsec) {
        vector<public_signing_key> pubkeys;
        secret_signing_key mykey;
        public_signing_key mypublickey;
        string prep_data_prefix = get_prep_dir(2, lgp, lg2);
        Config::read_player_config(prep_data_prefix,mynum,pubkeys,mykey,mypublickey);
        keys = new CommsecKeysPackage(pubkeys,mykey,mypublickey);
    }
    
    Machine(playerno, pnbase, hostname, progname, memtype, lgp, lg2,
      opt.get("--direct")->isSet, opening_sum, opt.get("--parallel")->isSet,
      opt.get("--threads")->isSet, max_broadcast, keys).run();

    cerr << "Command line:";
    for (int i = 0; i < argc; i++)
      cerr << " " << argv[i];
    cerr << endl;
}


