# iPod manager

http://yuo.be/ipod.php

The iPod manager source code (contained in the `foo_dop` folder) is released under the Lesser GNU Public Licence (see COPYING and COPYING.LESSER).

VS2015 projects can be found in the `vc14` folder.

## Building

First, clone the repo (use `git clone --recursive` to clone the submodule at the same time).

You need a copy of the Windows version of the CoreFoundation headers to compile iPod manager. One source of these is the [WebKit Support Library](https://developer.apple.com/opensource/internet/webkit_sptlib_agree.html); extract the contents of the `win` folder to the `Apple` folder.

Then, fire up Visual Studio 2015, and open the solution in the `vc14` folder. Amend the include directory for the CoreFoundation headers in foo_dop project properties if required. It is possible you may also need to fiddle with the CoreFoundation headers to make them compile. Then, you should be set!

Produced builds require `iTunesCrypt.dll` found in the `MobileDeviceSign` folder.

## Acknowledgements

Required to bild iPod manager, and contained in this repo are:

### zlib
Copyright (C) 1995-2010 Jean-loup Gailly and Mark Adler

http://zlib.net

For the licence, please see `zlib-1.2.5/zlib.h`


### SQLite
Publc domain

https://www.sqlite.org
