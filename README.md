NS-3 For Evaluating INT Signals for Congestion Control
==================
This repo is a copy of ns-3.36 with changes to support INT signaling, evaluation of [R+](https://github.com/smcclure20/rplus) CCAs.

# Simulator Instructions
For basic operation of the simulator (building, configuring, etc.), refer to the ns instructions [below](#the-network-simulator-version-3).

Note: it is recommended to configure the build to ignore examples unless you are planning to use them (`./ns3 configure --disable-examples`)

# Requirements
`./ns3` requires `cmake` and `g++`

The R+ support also has the additional requirements:
`protobuf-compiler libboost-all-dev`

# Running R+ Outputs

## Individual simulations
R+ simulations are run with the `single-link-v2.cc` script in the `scratch/` dir. All arguments can be seen in the code, but as an example, one may run it with:

`./ns3 run rplus-sim/single-link-v2.cc -- --cca=ns3::TcpRemy --netfile=/path/to/config-default.cfg --samplesize=1 --whiskerfile=/path/to/rplus/results/cfg-default-results-dir/queue/cfg-default-intq.4 --configruns=10 --intenabled=True --linkintutil=True --linkinterval=10 --delaycoef=1 --tputcoef=1 --byteswitched=False --simtime=11.0 --savewhiskerstats=False --seed=1`

### Notes:
- The script is usually run with `samplesize=1` so that only one (seeded) configuration is run each time. To run the same configuration multiple times, use the `configruns` parameter.
- `intenabled` makes it so that nodes add/edit the INT header (individual signals do not need to be enabled/disabled)

# Running many simulations in parallel
To run many simulations in parallel with different parameters, you can use the [sem](https://simulationexecutionmanager.readthedocs.io/en/develop/) python package. Python scripts using this package can be found in `scripts/`. 

As an example, you may run `rplus-campaign-mg-mt.py` as follows:

`python3 rplus-campaign-mg-mt.py -c remy -t "int,intq,intl,noint" -g "0,1,2,3,4" -w cfg-default-results-dir -cf config-default.cfg -s optional-suffix-to-result-filenames` 

This will run 200 simulations of the R+ outputs in the directory with the name with the name `cfg-default-results-dir ` for each selected generation (0-4) and each signal set listed with the network config `config-default.cfg` (each signal set is expected to have its own named subdirectory). 

### Notes:
- The script expects there to be a single directory with all R+ outputs to test. Within this, there should be a subdir for each signal set that contains all generations. 
  - Example (for the above example command):
  ```
    cfg-default-results-dir/
      -- /int/
        -- cfg-default-int.0
        -- cfg-default-int.1
        -- ...
      -- /intq/
        -- cfg-default-int.0
        -- cfg-default-int.1
        -- ...
      -- ...
  ```
  
- You *must* set some file paths at the top of the script for your environment.

## Main Changes
Here, we note the main modifications over baseline ns3 for easier changes to our work:
- `src/internet/model/tcp-remy.cc` implements R+ outputs as a CCA
  - `src/internet/model/remy` contains additional files necessary for support
- `src/internet/model/tcp-socket-base.cc` and `src/internet/model/tcp-socket-state.cc` were modified slightly to support INT signaling (and allowing the CCA to access it)
- `src/network/model/int-packet-tag.cc` adds a tag that hosts can use to record INT metadata of a packet even after the INT header is stripped
- `src/point-to-point/model/int-header.cc` implements the INT header
- `src/point-to-point-net-device/model/point-to-point-net-device.cc` contains the code for how a node (switch) can read and update the INT header of a packet
- `scratch` contains the top-level simulator script as well as some additional utils



The Network Simulator, Version 3
================================

## Table of Contents:

1) [An overview](#an-open-source-project)
2) [Building ns-3](#building-ns-3)
3) [Running ns-3](#running-ns-3)
4) [Getting access to the ns-3 documentation](#getting-access-to-the-ns-3-documentation)
5) [Working with the development version of ns-3](#working-with-the-development-version-of-ns-3)

Note:  Much more substantial information about ns-3 can be found at
https://www.nsnam.org

## An Open Source project

ns-3 is a free open source project aiming to build a discrete-event
network simulator targeted for simulation research and education.
This is a collaborative project; we hope that
the missing pieces of the models we have not yet implemented
will be contributed by the community in an open collaboration
process.

The process of contributing to the ns-3 project varies with
the people involved, the amount of time they can invest
and the type of model they want to work on, but the current
process that the project tries to follow is described here:
https://www.nsnam.org/developers/contributing-code/

This README excerpts some details from a more extensive
tutorial that is maintained at:
https://www.nsnam.org/documentation/latest/

## Building ns-3

The code for the framework and the default models provided
by ns-3 is built as a set of libraries. User simulations
are expected to be written as simple programs that make
use of these ns-3 libraries.

To build the set of default libraries and the example
programs included in this package, you need to use the
tool 'ns3'. Detailed information on how to use ns3 is
included in the file doc/build.txt

However, the real quick and dirty way to get started is to
type the command
```shell
./ns3 configure --enable-examples
```

followed by

```shell
./ns3
```

in the directory which contains this README file. The files
built will be copied in the build/ directory.

The current codebase is expected to build and run on the
set of platforms listed in the [release notes](RELEASE_NOTES.md)
file.

Other platforms may or may not work: we welcome patches to
improve the portability of the code to these other platforms.

## Running ns-3

On recent Linux systems, once you have built ns-3 (with examples
enabled), it should be easy to run the sample programs with the
following command, such as:

```shell
./ns3 run simple-global-routing
```

That program should generate a `simple-global-routing.tr` text
trace file and a set of `simple-global-routing-xx-xx.pcap` binary
pcap trace files, which can be read by `tcpdump -tt -r filename.pcap`
The program source can be found in the examples/routing directory.

## Getting access to the ns-3 documentation

Once you have verified that your build of ns-3 works by running
the simple-point-to-point example as outlined in 3) above, it is
quite likely that you will want to get started on reading
some ns-3 documentation.

All of that documentation should always be available from
the ns-3 website: https://www.nsnam.org/documentation/.

This documentation includes:

  - a tutorial

  - a reference manual

  - models in the ns-3 model library

  - a wiki for user-contributed tips: https://www.nsnam.org/wiki/

  - API documentation generated using doxygen: this is
    a reference manual, most likely not very well suited
    as introductory text:
    https://www.nsnam.org/doxygen/index.html

## Working with the development version of ns-3

If you want to download and use the development version of ns-3, you
need to use the tool `git`. A quick and dirty cheat sheet is included
in the manual, but reading through the git
tutorials found in the Internet is usually a good idea if you are not
familiar with it.

If you have successfully installed git, you can get
a copy of the development version with the following command:
```shell
git clone https://gitlab.com/nsnam/ns-3-dev.git
```

However, we recommend to follow the Gitlab guidelines for starters,
that includes creating a Gitlab account, forking the ns-3-dev project
under the new account's name, and then cloning the forked repository.
You can find more information in the [manual](https://www.nsnam.org/docs/manual/html/working-with-git.html).
