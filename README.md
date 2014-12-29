#### CommViewer
Draws a small, mouse moveable/clickable, text box in the upper left hand corner
of the simulator screen. The output shows PilotEdge Connection, PTT (Push-To-Talk),
TX/RX, Com1, and Com2 status.

When the output box is clicked on, the plugin will select an unselected Com
or unselect a Com previously selected via mouse click. Typically, this is useful
for listening to ATIS information without having to leave the primary Com. Note,
for aircraft with an audio panel that allows multiple Com selection, the mouse
click is ignored.

To move the window, place the mouse pointer over the window, hold down the
left mouse button, move the window to the desired location and release the
mouse button.


#### Example Output (COMx: 0=Unselected, 1=Selected, TX/RX: ON=1, OFF=0):
```
[Pilot Edge] Connected: YES     TX: 0   RX: 1
PTT: OFF   COM1: 1   COM2: 0
```
```
[Pilot Edge] Connected: NO      TX: 0   RX: 0
PTT: ON    COM1: 1   COM2: 0
```
```
[Pilot Edge] Connected: YES     TX: 1   RX: 0
PTT: OFF   COM1: 1   COM2: 1
```

![Alt text](./images/CommViewer.png "X-Plane screenshot")


#### Windows Build
Plugin compiled using MSVC++ 2012 Express.

Open a MSVC++ tool shell specific to the desired target (32/64 bit)
and "cd" to the directory containing the plugin code then:

- to build a 64-bit binary: > make.msvc.bat
- to build a 32-bit binary: > make.msvc.bat 386

Move the win.xpl plugin to your X-Plane 10/Resources/plugins folder.


#### Mac & Linux Build
Run make from a shell window:

- to build a 64-bit binary: $ make
- to build a 32-bit binary: $ make 386=1

Move the *.xpl plugin to your X-Plane 10/Resources/plugins folder.


#### TODO
- add hotkey handling to hide the window
- add comm audio volume control


#### MISC/NOTES


#### VERSIONS
- 1.5.2 add version, date, and email info to the plugin's info, other optimizations
- 1.5.1 window callbacks sanity check the incoming window id
- 1.5.0 added a pilotedge plugin registration check with proper ref assignment
- 1.4.3 add make file and osx binaries
- 1.4.2 bug fix, correct typecast sizes and types
- 1.4.1 added PilotEdge connection and TX/RX status
- 1.4 box movement check changes
- 1.3 standard plugin folder structure
- 1.2 Uses the latest SDK, standard plugin folder
- 1.1 Floating window
- 1.0 Initial working build
