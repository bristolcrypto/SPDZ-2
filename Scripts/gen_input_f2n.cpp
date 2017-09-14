// (C) 2017 University of Bristol. See License.txt

#include <iostream>
#include <fstream>
#include "Math/gf2n.h"

using namespace std;

int main() {
	ifstream cin("gf2n_vals.in");
	ofstream cout("gf2n_vals.out");

	gf2n::init_field(40);

	int n; cin >> n;
	for (int i = 0; i < n; ++i) {
		gf2n_short x; cin >> x;
		cerr << "value is: " << x << "\n";
		x.output(cout,false);
	}

	cin.close();
	cout.close();
	return 0;
}