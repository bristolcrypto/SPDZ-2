The changelog explains changes pulled through from the private development repository. Bug fixes and small enchancements are committed between releases and not documented here.

## 0.0.3 (Mar 2, 2018)

- Added offline phases based on homomorphic encryption, used in the [SPDZ-2 paper](https://eprint.iacr.org/2012/642) and the [Overdrive paper](https://eprint.iacr.org/2017/1230).
- On macOS, the minimum requirement is now Sierra.
- Compilation with LLVM/clang is now possible (tested with 3.8).

## 0.0.2 (Sep 13, 2017)

### Support sockets based external client input and output to a SPDZ MPC program.

See the [ExternalIO directory](./ExternalIO/README.md) for more details and examples.

Note that [libsodium](https://download.libsodium.org/doc/) is now a dependency on the SPDZ build. 

Added compiler instructions:

* LISTEN
* ACCEPTCLIENTCONNECTION
* CONNECTIPV4
* WRITESOCKETSHARE
* WRITESOCKETINT

Removed instructions:

* OPENSOCKET
* CLOSESOCKET
 
Modified instructions:

* READSOCKETC
* READSOCKETS
* READSOCKETINT
* WRITESOCKETC
* WRITESOCKETS

Support secure external client input and output with new instructions:

* READCLIENTPUBLICKEY
* INITSECURESOCKET
* RESPSECURESOCKET

### Read/Write secret shares to disk to support persistence in a SPDZ MPC program.

Added compiler instructions:

* READFILESHARE
* WRITEFILESHARE

### Other instructions

Added compiler instructions:

* DIGESTC - Clear truncated hash computation
* PRINTINT - Print register value

## 0.0.1 (Sep 2, 2016)

### Initial Release

* See `README.md` and `tutorial.md`.
