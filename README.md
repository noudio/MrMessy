 # MrMessy
Jack Audio X11 apps for audio visualization
===========================================

The apps show themselves as a kind of 'virtual instruments' on your desktop.

Funfacts:
1) The pictures of the instruments are from ones that I own personally.
2) All controls can have callable code, currently only the powerbuttons work
3) The instruments are resizeable in the most strangest of ways.
4) Press the top-left corner of an instrument, to have the window as a borderless desktop app

Current state: Initial commit
Source is still a bit messy, lots of viral introns in code

Platforms:
----------
linux with X11, Xext, and Jack dev libraries.

e.g. ubuntu:
apt install x11proto-dev libjack-dev

How to Compile (currently):
---------------------------

./make.sh

How to run
----------

spectrum analyzer:
(bash)$ ./xdisp

![spectrum-analyzer](https://github.com/noudio/MrMessy/blob/main/doc/xdisp-spectrum-analyzer.png)

scope:
(bash)$ ./xdisp --scope

![scope](https://github.com/noudio/MrMessy/blob/main/doc/xdisp-scope.png)

By default it creates jack ports, but doesnt connect them.
Use jackctl patch bay to setup automatic connections.
Or otherwise ./xdisp [--connect] makes automatic connection to the write of
system output

