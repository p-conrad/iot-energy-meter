# IoT Energy Meter

This is a program intended to run on WAGO I/O System 750/753 series programmable logic controllers
(PLC), replacing the default CoDeSys/e!Cockpit runtime. Taking control of the local KBus, it scans
for all compatible 750-795/794 power measurement modules and cyclically reads out a predefined set
of measurement values while honouring the PLC cycle time.  Once a set of measurements is completed,
a timestamp is added and the results are sent out via MQTT to a configured MQTT broker.

This project is the result of my master's thesis and was originally created as a part of the
[WINNER Reloaded](https://winner-projekt.de/) research project for reading energy data in
a residential neighbourhood. Development concluded at the beginning of 2021 and while I have been
updating some parts to reflect recent changes to WAGO's PFC Firmware SDK, some more testing will
be required to guarantee correct compilation and execution on newer firmware versions. This
repository will be updated accordingly when I get the chance to test and verify it with a real PLC
again.


## Features

* This program can read most of the energy values the 795/794 power measurement modules provide,
  from an arbitrary number of modules on the KBus. If there are more than 4 values configured,
  they are read in multiple cycles.
* Transient reaction processes of the modules are taken into account, preventing the reading of
  unstable or incorrect values.
* Measurement results can be sent via MQTT either using plain text or
  [Protocol Buffers](https://developers.google.com/protocol-buffers/) according to the message
  definition in this repository. Protobuf messages are limited to this format (currently voltage,
  effective and reactive power of each phase) and extending it requires code changes, while plain
  text should work out of the box with any of the predefined measurements.
* Messages are sent using MQTT 5, taking advantage of the protocol's
  [topic aliases](https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901113) to
  reduce bandwidth usage. However, changing the code to MQTT 3.1.1 requires only a few simple
  changes (see 3d18f386).
* Furthermore, MQTT messages are staggered across multiple cycles where possible, to prevent spikes
  in bandwidth usage and allow for a more stable run time behaviour. For instance: Reading
  8 measurement values from 10 modules would result in 10 messages after 8/4=2 cycles. Instead of
  sending them all at once (and none during the next cycle), 5 messages are sent during each cycle
  instead.

Moreover, this project may provide some educational value by showcasing an end-to-end example for
developing a real-world application with WAGO's PFC Firmware SDK, including:
* an example of how to set up your development environment with [clangd](https://clangd.llvm.org/),
  so you get the full editing convenience including code completion and syntax checking,
* a deployment script to automate compilation and deployment to the device, allowing for rapid
  development, and
* an example of code documentation using [Doxygen](https://www.doxygen.nl/).


## Limitations

This project has mainly been developed for research and has a few limitations:
* Modules other than power measurement modules are not supported. While the program will correctly
  determine the process data size and allocate the process images accordingly, no actual data
  besides zeroes will get sent to these modules and no actual data is retrieved from them. The
  absence of erroneous behaviour for these modules cannot be guaranteed.
* Also, the 750-493 power measurement module is not supported. This module has a hugely different
  (more complex) way of querying data. The program will correctly detect this and simply ignore
  these modules.
* Using more than one PM module has not been tested with an actual device, due to lack of
  available hardware. If you try it, any feedback would be appreciated.
* Error checking is still limited. While all error bits are correctly stored and accessible in the
  process image, most of them are ignored.
* The program segfaults when compiling with optimization level -O2. This has not been debugged yet.
  However, -O1 works fine and computations in each cycle usually finish within 5ms, leaving plenty
  of headroom.

Most importantly, **this program is not endorsed or supported by WAGO in any way. It has not been
fully tested and no guarantees can be made regarding its reliability. Do not use it in an
environment where failure may cause injury, loss of life, or financial damage.**

In a safety-critical environment, replacing the PLC runtime for custom code is rarely a good idea,
so please consider carefully before using this project. While it had been in productive use
without issues over several months in a lab environment, your setup and experience may differ.


## Setting Up and Building the Project

1. This project needs to be configured and compiled within the context of WAGO's
   [PFC Firmware SDK](https://github.com/WAGO/pfc-firmware-sdk). Be sure to follow the install
   instructions there up to the point where you have a working firmware image.
2. Create a new directory named `iot-energy-meter` in `ptxproj/src` and copy all the source files
   in `src` there.
3. Most settings of the program are currently hardcoded, so you probably also want to change your
   MQTT settings in `mqtt.h` before building and maybe also update the `listOfMeasurements`
   variable to include your desired measurements (see `unit_description.h` for examples).
4. Copy the rule files into `ptxproj/rules`
5. In your project directory, call `ptxdist menuconfig` and enable the program there.
6. Build the project using `ptxdist targetinstall iot-energy-meter`.
7. You can find the finished package in `ptxproj/platform-wago-pfcXXX/packages`.

Alternatively, you can use the `deploy.sh` script, which will handle most of the work for you: Just
copy the rule files as before, do your configuration in this directory, and adjust your device
address in the script before calling. This will update the source code in the project, build it,
push the result to your PLC, and drop you into an SSH session, ready fire it up.

If you like to develop the project, here are some more steps:
1. Set your project path in `compile_commands.json`. Note that only absolute paths work.
2. Set up your favourite editor or IDE with clangd. Now you should have code completion and
   inspection features to ease development for you.


## Alternatives

Need to retrieve energy measurements from your device but don't feel comfortable tampering with
the runtime, or require stronger safety guarantees?
These alternatives may work for you:
* Use WAGO's [Energy Management](https://www.wago.com/global/energy-management). This is the most
  straightforward and officially supported solution.
* Program your PLC using the WagoAppPowerMeasurement, WagoAppCloud, and optionally WagoAppJSON
  libraries. It can be a bit tedious and needs to be done individually for each module,
  but it should work fine once everything is correctly set up.
* In your PLC project, set up a Modbus slave and connect it to your controller. Add data points for
  all values you want to publish and cyclically fill them using the WagoAppPowerMeasurement library.
  You can then use your favourite Modbus tool or library to read and further process them. Jonas
  Neubert has an entertaining talk on this topic [here](https://www.youtube.com/watch?v=EMkWRlbpJsk).
* Try [Node-RED](https://nodered.org/), a graphical low-code programming environment for graphically
  connecting devices and more. WAGO distributes [their custom version](https://github.com/WAGO/node-red-iot)
  with some useful packages preinstalled and there is also a library to map some common modules
  and a tutorial video available [here](https://flows.nodered.org/node/node-red-contrib-remote-io)
  and [here](https://www.youtube.com/watch?v=9syAlOw6a_A).
