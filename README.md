# Slip & Frag

## What is it

Slip & Frag is a sourceport of the Quake game engine, as created by [id Software](https://www.idsoftware.com/). 

Slip & Frag aims to be two things:

* An engine that allows you to play Quake, the game, as closely as it was when published in 1996 for the DOS platform. This means somewhat low resolution textures, low polygon count, colorful and high-contrast worlds, with sharp-edged, dangerous-looking creatures. That, and messages with large white (or brown) letters.

* An engine that runs in modern operating systems (MacOS 10.14 and later, Windows 10 and later, Meta Quest/2/3/Pro/3s v62 and later, and others), removing the limitations that restricted the original engine to run in a limited, fixed amount of memory, thus allowing it to accept the content created during these years by the user community.

## What is available

Latest version: **1.1.31** - see [Changelog](CHANGELOG.md) for details.

Releases are available [here](https://github.com/Izhido/SlipNFrag/releases/latest). They are:

* **Win64**: Desktop version for Windows. Uses the original software renderer to bring the game as closely as possible as when it was played in 1997. Playable with a keyboard, mouse, and optionally an Xbox One (or compatible) controller. (At least one user reported to me that this version can also be played in Linux machines through Wine.)

* **MacOS**: Desktop version for MacOS. Plays in both Intel and Apple Silicon machines. Also playable with keyboard, mouse, and Xbox One controller.

* **XR**: VR version that runs as a standalone application in Meta Quest devices (1, 2, Pro, 3, 3s). Uses OpenXR / Vulkan to render the game. Playable with the controllers supplied with your VR device.

* **PCXR**: VR version that runs in Windows as a PCVR application, that sends its output to a connected Meta Quest device (same as above). Uses the OpenXR runtime exposed by the Meta Quest Link app, and renders using Vulkan. Playable also using the controllers supplied with your VR device.

## What do you need

First (and most important) of all, you need Quake, the game, installed in your computer. 

Quake can be purchased from several venues (Steam, GOG, marketplaces within Amazon, and many others). There is also a shareware version of the game, hosted on many sites, which can be used to play the game with Slip & Frag, if desired.

*IMPORTANT*: Slip & Frag does not currently support the .pak files from the Quake re-release, issued for the 25th anniversary of the game. The original .pak files, however, can still be found in the folder where the new game is installed.

For the **XR** release, you'll need a Meta Quest device (1, 2, Pro, 3, 3s), updated to the latest available version.

Additionally, in order to play the **PCXR** release, you need:

* A fairly recent PC running Windows, with a video card no more than 5 years old;

* Meta Quest Link installed and running;

* A Meta Quest device connected to the PC in Link mode;

* OpenXR runtime set up to be the one provided by Meta. (**This is important** - the OpenXR runtime provided by SteamVR is currently not compatible with Slip & Frag.)

## What to do

After getting what you need, do the following to play the game:

### Win64

* Extract and copy the `SlipNFrag-Win64.exe` executable into the same folder as your `ID1` folder.

* Double click on the executable, and press the Play button to start.

* Optionally, you can do the following from Powershell (or your favorite command-line prompt):

```
cd C:\Folder\Where\Everything\Is\Installed
.\SlipNFrag-Win64.exe -basedir "C:\Program Files\Steam\steamapps\common\Quake" -game alkaline +map start
```
> (Standard command-line options and rules from the game still apply. Feel free to experiment as you wish with custom maps / mods.)

### MacOS

* Copy the `ID1` folder from the machine where you have Quake installed, anywhere you wish in your Mac.

* Open the app, and click on the small Settings icon at the bottom right of the window (alternatively, you can also go to Preferences in the menu).

* At the "Game directory (-basedir)" prompt, enter the path to the `ID1` folder you just copied, and configure everything else as you wish.

* Click on the Close button.

* Click on the large Play button to start playing the game.

### Meta Quest/2/3/Pro/3s

* Ensure that you have Developer mode enabled for your device. (This will take some extra steps through several places in your device *and* the Meta developer website. You'll only have to do this once, though. Google for "enable developer mode meta quest" for more details).

* Install the .apk file in your device, using either:
    * ADB (See https://www.vrtourviewer.com/docs/adb/ for details);
    * Android Studio (https://developer.android.com/studio);
    * SideQuest (https://sidequestvr.com/setup-howto);
    * Your favorite .apk installation tool.

* Start Slip & Frag from your device *once*, from the Unknown sources screen or tab. When you do this, the following folder will be automatically created in your device:

```
/sdcard/android/data/com.heribertodelgado.slipnfrag-xr/files
```

* Use any file transfer app (Windows Explorer, or SideQuest if Explorer is unavailable) to copy the `ID1` folder to that exact location.

* Exit, and then restart, Slip & Frag in your device. Press A + X in your controllers (as indicated in the headset) to start playing.

* Optionally, to load your favorite custom mods / maps, create the following file in the folder above in your device:

```
/sdcard/android/data/com.heribertodelgado.slipnfrag-xr/files/commandline.txt
```

* In that file, enter all command-line arguments your custom map / mod requires. For example:

```
-game alkaline +map start
```
> (Standard command-line options and rules from the game still apply. Feel free to experiment as you wish with custom maps / mods.)

### PCXR

* Extract and copy all files in the package anywhere you wish in your machine. (If desired, you can also copy them in your `ID1` folder - just be aware that there are several PNG files and an extra `shaders` folder which might overwrite stuff in your existing Quake folder.)

* Connect your Meta Quest device to your computer via Meta Quest Link. Ensure that your boundaries and the ground level in your device fits the area you'll use to play the game.

* Open Powershell (or your favorite command-line prompt), and type the following to start playing:

```
cd C:\Folder\Where\Everything\Is\Installed
.\SlipNFrag-PXCR.exe -basedir "C:\Program Files\Steam\steamapps\common\Quake" -game alkaline +map start
```
> (Standard command-line options and rules from the game still apply. Feel free to experiment as you wish with custom maps / mods.)

## Game controller bindings

Thw Win64 and MacOS releases of Slip & Frag can be played, in addition to mouse and keyboard, by using an "extended profile" game controller, such as the Xbox One controller (or similar controllers).

The (fixed) bindings of the game controller to play the game are the following:

* [**A**] : Jump / Swim up
* [**B**] : Swim down
* [**X**] : Hold to Run
* [**Y**] : Swim Up (also, quit the game upon menu)
* (**DPad**) : Arrow keys (move and turn in game, select and scroll in console and menu)
* [**Triggers**] : Attack / fire
* [**Shoulders**] : Cycle among weapons
* (**Left thumbstick**) : Move forward, backward, sidestep
* (**Right thumbstick**) : Turn left or right / look up or down (if mlook is enabled)
* [**Left thumbstick click**] : Reset look to center
* [**Right thumbstick click**] : Hold to enable mlook, release to disable
* [**Menu**] : Display game's main menu

When in the game menu, button [**B**] can also be used to move one menu back. Also, if it wasn't clear enough, in the Quit menu, button [**Y**] can be used to exit the game.

## What if I want to build the executables by myself

As the process of compiling and running Slip & Frag in a development machine can be a bit complicated, instructions are provided in [Build instructions](BUILD.md) to that effect.

## Privacy Policy

For details on how the engine handles user data, see [Privacy Policy](PRIVACY.md).

## What if I need to ask further questions

Visit [Support](SUPPORT.md) if you need some help on how to run or compile Slip & Frag.