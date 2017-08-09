# ECE 3056 - Computer Architecture

This repository contains the four lab assignments I completed during ECE 3056: Architecture, Concurrency, and Energy in Computation (Spring 2017) taught by [Dr. Moin Qureshi](http://moin.ece.gatech.edu). The assignments concern the progressively more sophisticated architectural simulations of the LC-3b ISA created by Yale N. Patt.

#### Lab 1
This assignment involved writing an instruction-level simulator for the LC-3b. The simulator takes one input file entitled isaprogram, which is an assembled LC-3b program. The simulator  executes the input LC-3b program, one instruction at a time, modifying the architectural state of the LC-3b after each instruction.

#### Lab 2
This assignment involved creating a cycle-level simulator for the LC-3b. The simulator takes two input files:
1. A file entitled ucode which holds the control store.
2. A file entitled isaprogram which is an assembled LC-3b program.

The simulator executes the input LC-3b program, using the microcode to direct the simulation of the microsequencer, datapath, and memory components of the LC-3b.

#### Lab 3
This assignment required a simulator for the pipelined LC-3b. The simulator takes the same two input files as described above.

#### Lab 4
The final assignment required the implementation of a cache simulator, with multiple cache levels, and configurable for cache size, line size, and replacement policies. The first part concentrates only on the first level date cache. The second part deals with taking the cache object and simulating a two-level cache hierarchy and connecting it to main memory. The cache was evaluated with traces for 100M instructions from three SPEC2006 benchmarks: bzip2, lbm, and mcf.
