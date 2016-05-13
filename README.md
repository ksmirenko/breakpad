# About this fork

I have created this fork to customize Breakpad and its tools for me as I am working to [integrate Breakpad](https://github.com/ksmirenko/breakpad-testfield) into Qt projects.

Please see [this repo](https://github.com/jon-turney/google-breakpad) for the original, up-to-date DWARF-supporting Breakpad edition. The majority of features (dump_syms_dwarf and changes in processor) in my repository come from that one.

## Features and differences from Google's original

* `tools/windows/dump_syms_dwarf` - a tool which can read DWARF debugging information from PE/COFF executables and create symbol files
* changes in processor - in the absence of debug_file and debug_identifier, defaults of code file name and identifier of 33 zeroes are used.
* `tools/windows/symupload_nodump` - a tool for uploading existing symbol files (the original `symupload` performes symbol dumping as well, which is not what you want in case of DWARF files). If no debug_file or debug_identifier can be extracted from module (which takes place in case of DWARF/PECOFF), code_file is used as debug_file, and a string of 33 zeroes is used as debug_identifier.
* changes in `http_upload` - the old version is used that uploads only one file (and, therefore, takes file name and file contents as parameters, not a map). See `http_upload.h` for details.

## Build

I use the following configuration:

* for Breakpad: Mingw-w64 with MSYS2 64bit (I use Windows 8.1 64-bit)
* for symupload_nodump: Visual C++ 2015 with Visual Studio 2015 Community

To build Breakpad, run:
```
autoreconf -fvi
./configure
make
```
This will give you dump_syms_dwarf, minidump_stackwalk and some other binaries.

# Breakpad

Breakpad is a set of client and server components which implement a
crash-reporting system.

* [Homepage](https://chromium.googlesource.com/breakpad/breakpad/)
* [Documentation](https://chromium.googlesource.com/breakpad/breakpad/+/master/docs/)
* [Bugs](https://bugs.chromium.org/p/google-breakpad/)
* Discussion/Questions: [google-breakpad-discuss@googlegroups.com](https://groups.google.com/d/forum/google-breakpad-discuss)
* Developer/Reviews: [google-breakpad-dev@googlegroups.com](https://groups.google.com/d/forum/google-breakpad-dev)
* Tests: [![Build Status](https://travis-ci.org/google/breakpad.svg?branch=master)](https://travis-ci.org/google/breakpad)

## Getting started in 32-bit mode (from trunk)

```sh
# Configure
CXXFLAGS=-m32 CFLAGS=-m32 CPPFLAGS=-m32 ./configure
# Build
make
# Test
make check
# Install
make install
```

If you need to reconfigure your build be sure to run `make distclean` first.

## To request change review:

1.  Get a copy of depot_tools repo.
    http://dev.chromium.org/developers/how-tos/install-depot-tools

2.  Create a new directory for checking out the source code.
    mkdir breakpad && cd breakpad

3.  Run the `fetch` tool from depot_tools to download all the source repos.
    `fetch breakpad`

4.  Make changes. Build and test your changes.
    For core code like processor use methods above.
    For linux/mac/windows, there are test targets in each project file.

5.  Commit your changes to your local repo and upload them to the server.
    http://dev.chromium.org/developers/contributing-code
    e.g. `git commit ... && git cl upload ...`
    You will be prompted for credential and a description.

6.  At https://codereview.chromium.org/ you'll find your issue listed; click on
    it, and select Publish+Mail, and enter in the code reviewer and CC
    google-breakpad-dev@googlegroups.com
