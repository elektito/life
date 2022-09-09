# Life

This is a simple particle simulator. There are four particle types
(red, green, blue, and yellow), each either pulling or pushing others
by a certain amount.

Press `V` to view the GUI.

## Build Instructions

Before anything, make sure the submodules are initialized and updated
(`git submodule update --init`).

For a native build, simply run `make`. This will create an executable
named `life`.

To build the WASM version, make sure you have an environment variable
named `EMSDK_PATH` pointing to `emsdk` location, and then build by
running something like this:

    EMSDK_PATH=~/source/emsdk make web

This will create a `web` directory containing life.html, life.js, and
life.wasm.
