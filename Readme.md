# WINNER Power Reader

This is a program intended to run on WAGO I/O System 750/753 series programmable logic controllers (PLC), replacing the
default CoDeSys/e!Cokpit runtime. Taking control of the local KBus, it scans for all compatible 750-795/794 power
measurement modules and cyclically reads out a predefined set of measurement values while honoring the PLC cycle time.
Once a set of measurements is completed, a timestamp is added and the results are sent out via MQTT to a configured MQTT
broker.


## Development

This project needs to be configured and compiled within the context of WAGO's [PFC Firmware
SDK](https://github.com/WAGO/pfc-firmware-sdk). Be sure to follow the install instructions there up to the point where
you have a working firmware image. The official guide requires Ubuntu version 16.04 (64bit), but has been verified to
work with 18.04, 20.04 and 20.10 as well after applying some fixes, which are documented below. Other distributions have
not been tested and may or may not work. When in doubt (or to make sure you get a firmware as close as possible to the
official one) it might be best to install Ubuntu 16.04 in a virtual machine and work with that.

Some additional notes on the installation guide:
* The version of PTXdist distributed by WAGO still seems to rely on Python 2, which is deprecated and might not be
  present at all in newer Ubuntu versions. If an error about python2 not being found shows up, installing
  'python2-minimal' should fix it.
* Step 1.2: Adding the 'git-core' PPA and using packagecloud.io to install git-lfs should not be necessary. Simply
  installing git-lfs with apt should be enough.
* Step 3.3: On systems other than Ubuntu 16.04, verify your version of gawk with `gawk --version`. If the version is
  5 or above, a small adjustment needs to be made to one of the scripts to accomodate for a subtle syntax change prior
  to building and installing: In the file `scripts/lib/ptxd_lib_dgen.awk`, change line 56 from `/^\#|^$/ {` to
  `/^#|^$/ {` (-> remove the backslash).

# Errors
```
ptxproj/platform-wago-pfcXXX/sysroot-host/lib/ct-build/internal/base.mk:29: *** empty variable name.  Stop.
```

