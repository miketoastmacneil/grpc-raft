
# Raftpp

A side project C++ implementation of Raft using GRPC. At the moment
being written in downtime and isn't really suitable for production.

Likely doesn't build or run in fact. 

## Setup
This assumes you've gone through the GRPC  [quick start installation](https://grpc.io/docs/languages/cpp/quickstart/),
and have GRPC and its dependendencies (namely Abseil) installed on your local machine. 
One difference is this repo assumes the environment variable `GRPC_INSTALLATION=<installation-dir>` has set rather than `MY_INSTALL_DIR` recommended by GRPC. 

## Running
At the moment only up to 5 servers can be run and this can be done through a (update script).