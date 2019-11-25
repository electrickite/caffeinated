caffeinated
===========

Caffeinated is a small utility which prevents the system from entering an idle
state by using a systemd logind idle inhibitor lock. It can be run either in the
forground or as a daemon.

Requirements
------------
Caffeinated requires the following software to build:

  * C compiler (ie. GCC)
  * make
  * pkg-config
  * libsystemd _or_ libelogind

Installation
------------
Run the following command to build and install caffeinated (as root if
necessary):

    $ make clean install

Or add `ELOGIND=1` to use libelogind:

    $ make ELOGIND=1 clean install

Usage
-----
See `man caffeinated` for details.

Authors
-------
caffeinated is written by Corey Hinshaw.

License and Copyright
---------------------
Copyright 2019 Corey Hinshaw

Licensed under the terms of the MIT license. See the license file for details.
