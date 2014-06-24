#### CommViewer
Draws a small, mouse moveable/clickable, text box in the upper left hand corner
of the simulator screen. The output shows PTT (Push-To-Talk), Com1, and Com2 status.

When the output box is clicked on, the plugin will select an unselected Com
or unselect a Com previously selected via mouse click. Typically, this is useful
for listening to ATIS information without having to leave the primary Com. Note,
for aircraft with an audio panel that allows multiple Com selection, the mouse
click is ignored.

To move the window, place the mouse pointer over the window, hold down the
left mouse button, move the window to the desired location and release the
mouse button.


#### Example Output (where: 0=Unselected, 1=Selected):

PTT: OFF   COM1: 1   COM2: 0

PTT: ON   COM1: 1   COM2: 0

PTT: OFF   COM1: 1   COM2: 1


#### Windows Build
Plugin compiled using MSVC++ 2012 Express.

Open an MSVC++ tool shell specific to the desired target (32/64 bit) and "cd"
to the directory containing the plugin code. Change XPLM_64.lib or XPLM.lib,
32-bit and 64-bit respectively, on the "set LINK_LIBS" in make.msvc.bat.

Run make.msvc.bat from the shell window then move the CommViewer.xpl plugin to
X-Plane 10/Resources/plugins folder.


#### TODO
- automated xcode build
- automated linux build
- automated windows build
- proper window placement when sim window resized
- floating window


#### MISC/NOTES
- xcode cli  Release|Debug
- xcodebuild -project saitek.xcodeproj -alltargets -configuration Debug

#### VERSIONS
- 1.2 Uses the latest SDK, standard plugin folder
- 1.1 Floating window
- 1.0 Initial working build
