// (C) 2016 University of Bristol. See License.txt

#include <iostream>
#include <fstream>
#include "Math/gfp.h"

using namespace std;

int main() {
	ifstream cin("gfp_output_vals.in");
	ofstream cout("gfp_output_vals.out");

	gfp::init_field(bigint("172035116406933162231178957667602464769"));

    cout << "Output Values:\n";
    gfp b;
    while (!(cin.peek() == EOF)) {
        b.input(cin, false);
        b.output(cout, true);
        cout << "\n";
    }

	cin.close();	
	cout.close();
	return 0;
}
