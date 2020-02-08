caffeinated
===========
Caffeinated is a small utility which prevents the system from entering an idle
state by using systemd logind and Wayland idle inhibitors. It can be run either
in the foreground or as a daemon.

Requirements
------------
Caffeinated requires the following software to build:

  * C compiler
  * GNU make
  * libsystemd _or_ libelogind
  * libbsd

Additionally, Wayland idle inhibit support requires:

  * wayland-client
  * wayland-protocols

Installation
------------
Run the following command to build and install caffeinated (as root if
necessary):

    $ make clean install

Add `SDBUS=elogind` to use libelogind instead of libsystemd:

    $ make SDBUS=elogind clean install

Wayland idle inhibit support can be enabled with `WAYLAND=1`:

    $ make WAYLAND=1 clean install

Usage
-----
See `man caffeinated` for details.

Authors
-------
caffeinated is written by Corey Hinshaw.

License and Copyright
---------------------
Copyright 2019-2020 Corey Hinshaw

Licensed under the terms of the MIT license. See the license file for details.
