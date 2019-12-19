# Timing channel benchmarking tool

This project provides an application that can be used to benchmark
potential hardware timing channels and measure their bandwidth. Part of
this project is a set of
[tools](https://github.com/SEL4PROJ/channel-bench-tools) which are
required to process the output of the benchmark and produce meaningful
data.

## Repository initialisation 

This project is managed in the same manner as all other repositories.
See the [getting
started](https://docs.sel4.systems/GettingStarted.html#projects) page
for more info.

To create a directory containing this project, execute the following
commands. (Note that this project is currently under active development;
use the `timing.xml` manifest as below to check out branches that
provide development support for this project).

```
mkdir channel-bench
cd channel-bench
repo init -u https://github.com/SEL4PROJ/channel-bench-manifest.git -m timing.xml
```

## Configuring and building

Once the project has been created, a project can be configured as
follows:

```
cd channel-bench
mkdir build
cd build
../init-build.sh -DPLATFORM=<platform-name>
```

Configuration choices can then be made in the CMake menu using `ccmake
.` from within the build directory.

Note: Only one benchmark kind should be configured at any time to ensure
that results from different benchmarks are not mixed in the output.

Once the configuration is complete, `ninja` can be used to build an
image.

Determining sane configurations may be difficult. To determine what
combinations may be reasonable, check the
`projects/channel-bench/configs` directory which contains configurations
for our old build system. You should be able to ignore options that do
not appear in the CMake menu.

### Supported platforms and architectures

This project currently supports `aarch32` with the `sabre` platform and
`x86` platforms.

## Processing benchmark output

Processing benchmark output is done using the
[channel-bench-tools](https://github.com/SEL4PROJ/channel-bench-tools)
which will be in the `repo` project at `tools/channel-bench`.

## Development support

This project is still under development and `master` is not stable. The
`timing` channel in the following repositories reflects the ongoing
development work to support this project.

 * [sel4proj/channel-bench](#)
 * [sel4/sel4bench](https://github.com/seL4/sel4bench)
 * [sel4/seL4_libs](https://github.com/seL4/seL4_libs)

Make sure that `repo` is initialised to use the `timing.xml` manifest to
track these branches rather than their `master` master branches.
