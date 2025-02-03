# Slip & Frag

Slip & Frag is a sourceport of the Quake game engine, as created by [id Software](https://www.idsoftware.com/). 

Slip & Frag aims to be two things:

* An engine that allows you to play Quake, the game, as closely as it was when published in 1996 for the DOS platform. This means somewhat low resolution textures, low polygon count, colorful and high-contrast worlds, with sharp-edged, dangerous-looking creatures. That, and messages with large white (or brown) letters.

* An engine that runs in modern operating systems (MacOS 10.14 and later, Windows 8 and later, Meta Quest/2/3 v62 and later, and others), removing the limitations that restricted the original engine to run in a limited, fixed amount of memory, thus allowing it to accept the content created during these years by the user community.

## Downloads
>As of now, the download URLs point to the previous repository in Bitbucket - for the next release, the files will be available in this repository as a proper Release. 

### Meta Quest/2/3 (the **OXR** version):

Latest version: **1.0.27** - see [Changelog](CHANGELOG.md) for details.

* Android .apk file: [slipnfrag-release_1.0.27.apk](https://bitbucket.org/Izhido/slip-frag/downloads/slipnfrag-release_1.0.27.apk)
>*IMPORTANT*: Due to recent changes in the code, detailed in the Changelog for version 1.0.26, this version **requires** that your device is updated to the latest OS version (v62 or later in Quest 2 or later devices).

### Windows:

Latest version: **1.0.27** - see [Changelog](CHANGELOG.md) for details.

* Win64 .exe file: [SlipNFrag-Win64_1.0.27.zip](https://bitbucket.org/Izhido/slip-frag/downloads/SlipNFrag-Win64_1.0.27.zip)

### MacOS:

Latest version: **1.0.27** - see [Changelog](CHANGELOG.md) for details.

* MacOS .dmg file: [SlipNFrag_1.0.27.dmg](https://bitbucket.org/Izhido/slip-frag/downloads/SlipNFrag_1.0.27.dmg)
>*IMPORTANT*: Following recommendations from Apple for the configuration in the Xcode project, the minimum version of MacOS required is now *10.14.6*.

## Setup

Slip & Frag does not currently come with the game assets required to play it. You will need, instead, to provide the engine with those assets yourself.

Quake can be purchased from several venues (Steam, GOG, marketplaces within Amazon, and many others). There is also a shareware version of the game, hosted on many sites, which can also be used to play the game with Slip & Frag, if desired.

Once you acquire a copy of the game, you will find in there a folder called ID1, containing one or more files called pak0.pak, pak1.pak, and so on. That folder is what you'll need to play the game using Slip & Frag.

*IMPORTANT*: Slip & Frag does not currently support the .pak files from the Quake re-release, issued for the 25th anniversary of the game. The original .pak files, however, can still be found in the folder where the new game is installed.

### Windows

* Copy the ID1 folder, described above, anywhere you wish in your machine.

* Extract and copy the Win64 executable in that same folder - meaning, the executable and the ID1 folder should be now at the same level in that folder.

* Double click on the executable, and press the Play button to start.

### MacOS

* Copy the ID1 folder, described above, anywhere you wish in your machine.

* Open the app, and click on the small Settings icon at the bottom right of the window (alternatively, you can also go to Preferences in the menu).

* At the "Game directory (-basedir)" prompt, enter the path to the ID1 folder you just copied.

* Configure the other options as you see fit (optional, as they're not required to play the original game).

* Click on the Close button.

* Click on the large Play button to start playing the game.

### Meta Quest/2/3

* Ensure that you have Developer mode enabled for your device. (This will take some extra steps through several places in your device AND the Oculus developer website. You'll only have to do this once, though. See  https://learn.adafruit.com/sideloading-on-oculus-quest/enable-developer-mode for details).

* Install the .apk file in your device, using either:
    * ADB (See https://www.vrtourviewer.com/docs/adb/ for details);
    * Android Studio (https://developer.android.com/studio);
    * SideQuest (https://sidequestvr.com/setup-howto);
    * Your favorite .apk installation tool.

* Start Slip & Frag from your device *once*, from the Unknown sources screen or tab. When you do this, the following folder will be automatically created in your device:

```
/sdcard/android/data/com.heribertodelgado.slipnfrag/files
```

* Use any file transfer app (Windows Explorer, or SideQuest if Explorer is unavailable) to copy the ID1 folder, described above, to that exact location.

* Exit, and then restart, Slip & Frag in your device. Press A + X in your controllers (as indicated in the visor) to start playing.

## Game controller bindings

Slip & Frag can be played, in addition to mouse and keyboard, by using an "extended profile" game controller, such as the Xbox 360 or Xbox One controllers (or other similar controllers).

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

>(Please notice that this applies to the *desktop* versions of Slip & Frag - the OXR version uses its own controller schema.)

## Customizations

Slip & Frag also allows you to play custom game modifications (mods). These mods exist virtually since the game was released in 1996, and there is an user community still producing new and exciting content for the game, which are totally worth checking out.

Slip & Frag will gladly accept new maps & mods intended to be run in the original ("vanilla") version of the game; there is also experimental (yet, fully functional) support for other mods that require more memory and resources from the game. (The only exception to this is maps / mods designed for the Quake re-release.)

Once you get familiarized with custom mods, and have read the setup instructions for them, if you wish to play them in your computer (or VR headset), you'll need to do the following:

### Windows

* Copy the folder(s) containing your mods into the same folder you created to host the ID1 folder. (Slip & Frag automatically assigns the ```-basedir``` option to be precisely that folder). Be sure to follow the instructions provided by the creator of the mod to set up everything correctly.

* Bring up either a Command Prompt or a PowerShell terminal, and type the command line that the custom mod / map specifies in its setup instructions, hit Enter, and then press the Play button.

### MacOS

* Copy the folder(s) containing your mods into the same folder you created to host the ID1 folder. (Slip & Frag automatically assigns the ```-basedir``` option to be precisely that folder). Be sure to follow the instructions provided by the creator of the mod to set up everything correctly.

* Open the app, and click on the Settings button (alternatively, you can also go to Preferences in the menu).

* At the "Additional command-line arguments" prompt, write down the command line with the options that your mod requires to run (most, if not all, mods *need* those additional options - consult the instructions of the mod to know how to proceed).

* Click Close on the Settings screen, then click the Play button to start playing your new mod.

### Meta Quest/2/3

* Copy the folder(s) containing your mods into the same folder specified above in Setup, which now should contain the ID1 folder with the original game assets. (Slip & Frag automatically assigns the ```-basedir``` option to be precisely that folder). Be sure to follow the instructions provided by the creator of the mod to set up everything correctly.

* Create a new text file in your computer, and give it the following name: 

```
commandline.txt
```

* In the new text file, write down the command line with the options that your mod requires to run (most, if not all, mods *need* those additional options - consult the instructions of the mod to know how to proceed).

* Using your favorite file transfer app, copy the new file to the folder specified above in Setup. This means your folder will now contain: the ID1 folder, the text filename, and the folders with your custom mod(s).

* Exit Slip & Frag (if it was opened), and restart it to start playing your new mod.

## Network play (Multiplayer mode)

Slip & Frag will also allow you to play with others, either through the Internet by using NetQuake servers (as opposed to QuakeWorld servers, which Slip & Frag does *not yet* support), or in your home WiFi (or LAN) network (again, as a NetQuake server / client).

Slip & Frag will allow you to do this in exactly the same way as the original game did, by entering the data required in the game itself (Menu / Multiplayer / Join or New Game / enter the address).

The Windows version will also allow you to specify connection parameters through the command line, if desired.

The Meta Quest/2/3 version will allow you to do this from the game menu, with the provided on-screen keyboard (available since version 1.0.7). If, however, you prefer to specify the data outside of the game menu, you can do so by following these steps.

To connect your device as a client (Join Game):

* Create a new text file with the following name (or open it if it already exists):

```
commandline.txt
```

* Add the following option at the beginning of the file:

```
-port 26000 -connect quake.spaceballcity.com
```

* (of course, replace the port number and the server name with the data from the server you wish to connect - also, the IP address of the local NetQuake server can be specified if you intend to play in your local WiFi or LAN network).

* Using your favorite file transfer app, copy the text file to the folder specified above in Setup, replacing it if needed. This means your folder will now contain: the ID1 folder, the text filename, and any folders with your custom mod(s), if you added any.

* Exit Slip & Frag (if it was opened), and restart it. Upon start, the engine will immediately attempt to connect to the server to begin playing.

To host a server from within your device, you do not need to do anything special - proceed with the Menu options of the game as usual, and a new local NetQuake server will be ready for others to join in your local network.

## Build instructions

As the process of compiling and running Slip & Frag in a development machine can be a bit complicated, instructions are provided in [Build instructions](BUILD.md) to that effect.

## Privacy Policy

For details on how the engine handles user data, see [Privacy Policy](PRIVACY.md).

## Support

Visit [Support](SUPPORT.md) if you need some help on how to run or compile Slip & Frag.