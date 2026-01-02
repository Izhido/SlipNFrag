# Build instructions

Slip & Frag is provided as a C++ project, itself a modification of the original C code of the Quake engine, with added constructs from other languages such as Objective C or Kotlin, used in the platforms that are supported. 

Source code for the project is provided [here](https://github.com/Izhido/SlipNFrag.git). Details for each supported platform are provided below.

### Windows (x64)

A Visual Studio 2022 solution file is provided at the root of the source code folder, called *SlipNFrag.sln*. The solution contains a single project, located at *SlipNFrag-Win64/SlipNFrag-Win64.vcxproj*. This is a Win32 project configured to run as a 64-bit application, that does not depend on any other APIs other than the ones that came with a Windows 10 or later machine. (A few uses have reported that this executable, as it is, can also be made to run in a Linux environment properly configured.)

To set up the environment to build, debug and test the project for the first time:

* Clone or download the latest version of stb (as found in https://github.com/nothings/stb ). Create a stb folder at the root source folder, next to the `SlipNFrag-Win64` folder. Ensure that the "stb_xxx.h" files are located at the root of the stb folder to ensure proper compilation.

* Open Visual Studio 2022 (or newer), then open the *SlipNFrag.sln* solution in the top folder.

* Compile and run the project as usual for Visual Studio to debug or test it.

To generate a Release build of the project, ensure that you selected the Release configuration in the IDE, then right-click on the project file - Publish - Create app packages, and follow the prompts.  

### Windows PCXR

The code is provided as a CMake project, residing at `SlipNFrag-PCXR` folder from the root of the source code folder. This project creates a console-based Windows executable that uses both OpenXR and Vulkan to perform the rendering of the game. The OpenXR runtime used is the one provided by Meta through their own Meta Quest Link application, that connects your VR device to the executable and plays the game through the runtime into your device.

To set up the environment to build, debug and test the project for the first time:

* Download the latest version of the [OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK/releases/latest). Copy it to a folder at the root source folder, next to the `SlipNFrag-PCXR` folder.

* Open the newly downloaded OpenXR SDK in Visual Studio (2022 or newer), configure the CMake project and run the `Install OPENXR` target (both Debug and Release mode). Close Visual Studio.

* Open `SlipNFrag-PCXR` in Visual Studio, and configure CMake.

* Go to Debug - Debug and Launch Settings for... to specify the command-line arguments for the project. (Standard command-line rules for the game apply; -basedir, -game, +map, the usual commands are in effect as expected.)

* Locate and compile (or run) the `slipnfrag-pcxr.exe` target from within Visual Studio.

Generating a Release build of the project is as simple as choosing the "Release" mode of the project, and compiling the project again. Make sure to copy also the PNG assets and the `shaders` folder created when compiling the project, and packaging them as part of the new release.

### MacOS

You will find a project package called *SlipNFrag.xcodeproj* at the root of the source code folder. Open this package using Xcode 12 or newer. The source files that this project references are in the *SlipNFrag-MacOS* folder - or, if you intend to build the experimental iOS target, in the *SlipNFrag-iOS* folder. 

The project itself has 3 targets:

* One with just the source code of the engine and minimal, almost empty porting helper .cpp files (**SlipNFrag**);

* One with all the sources required to build the MacOS version (**SlipNFrag-MacOS**);

* One with all the sources required to build the experimental iOS version (**SlipNFrag-iOS**).

To set up the environment to build, debug and test the project for the first time, for either target:

* Clone or download the latest version of stb (as found in https://github.com/nothings/stb ). Create a stb folder at the root source folder, next to the `SlipNFrag-MacOS` and `SlipNFrag-iOS` folders. Ensure that the "stb_xxx.h" files are located at the root of the stb folder to ensure proper compilation.

* Open Xcode 12 (or newer), then open the *SlipNFrag.xcodeproj* project in the top folder.

* Choose your target (MacOS or iOS) from the Target selector at the top of the main Xcode screen.

* Compile and run the project as usual for Xcode to debug or test it. For MacOS, pressing the Play button at the top of the screen should suffice. For iOS, you will need to setup the Signing configuration of the project with your own Apple Developer account before compiling.

To generate a Release build of either the MacOS or iOS target, ensure that your target's schema has the Build Configuration of its Run settings in Release, instead of Debug, mode. After that, generate an Archive of the project, follow the prompts if required, then Distribute App / Copy App in Organizer.

In the particular case of the MacOS executable, the .dmg file that is used to distribute the project can be generated from the Disk Utility in MacOS. Follow the instructions in "Create a disk image from a folder or connected device" specified [here](https://support.apple.com/guide/disk-utility/create-a-disk-image-dskutl11888/mac) to do so.
 
### Meta Quest/2/3/Pro/3s

Setting up the environment to debug, test or create a release of the Meta Quest, Quest 2 and Quest 3 versions involves more steps than the versions above, due to the amount of external components involved in the compile & release process.

The XR version is an Android Studio project that can be found at `SlipNFrag-XR` folder from the root of the source code folder. The minimum required version of Android Studio is version *2024.1.1*, due to recent updates in the project components. The project itself is a CMake-backed native activity project, configured with special settings that allow it to run in standalone VR headsets such as the Meta Quest/2/3/Pro/3s.

The following is the list of components, and their version numbers, required to build the Android project, as of this writing:

* Android SDK Platform **16.0** (R) API level **36.1**

* NDK (Side by Side) **29.0.14206865**

* Android SDK Build-Tools **36.1.0**

* OpenXR SDK **1.1.54**

* CMake **4.1.2** (or later)

* stb (latest version gathered from https://github.com/nothings/stb )

* Minizip (latest version gathered from https://github.com/domoticz/minizip )

* lhasa **0.5.0** (or later, gathered from https://github.com/fragglet/lhasa )

> (Versions for other components can be checked in *Project Structure* in the Android Studio project.)

To set up the environment to build, debug and test the project for the first time:
> (There used to be a dependency on the Oculus OpenXR Mobile SDK for building. This dependency was removed, and now all it is needed is Khronos OpenXR SDK.)

* Ensure that your VR headset is in Developer mode, and that it can run applications from Unknown sources. (See the [Readme](README.md) for details).

* Download the latest release of the OpenXR SDK (as stated above). Copy the contents of the *OpenXR-SDK-release-xxx* from the .zip file at the root source folder.
>(If the name of the folder from the OpenXR SDK references a newer version than the specified above, modify the path to the SDK specified in `SlipNFrag-XR/app/src/main/cpp/CMakeLists.txt` file and change it so it points to the new version.)
 
* Clone or download the latest version of stb (as stated above). Create a *stb* folder at the root source folder, next to the *OpenXR-SDK-release-xxx* folder. Ensure that the "stb_xxx.h" files are located at the root of the *stb* folder to ensure proper compilation.

* Clone or download the latest version of Minizip (as stated above). Create a *minizip* folder at the root source folder, next to the *stb* folder. Ensure that "ioapi.c" and "unzip.c" files are located at the root of the *minizip* folder, along with all other files. 

* Clone or download the version above mentioned of lhasa. Create a *lhasa-x.x.x* folder (with x.x.x representing the version above) at the root source folder, next to the *minizip* folder. Ensure that the "lib" folder is located at the root of the *lhasa-x.x.x* folder, along with all other files.

The folder structure, so far, should look like this:

```
/path/to/SlipNFrag/
   OpenXR-SDK-release-x.x.x/
      ...
   stb/
      stb_vorbis.c
      ...
   minizip/
      ioapi.c
      unzip.c
      ...
   lhasa-x.x.x/
      lib/
         public/
         ...
      ...
   id1
   renderer
   ...
```

* Open Android Studio 2024.1.1 (or newer), then open the project in the `SlipNFrag-XR` folder. Wait for Gradle to finish configuring the environment for the project, and follow the prompts if instructed to do so.
>(Check that your environment has the components described above, with their respective versions.)

* Locate the Build Variants tab in Android Studio, ensure that you have selected the following:
    - Module: **:app**
    - Active Build Variant: **debug**
    - Active ABI: **arm64-v8a**

* Ensure that your device is connected and visible in Android Studio (look for mentions of the device name in the top bar of the IDE), and then press the Play button to run & test the project, or the Debug button to debug it.
>IMPORTANT: Be sure to follow the instructions in the [Readme](README.md) to copy the game assets to your device to be able to play the game as expected.

If you need to enable the Vulkan Validation Layers for this project, grab the latest version of them from [the official source](https://github.com/KhronosGroup/Vulkan-ValidationLayers) and copy them into a new `SlipNFrag-XR/app/src/main/jniLibs` folder. **Do not** attempt to link the libs in `CMakeLists.txt` - this will simply make the executable to refuse to start.

To generate a Release build of the project, in .apk form, do the following:

* Unless you absolutely need them, remove the Vulkan Validation Layers from the project, by removing the `SlipNFrag-XR/app/src/main/jniLibs` folder specified above.

* If you haven't done so, create an *android.debug.keystore* file to sign your app. The file can be generated from the Oculus OpenXR Mobile SDK folder, by doing the following:
    - Windows: from a command prompt, run `ovr_openxr_mobile_sdk_xxx\bin\scripts\build\ovrbuild_keystore.py.bat`. NOTE: You will need Python properly configured in your system to do this.
    - MacOS and others: open a terminal, and run `python3 ./ovr_openxr_mobile_sdk_xxx/bin/scripts/build/ovrbuild_keystore.py`.

* Locate the Build Variants tab in Android Studio, ensure that you have selected the following:
    - Module: **:app**
    - Active Build Variant: **release**
    - Active ABI: **arm64-v8a**

* From Android Studio, do any of the following:
    - Either, go to *Build* - *Build Bundle(s) / APK(s)* - *Build APK(s)*, then follow the prompts;
    - Or, go to *Build* - *Generate Single Bundle / APK*, fill whatever data is needed (including the newly generated android.debug.keystore file, keys and passwords) and follow the prompts.
>(Slip & Frag currently supports APK generation using Signature Version V2 - Full APK Signature.)

## Support

If you need additional help to perform these steps, see [Support](SUPPORT.md) for your current options.