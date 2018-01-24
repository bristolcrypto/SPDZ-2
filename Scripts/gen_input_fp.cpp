// (C) 2017 University of Bristol. See License.txt

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
    opt.parse(argc, argv);
    int nparties, lgp, lg2;
    opt.get("-N")->getInt(nparties);
    opt.get("-lgp")->getInt(lgp);
    opt.get("-lg2")->getInt(lg2);
    read_setup(nparties, lgp, lg2);

	const char* input_name = "gfp_vals.in";
	const char* output_name = "gfp_vals.out";
	ifstream cin(input_name);
	ofstream cout(output_name);

	int n; cin >> n;
	for (int i = 0; i < n; ++i) {
		bigint a;
		cin >> a;
		gfp b;
		to_gfp(b, a);
		b.output(cout, false);
	}
	if (cin.fail())
	{
		cout.close();
		unlink(output_name);
		throw runtime_error("Failed to read " + string(input_name));
	}

	n = -(n % BUFFER_SIZE) + BUFFER_SIZE;
	cerr << "Adding " << n << " zeros to match buffer size" << endl;
	for (int i = 0; i < n; i++)
		gfp(0).output(cout, false);
	cerr << "Output written to " << output_name
			<< ", copy to Player-Data/Private-Input-<playerno>" << endl;

	cin.close();	
	cout.close();
	return 0;
}
