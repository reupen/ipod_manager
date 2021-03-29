# iPod manager

http://yuo.be/ipod-manager

[![Build status](https://reupen.visualstudio.com/Columns%20UI/_apis/build/status/reupen.ipod_manager?branchName=master)](https://reupen.visualstudio.com/Columns%20UI/_build/latest?definitionId=7&branchName=master)

## Licence

The iPod manager source code (contained in the `foo_dop` directory) is released under the Lesser GNU Public Licence (LGPL) (see `foo_dop/COPYING` and `foo_dop/COPYING.LESSER`).

The iPod manager SDK source code (contained in the `dop-sdk` directory) is released under the BSD Zero Clause (0BSD) licence (see `dop-sdk/LICENCE`).

## Building

1. Clone the repo (use `git clone --recursive` to clone all submodules at the same time).

2. Install `ms-gsl` and `wil` using [Vcpkg](https://github.com/Microsoft/vcpkg).

3. Open the solution in the `vc16` folder using Visual Studio 2019, and build the solution from the Build menu. 

Produced builds require `iTunesCrypt.dll` found in the `MobileDeviceSign` folder.

## Acknowledgements

Required to build iPod manager, and contained in this repo are:

### zlib

Copyright (C) 1995-2010 Jean-loup Gailly and Mark Adler

http://zlib.net

For the licence, please see `zlib-1.2.5/zlib.h`


### SQLite

Public domain

https://www.sqlite.org
