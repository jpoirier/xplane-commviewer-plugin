#### CommViewer

Draws a small, mouse clickable, text box in the upper left hand corner of the
simulator screen. The output shows PTT (Push-To-Talk), Com1, and Com2 status.

When the output box is clicked on, the plugin will select an unselected Com
and unselect and previously selected com. Typically, this is useful for
listening to ATIS information without having to leave the primary com. Note,
for aircraft with an audio panel that allows multiple com selection, the mouse
click is ignored.

#### Example Output (where: 0=Unselected, 1=Selected):

- PTT: OFF   COM1: 1   COM2: 0  -

- PTT: ON   COM1: 1   COM2: 0  -

- PTT: OFF   COM1: 1   COM2: 1  -


#### Windows Build

Plugin compiled using MSVC++ 2012 Express.

Open an MSVC++ tool shell specific to the desired target (32/64 bit) an "cd"
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
