# Concretely efficient Private Set Union via Circuit PSI

This is the accompanying codebase for the paper "Concretely efficient Private Set Union via Circuit PSI".

We use:
- SilentOT from https://github.com/osu-crypto/libOTe
- Circuit and boolean logic frameworks from https://github.com/ladnir/cryptoTools
- MP-OPRF and application framework from https://github.com/dujiajun/PSU
- circuit-PSI from https://github.com/Visa-Research/volepsi
- secure-join by Peceny et al. https://eprint.iacr.org/2024/547.pdf https://github.com/ladnir/secure-join

## Requirements
Our tool is tested on Ubuntu 22.04.
The following tools need to be installed prior to setup:

- CMake 3.19 >=
- Make 4.3 >=
- g++/gcc 11.4 >=
- ld 2.38 >=
- OpenSSL 3.0.2 >=


## Installation

Install volePSI and secure-join and its dependencies via the following script.

```bash setup.sh```

Also install BOOSTs necessary extensions.

```sudo apt install libboost-filesystem-dev libboost-thread-dev libboost-regex-dev```

Then compile this codebase.

```bash build.sh```

The resulting binary can be found in the build folder.

## Building with Docker

A docker file is appended in the project. To build run:

```docker build -t circuitPSU:1.0 . ```

## Usage

To execute locally use:
``` ./test_psu -r 20 -s 20 ```
to perform private set union on 2^20 elements for each party.

Over networks use:
``` ./test_psu -u <1/2> -r 20 -s 20 --host <ip>```

such that party 1 is the sender and party 2 the receiver, ```<ip>``` is the ip of the receiver.

## Note

The following code is given without guarantees towards correctness or safety/security, and is intended for research purposes.
Use with caution.
