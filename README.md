 # MrMessy
Jack Audio X11 apps for audio visualization
===========================================

The apps show themselves as a kind of 'virtual instruments' on your desktop.

https://github.com/noudio/MrMessy/assets/6895032/08d1c9b3-aa1f-43cb-af1a-93fd32b5d94b

Funfacts:
1) The pictures of the instruments are from ones that I own personally.
   - The spectrum analyzer is a Hewlett Packard HP-8591A.
   - The scope is a Philips PM-3295.
2) Uses shared memory with X-windows server for fast updating of the the graphics.
3) All useful intrument controls (knobs, displays etc.) are defined and can have callback code attached, but currently only the powerbuttons and topleft corners are functional in that way.
4) The instruments are resizeable in the most strangest of ways 🙃.

Current state: Initial commit... 
- Apps are functioning correctly
- It's a toy, there are no useful measurements shown yet. Just the visuals...
- Source is still a bit messy, lots of viral introns, stale sections etc. in code, but all code written by me.

Platforms:
----------
Linux with X11, Xext, and Jack dev libraries.

e.g. ubuntu:
```
$ apt install x11proto-dev libjack-dev
```

How to Compile (currently):
---------------------------

```
$ ./make.sh
```

How to run
----------

- Spectrum analyzer:
```
$ ./xdisp
```
![spectrum-analyzer](https://github.com/noudio/MrMessy/blob/main/doc/xdisp-spectrum-analyzer.png)

- Scope:
```
$ ./xdisp --scope
```
![scope](https://github.com/noudio/MrMessy/blob/main/doc/xdisp-scope.png)

- Black spectrum analyzer:
```
(bash)$ ./xdisp --black
```
This can be useful for video grabbing, due to its black background. I used it for example to create this youtube vid: [andjelisa on rararadio](https://youtu.be/DI8FW4kV9h8?list=PL6jUM7gk5v0XpZLoLPm6GyWbw1ySJ1M1x&t=6864) , translating the black background to transparent and rotating the output by 90 degs 😎

Controls:

- Click the top-left corner of the instrument to have the app as a non-decorated desktop app (i.e. no borders and caption). Press it again to undo this, for repositioning or resizing. See the demo video for a demonstration of this.
- Click the power button or press 'q' to exit the program.
- By clicking other knobs or items, one can see what items in the code are defined, they are written to stdout.

At startup, 'xdisp' only creates jack ports, but it doesnt connect them. You can use jackctl patch-bay to conveniently setup connections automatically.
Or otherwise: `./xdisp [--connect]` makes an automatic connection to the writers of the system output.

