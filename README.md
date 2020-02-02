# iPod manager

http://yuo.be/ipod_manager

[![Build status](https://reupen.visualstudio.com/Columns%20UI/_apis/build/status/reupen.ipod_manager?branchName=master)](https://reupen.visualstudio.com/Columns%20UI/_build/latest?definitionId=7&branchName=master)

The iPod manager source code (contained in the `foo_dop` folder) is released under the Lesser GNU Public Licence (see COPYING and COPYING.LESSER).

A VS 2017 solution can be found in the `vc15` folder.

## Building

First, clone the repo (use `git clone --recursive` to clone the submodule at the same time).

Then, fire up Visual Studio 2017, and open the solution in the `vc15` folder, and build the solution from the Build menu. 

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
