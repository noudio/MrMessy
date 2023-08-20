 # MrMessy
Jack Audio X11 apps for audio visualization

Current state: Initial commit
Source is still a bit messy, lots of viral introns in code


Platforms:
==========
linux with X11, Xext, and Jack dev libraries.

e.g. ubuntu:
apt install x11proto-dev libjack-dev


How to Compile (currently):
===========================

./make.sh

How to run
==========

spectrum analyzer:
./xdisp

scope:
./xdisp --scope


By default it creates jack ports, but doesnt connect them.
Use jackctl patch bay to setup automatic connections.
Or otherwise ./xdisp [--connect] makes automatic connection to the write of
system output

