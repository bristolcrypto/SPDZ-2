(C) 2018 University of Bristol. See License.txt

Suppose we want to add 2 integers mod p in clear, where p has 128 bits and compute over 2 parties inputs: P0, P1.

First create a file named "addition.mpc" in Programs/Source/ folder containing the following:


Computation on Clear Data
==================

```
a = cint(2)
b = cint(10)

c = a + b
print_ln('Result is %s', c)
```

Next step is to transform the file into bytecode which will be run later on the VM between different parties.
For that, type in terminal:

```
./compile -p 128 addition

```

The command will output to stderr the number of registers, rounds and other parameters used for measuring the requirements from the offline phase.

To run the program, you first need to set up the parameters of the different players. This can be done with the following command.

```
sh Scripts/setup-online.sh
```

To simply run the program between 2 parties simulated locally, type in terminal:

```
sh Scripts/run-online.sh addition

```

The output will be a summary for the online phase including the result of the computation.

````
	Result is 12

````


Computation on Secret Shared data
=================================

```
a = sint(2)
b = sint(10)

c = a + b
print_ln('Result is %s', c.reveal())

```

This means that a = a_0 + a_1, b = b_0 + b_1 where a_i belongs to party P_i. 

It's that simple! When we reveal a secret register, the MAC-checking - according
to SPDZ online phase - is done automatically. If there are multiple calls to
reveal() then the rounds of communication are merged automatically by the
compiler.

Remember that a and b are hard-coded constants so the data is shared by one
party having the actualy inputs (a_0=2, b_0=10) where the other one has (a_1 =
0, b_1 = 0). Usually it's easier to debug when things are written in this way.

If we want to run a real MPC computation - P1 shares a and P2 shares b - and
reveal the sum of the values then we can write the following.

```
a = sint.get_raw_input_from(0)
b = sint.get_raw_input_from(1)

c = a + b
print_ln('Result is %s', c.reveal())

```

SPDZ also supports multiplication, division (in a prime field) as well as
GF(2^n) data types. All types can be seen in types.py file.


Array Lookup
=============

Suppose party P0 inputs an array of fixed length n: A[1]...A[n] and party P1
inputs a SS index [index].

```
A = [sint.get_raw_input_from(0) for _ in range(100)]
index = sint.get_raw_input_from(1)
```

Let's see how can we obtain [A[i]].

The standard way to solve this task in a non-MPC way is the following:

```
clear_index = index.reveal()
print_ln('%s', A[clear_index].reveal())

```

But this solution reveals party P1's input which isn't secure!

Another way of doing this is:inary form. There is 

```
accumulator = 0
for i in range(len(A)):
	accumulator = accumulator + (i == index) * A[i]

print_ln('%s', accumulator.reveal())

```

In this case the only revealed value is A[index]. The main trick was that
comparison between sint() and clear data returns a sint(). In conclusion, all
data is masked except the last step.

Providing input to SPDZ
========================

Looking back at the addition example, in Player-Data/Private-Input-{i} for i in
{0,1} we have to provide one integer in each file. Because the conversion
between human readable form to SPDZ data types is time expensive, we have to
feed the numbers in binary form. There is a script for that: gen_input_fp.cpp
and gen_input_f2n.cpp in the Scripts directory designed for generating gfp
inputs. The executables can be found after compiling SPDZ. Customizing those
should be straightforward. Make sure you copy the output files to Player-Data
/Private-Input-{i} files.

There is a sockets interface to provide input and output from external client processes. See the [ExternalIO directory](./ExternalIO/README.md).

Other examples
==============

aes.mpc has the AES evaluation in MPC where the key belongs to party 0
and the message to party 1.

For example, say that we have a key equal to

```
2b7e151628aed2a6abf7158809cf4f3c
```

We have to convert this into SPDZ datatypes so we use the scripts
gen_input_f2n.x which can be found in the main directory after compiling
SPDZ. Next, the file 'gf2n_vals.in' should contain the following:

```
16
0x2b 0x7e 0x15 0x16 0x28 0xae 0xd2 0xa6 0xab 0xf7 0x15 0x88 0x9 0xcf 0x4f 0x3c
```

Now execute the gen_input_f2n.x script and copy the output file to
Player-Data/Private-Input-0

The same method applies to generate the message as an input to SPDZ.

Since all items are in place we can run the online phase and see the
revealed encryption result:
