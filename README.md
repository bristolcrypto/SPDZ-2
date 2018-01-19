(C) 2017 University of Bristol. See License.txt

Software for the SPDZ and MASCOT secure multi-party computation protocols.
See `Programs/Source/` for some example MPC programs, and `tutorial.md` for
a basic tutorial.

See also https://www.cs.bris.ac.uk/Research/CryptographySecurity/SPDZ

#### Preface:

The software contains some functionality for benchmarking that breaks security. This is deactivated by default. See the compilation section on how to activate it if needed.

In particular, the online phase will discard preprocessed data and crash when it runs out if not in benchmarking mode. In benchmarking mode, it will reuse preprocessed data.

#### Requirements:
 - GCC
 - MPIR library, compiled with C++ support (use flag --enable-cxx when running configure)
 - libsodium library, tested against 1.0.11
 - CPU supporting AES-NI and PCLMUL
 - Python 2.x, ideally with `gmpy` package (for testing)
 - NTL library for the SPDZ-2 offline phase (optional; tested with NTL 9.10)

#### OS X:
 - `g++` might actually refer to clang, in which case you need to change `CONFIG` to use GCC instead.
 - It has been reported that MPIR has to be compiled with GCC for the linking to work:
   ```./configure CC=<path to GCC gcc> CXX=<path to GCC g++> --enable-cxx```

#### To compile SPDZ:

1) Edit `CONFIG` or `CONFIG.mine` to your needs:

 - To benchmark only the online phase (skipping the secure offline phase), add the following line at the top: `MY_CFLAGS = -DINSECURE`
 - `PREP_DIR` should point to should be a local, unversioned directory to store preprocessing data (default is `Player-Data` in the current directory).
 - For the SPDZ-2 offline phase, set `USE_NTL = 1` and `MOD = -DMAX_MOD_SZ=6`.
 - For the MASCOT offline phase or to use GF(2^128) in the online phase, set `USE_GF2N_LONG = 1`.
 - For processors without AVX (e.g., Intel Atom) or for optimization, set `ARCH = -march=<architecture>`.

2) Run make (use the flag -j for faster compilation multiple threads). Remember to run `make clean` first after changing `CONFIG` or `CONFIG.mine`.

#### To setup for benchmarking the online phase

This requires the INSECURE flag to be set before compilation as explained above. For a secure offline phase, see the section on SPDZ-2 below.

Run:

`Scripts/setup-online.sh`

This sets up parameters for the online phase for 2 parties with a 128-bit prime field and 40-bit binary field, and creates fake offline data (multiplication triples etc.) for these parameters.

Parameters can be customised by running

`Scripts/setup-online.sh <nparties> <nbitsp> <nbits2>`


#### To compile a program

To compile the program in `./Programs/Source/tutorial.mpc`, run:

`./compile.py tutorial`

This creates the bytecode and schedule files in Programs/Bytecode/ and Programs/Schedules/

#### To run a program

To run the above program (on one machine), first run:

`./Server.x 2 5000 &`

(or replace `5000` with your desired port number)

Then run both parties' online phase:

`./Player-Online.x -pn 5000 0 tutorial`

`./Player-Online.x -pn 5000 1 tutorial` (in a separate terminal)

Or, you can use a script to do the above automatically:

`Scripts/run-online.sh tutorial`

To run a program on two different machines, firstly the preprocessing data must be
copied across to the second machine (or shared using sshfs), and secondly, Player-Online.x
needs to be passed the machine where Server.x is running.
e.g. if this machine is name `diffie` on the local network:

`./Player-Online.x -pn 5000 -h diffie 0 test_all`

`./Player-Online.x -pn 5000 -h diffie 1 test_all`

#### Compiling and running programs from external directories

Programs can also be edited, compiled and run from any directory with the above basic structure. So for a source file in `./Programs/Source/`, all SPDZ scripts must be run from `./`. The `setup-online.sh` script must also be run from `./` to create the relevant data. For example:

```
spdz$ cd ../
$ mkdir myprogs
$ cd myprogs
$ mkdir -p Programs/Source
$ vi Programs/Source/test.mpc
$ ../spdz/compile.py test.mpc
$ ls Programs/
Bytecode  Public-Input  Schedules  Source
$ ../spdz/Scripts/setup-online.sh
$ ls
Player-Data Programs
$ ../spdz/Scripts/run-online.sh test
```

#### SPDZ-2 offline phase

This implementation is suitable to generate the preprocessed data used in the online phase.

For quick run on one machine, you can call the following:

`./spdz2-offline.x -p 0 & ./spdz2-offline.x -p 1`

More generally, run the following on every machine:

`./spdz2-offline.x -p <number of party> -N <total number of parties> -h <hostname of party 0> -c <covert security parameter>`

The number of parties are counted from 0. As seen in the quick example, you can omit the total number of parties if it is 2 and the hostname if all parties run on the same machine. Invoke `./spdz2-offline.x` for more explanation on the options.

SPDZ-2 provides covert security according to some parameter c (at least 2). A malicious adversary will get caught with probability 1/c. There is a linear correlation between c and the running time, that is, running with 2c takes twice as long as running with c. The default for c is 10.

The program will generate every kind of randomness required by the online phase until you stop it. You can shut it down gracefully pressing Ctrl-c (or sending the interrupt signal `SIGINT`).

#### Benchmarking the MASCOT offline phase

The MASCOT implementation is not suitable to generate the preprocessed data for the online phase because it can only generate either multiplication triples or bits. Nevertheless, an online computation only using data of one kind can run from the output of MASCOT offline phase if `Player-Online.x` is run with the options `-lg2 128 -lgp 128`.

In order to compile the MASCOT code, the following must be set in CONFIG or CONFIG.mine:

`USE_GF2N_LONG = 1`

It also requires SimpleOT:
```
git submodule update --init SimpleOT
cd SimpleOT
make
```

If SPDZ has been built before, any compiled code needs to be removed:

`make clean`

HOSTS must contain the hostnames or IPs of the players, see HOSTS.example for an example.

Then, MASCOT can be run as follows:

`host1:$ ./ot-offline.x -p 0 -c`

`host2:$ ./ot-offline.x -p 1 -c`
