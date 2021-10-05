# tricksbn_demo

Work-in-progress project repo to show tricksbn in an end-to-end demonstration. Intended to be a standalone project from which we can learn how tricksbn works, fix tricksbn bugs, and document it so future users don't have to start from source code...

## Project About & Status
This repo contains a CFS project that was adapted from the tvsio_app demonstration. The CFS project has a "temperature monitor" application which just listens for a "telemetry" message from sim containing temperature variable and prints out the contents of the message. 

The temp mon app is also supposed to send out a "command" reset message intended to be picked up by the sim, which will reset the temperature variable in the sim, this wasn't implemented/tested yet in tricksbn demo. 

tricksbn repo is included in this repo as a submodule called trick_sims which points to a branch in tricksbn called demo_dev

Currently only a single 1-byte variable is correctly being sent by tricksbn to cfs. There appears to be bugs in tricksbn preventing a more substantial demo from working correctly. 

See the powerpoint tricksbn_notes for my very rough notes as I tried to figure out how this software is intended to work. 

## Build & Run
### CFS Temperature Monitor
In the project root, `make install` on a machine with the necessary dependencies/config for building/running CFS. `cd build/exe/cpu1` and `./core-cpu1`
### Trick Temperature Sim
On the same machine `cd trick_sims/SIM_temp`, `source setenv.sh` (adjust setenv as needed, note that tricksbn unfortunately has a dependency on Qt5), `trick-CP`, `./S_main* RUN_temp/input.py`

In the present state of the repo, a single byte temperature variable should be sent to CFS and printed out by the temp monitor app, along with probably a lot of debug messages (sorry)
