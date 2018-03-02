// (C) 2018 University of Bristol. See License.txt

#include <iostream>
#include <fstream>
#include "Math/gfp.h"
#include "Processor/Buffer.h"
#include "Tools/ezOptionParser.h"
#include "Math/Setup.h"

using namespace std;

int main(int argc, const char** argv) {
    ez::ezOptionParser opt;
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
        "2", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Number of parties (default: 2).", // Help description.
        "-N", // Flag token.
        "--nparties" // Flag token.
    );

    opt.add(
            "gfp_vals.in", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "Input file (default: ./gfp_vals.in). Use \"-\" for STDIN.", // Help description.
            "-i", // Flag token.
            "--input" // Flag token.
	);

    opt.add(
                "gfp_vals.out", // Default.
                0, // Required?
                1, // Number of args expected.
                0, // Delimiter if expecting multiple args.
                "Output file (default: ./gfp_vals.out). Use \"-\" for STDOUT.", // Help description.
                "-o", // Flag token.
                "--output" // Flag token.
    	);
    opt.parse(argc, argv);
    int nparties, lgp, lg2;
    opt.get("-N")->getInt(nparties);
    opt.get("-lgp")->getInt(lgp);
    opt.get("-lg2")->getInt(lg2);
    read_setup(nparties, lgp, lg2);

	std::string input_name, output_name;
	bool use_stdin = false;
	bool use_stdout = false;
	istream* in;
	ostream* out;

	opt.get("-i")->getString(input_name);
	opt.get("-o")->getString(output_name);

	// Input Stream
	if (input_name == "-"){ // Open file or STDIN
		use_stdin = true;
		in = &cin;
	} else {
		in = new ifstream(input_name.c_str());
	}
	// Output Stream
	if (output_name == "-"){ // Open file or STDOUT
		use_stdout = true;
		out = &cout;
	} else {
		out = new ofstream(output_name.c_str());
	}

	int n; *in >> n;
	for (int i = 0; i < n; ++i) {
		bigint a;
		*in >> a;
		gfp b;
		to_gfp(b, a);
		b.output(*out, false);
	}
	if (in->fail())
	{
		if(!use_stdout) {
			((ofstream*)out)->close();
			delete out;
		}
		unlink(output_name.c_str());
		throw runtime_error("Failed to read input \"" + string(input_name)+"\"");
	}

	n = -(n % BUFFER_SIZE) + BUFFER_SIZE;
	cerr << "Adding " << n << " zeros to match buffer size" << endl;
	for (int i = 0; i < n; i++)
		gfp(0).output(*out, false);
	cerr << "Output written to " << output_name
			<< ", copy to Player-Data/Private-Input-<playerno>" << endl;

	// Clean up file streams.
	if(!use_stdin) {
		((ifstream*)in)->close();
		delete in;
	}
	if(!use_stdout) {
		((ofstream*)out)->close();
		delete out;
	}
	return 0;
}
