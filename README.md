# RunTimeFaultInjector framework

## An overview
The RunTimeFaultInjector framework is a run-time time-triggered fault injector, that means faults are injected to the target at runtime and no modification to target operating system or application should be required.

The RunTimeFaultInjector framework consists of the following components:
1. RunTimeFaultInjector Core - handles all of the communication with the target system and the injection of faults. The fault injection is handled by FaultInjectionManager and FaultInjectionMechanism modules. The former implements fault injection policy, i.e, when to inject, fault duration, etc. The latter implements low-level communication with the target system and handles the actual fault injection. One execution of RTFI core is one experiment. Before the experiment can take place, the target device must be prepared using vendor specific tools.


2. run_experiment_set.sh script - encapsulates RTFI core and handles all the necessary environment preparations: compilation of OS and application, loading OS and application to target device, running vendor tools in background. Also, experimental results are gathered by this script.

3. helper scripts - implement target specific operations, such as programming FPGA, loading OS and application

The Core is written in C and the rest are bash or python scripts.

## Software Requirements

1. Quartus Prime 18.1 or later - this runs only on MS Windows with cygwin or GNU/Linux
2. make, cmake, gcc
3. Ubuntu 18.04 LTS is recommended

## Installation on Ubuntu Based Systems
### UNIX tools
You need to install all the requreid UNIX tools. This can be done by one command run as root or with sudo.
```
$ apt install make cmake gcc
```

### Quartus Prime Lite edition
Please refer to INTEL.md for instructions on how to download, install, and configure Quartus from Intel.
