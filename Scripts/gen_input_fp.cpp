// (C) 2017 University of Bristol. See License.txt

#include <iostream>
#include <fstream>
#include "Math/gfp.h"
#include "Processor/Buffer.h"

using namespace std;

int main() {
	ifstream cin("gfp_vals.in");
	ofstream cout("gfp_vals.out");

	gfp::init_field(bigint("172035116406933162231178957667602464769"));

	int n; cin >> n;
	for (int i = 0; i < n; ++i) {
		bigint a;
		cin >> a;
		gfp b;
		to_gfp(b, a);
		b.output(cout, false);
	}
	n = -(n % BUFFER_SIZE) + BUFFER_SIZE;
	cerr << "Adding " << n << " zeros to match buffer size" << endl;
	for (int i = 0; i < n; i++)
		gfp(0).output(cout, false);

	cin.close();	
	cout.close();
	return 0;
}
