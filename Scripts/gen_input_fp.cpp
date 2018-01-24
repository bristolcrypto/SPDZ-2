// (C) 2017 University of Bristol. See License.txt

#include <iostream>
#include <fstream>
#include "Math/gfp.h"
#include "Processor/Buffer.h"

using namespace std;

int main() {
	const char* input_name = "gfp_vals.in";
	const char* output_name = "gfp_vals.out";
	ifstream cin(input_name);
	ofstream cout(output_name);

	gfp::init_field(bigint("172035116406933162231178957667602464769"));

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

	cin.close();	
	cout.close();
	return 0;
}
