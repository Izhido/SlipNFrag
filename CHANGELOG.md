# Changelog

## Version **1.0.28**:
* Migrated repository to GitHub. Downloads are now exposed as Releases in the repository.
* Reorganized README to explain more clearly what to do with the application(s).
* Rebuilt OXR version as a GameActivity-based application, to make it easier to get SDK updates.
* Official release of the PCXR version (a version that runs as a PCVR executable using OpenXR) to the general public.
* Improved lightmap texture handling in the OXR and PCXR version, to increase performance a bit.
* Removed almost all visual artifacts appearing in large mods / maps due to the OXR and PCXR renderer incorrectly skipping some surfaces, helped greatly by the reworked lightmap code above.
* Fixed incorrect creation of the model and sound precache in the core engine, which in rare occasions prevented the Copper mod (and the UDOB episode) from starting correctly.
* Fixed incorrect handling of data related to particle and sound-effect instancing using the expanded network protocol, which caused some large maps (as well as any modified maps from the original game) to crash under pressure.
* Fixed race condition between sound data processing and the sound cleanup code after switching to another map, to avoid a potential crash just before loading the map.
* Fixed race condition between the code that releases lightmaps no longer in use when rendering, and the code that disposes them after switching to another map, which caused a crash just after loading the map.
* Other minor optimizations (and a few more bug corrections) to the OXR and PCXR renderer were applied as well, to improve performance a bit.

### Version **1.0.27**:

* Fix to sound code that mishandled .wav files with an invalid loop start value that crashed the core engine of the application.
* Fixed storage of alias model names within the core engine, to allow names with exactly 16 characters to be handled properly.
* Fixed code that frees entities in the core engine after finishing a map, to restore properly free space for new entities.
* Increased fixed value for the execution runaway loop to 0x1000000, to accommodate for maps with large amounts of content.
* Maps containing surfaces whose extents are declared as zero or negative, will no longer have those surfaces rendered - fixing a crash in one recently created map.
* Fixed bug in the core engine that used outdated FOV values in the renderer after the screen changes its size.
* Transparency in areas of the player weapon that are close to the player itself, in the OXR and the experimental PCXR version, will now use dithered transparency, instead of blending transparency - this prevents skyboxes from bleeding into the transparent areas.
* Fixed ugly bug in sky rendering for the OXR version, related to invalid usage of FOV values, which disallowed the sky to cover the expected area.
* The core engine will now attempt a protocol bump to the expanded version if it detects that the alias models in a map go beyond the original limits in 3D space for the engine.
* Fixed bug that occurs when resampling large sound files, due to the code not using variables with large enough bit sizes.
* Applied patch to code that handles shared edges in the renderer of the core engine, to disallow edges having more than 2 shared surfaces - this caused some walls in recent modern maps to disappear completely when viewed from a certain angle.
* Reverted order of vertices when rendering the controllers in-game - this allowed us to restore the back-face polygon culling code, in addition to the fix mentioned above.
* Some of the new code that was introduced a long time ago, it was found to be not really needed, so an effort was made to restore the original code from the core engine in several places, while still maintaining the idea of letting the application to allocate memory dynamically. This should allow people to do comparisons between the original and the current code more easily.


### Version **1.0.26**:

* BREAKING CHANGE: The OXR version will now use the Khronos OpenXR loader, instead of the proprietary one by Meta. This means that, due to restrictions imposed by Meta in their devices, the app will now load only if your device is updated to v62 or later.
* Face culling has been disabled in the OXR version, in order to avoid walls and other surfaces to disappear unexpectedly in a few recent maps and mods.
* Fixed internal server/client protocol used for large maps, to prevent the location of the player to "wraparound" in the map as if it jumped from one side of the map to the opposite side.
* Fixed bug in handling of audio and music play, which when changing levels, would make the engine crash due to audio data unexpectedly absent from the level change.
* Lightmap data will no longer be generated for very, very large surfaces in a few modern maps, which due to their size would simply freeze the engine, not allowing to continue playing the game.
* Two new optimizations to the core engine were blatantly cop- er, ported from Quakespasm / IronWail in order to speed up calculations of very large entities in modern maps (thanks to Andrei Drexler for providing the optimizations).
* Other minor performance optimizations applied to code that help determine surface visibility, to speed up things a bit.

### Version **1.0.25**:

* Fixed concurrent access issue when sending messages to print to the console (using Sys_Printf()), which made frequent crashes when in the v63 update of Meta Quest devices.
* Splash screen added for the OXR version, following recommendations from Meta. It also had the unintended side effect of displaying the application's icon in the most recently used list in the headset (yay!).
* Fixed incorrect cleanup of returned values when COM_FindFile() fails, which made the core engine crash if a specific, requested demo file is not found (instead of printing a message and fail gracefully).
* A few minor corrections and (oh so slight) optimizations applied to the renderer code from the OXR version.

### Version **1.0.24**:

* Recompiled OXR version with the latest SDKs and library versions, and confirmed that it works in the Meta Quest 3 headset, at 120Hz.
* Fixed embarrasing bug related to lit turbulent surfaces (such as lit water) not being rendered at all since the last build of the OXR version.
* Fixed data loading icon being permanently stuck on screen on the OXR version.
* Minor optimizations applied to the shaders in use by the OXR version to make it a bit more performant.
 
### Version **1.0.23**:

* Game controller support has been (finally!) added to the MacOS and Windows (Win64) versions, for controllers that follow the Xbox 360 or Xbox One form factors (other controllers can also be used, as long as they match the "extended" profile for game controllers).
* Both MacOS and Win64 versions will have the core engine running in its own thread, separate from the rendering code (as it was previously implemented for the OXR version), thus preventing the applications from being frozen for too long without displaying anything on screen.
* The classic disk loading indicator from the game is now shown every time the MacOS and Win64 versions are loading game assets, instead of freezing the application during that short time.
* Fixed a race condition regarding thread locking when initializing sound in the MacOS version, preventing the application from getting stuck if something goes wrong upon initializing the sound system.

### Version **1.0.22**:

* World surfaces in custom maps will now have colored lights applied to them - previously, all lighting was white only. Any map will load colored lights as long as there is a .lit file next to it, containing the new color information.
* Minor fix applied to "signon" messages that allow game clients to initialize, thus allowing recent large maps to be loaded (instead of just rejecting it due to "overflow").
* As usual, minor additional optimizations were also applied to make the engine a bit more performant.

### Version **1.0.21**:

* Improvements in multithreading code in the OXR version allowed for a noticeable increase in performance in maps and mods with lots of content.
* Fixed bug when rendering at certain angles in the OXR version, when the map or mod forces a change in location or viewing direction (for example, at the beginning of Arcane Dimension's Tears of the False God).
* Fixed a weird rendering issue where world surface texels were split in 2 or more pieces, showing more than one color on screen due to lighting, making the final result incompatible with the way the original game is presented on screen. 
* Other miscellaneous fixes in the renderer of the OXR version which improved, bit by bit, the general performance of the application.
* Fixes in the cleanup process of the OXR version when shutting down because of an app switch, which in rare occasions were preventing correct resuming of the game.
* Fixed broken compilation for the MacOS version due to one missing file. Also, copied one minor code improvement originally in the OXR version into the MacOS one, to improve application stability over memory.

### Version **1.0.20**:

* World surfaces in the OXR version can now have their textures loaded from external sources, either from a "textures" folder next to the game data, or directly from .pak files.
* A few minor optimizations were applied to improve performance - faster generation of mipmaps, further consolidation of texture data copy calls to the GPU, using triangle strips instead of separate triangles to render geometry in some special cases, and so on.
* Fixed bug in surface lightmap loading code that copied more data than needed (potential buffer overflow).
* Minor reworking of the game engine loading code in the OXR version to speed things up a bit when initializing rendering pipelines and similar structures.

### Version **1.0.19**:

* For the OXR version, the player weapon will no longer appear besides the player, or even behind - it will be forced to be in front of the player at all times, as per the game's specification. (This means that, sometimes, projectiles will not be seen coming exactly at the muzzle of the weapon - but if the player recenters itself, or walks again at Home's center, this won't be an issue.)
* Similarly, a fix was applied to the location of the weapon when in "screen" mode (looking at demos or during intermissions or w/e), in the OXR version, which will now appear always at its correct location on screen.
* Fixed bug about weapon not being able to be fired for the first time when the game controls screen is shown (and, sometimes, even if it doesn't appear).
* A few minor stability fixes regarding both the underlying OpenXR and Vulkan runtime usage.
* If the game is interrupted by another app taking control (such as the Oculus Guardian showing up), the app will no longer be removed from memory - after the new app finishes, the game should resume exactly as it was before.
 
### Version **1.0.18**:

* Minor optimization applied to rendering engine in the OXR version - world surface textures are now loaded in arrays, allowing the renderer to better compact draw calls and thus rendering a bit faster than before.
* The area covered by the menu, console and the status bar in game is now a bit smaller, allowing the player to see more info through a quick glance, without reducing visibility significantly.
* Fix regarding visual effects lingering after the current map/level is unloaded, first found in Quakespasm and other source ports.
* Fix parsing of demo files to allow reading extra whitespace - discovered that a specific demo could not load its audio track because of the unexpected whitespace in its data. First found in a commit sent to Ironwail.

### Version **1.0.17**:

* Fixed bug in audio engine, that prevented supersampling of sound effects to be properly interpolated for a higher audio quality.
* Fixed bug related to the -path parameter which detected filename extensions incorrectly, making the engine fail when loading pakfiles through -path.
* The MacOS version will now use also the new floating-point audio engine to improve audio quality.
* Fixed known bug in the core engine that made the audio engine overwrite parts of sound effects if they are marked as looping sounds.

### Version **1.0.16**:

* Switched to a new audio processing code (floating-point based) in the OXR version, removed limitation on the number of simultaneous sound effects, and applied a few optimizations to improve audio quality.
* Restored original mipmap loading code for world surfaces, and implemented a new interpolation method to display textures in far away surfaces in the OXR version to improve rendering and diminish eye strain in VR displays.
* Displaying far away fences will now reduce the size of the holes, instead of increasing it, to avoid displaying completely empty areas when the player moves far away from them.
* Vastly improved GPU memory handling in the renderer of the OXR version to use significantly less memory than before, therefore using less resources from the VR device.
* Fixed parsing of the entities data in custom maps, to avoid a potential crash when loading skyboxes in some specific maps, containing non-world entities that somehow also specified skyboxes.
* Added parsing of semicolon comments in metadata specified by custom maps, which was detected to be used in several custom mods (such as Arcane Dimensions), which displayed bogus warning messages because of said undetected comments.
* Fixed crash that happened at random moments when starting the app, or loading a new level/map, due to an incorrect checking of potentially-visible surfaces being performed without regards of whether it is in the engine's server or client code. 
* Other miscellaneous optimizations applied to the OXR version to make it more performant.

### Version **1.0.15**:

* Maintenance release - improved rendering performance by fixing an embarrassing bug where a cleanup procedure was being performed thousands of times per frame, instead of only once as expected. Also, a few minor optimizations were applied to improve performance a tiny bit.

### Version **1.0.14**:

* Support for lit water was added to the core engine, the MacOS and the OXR version. Lit water allows "turbulent" surfaces, such as water or slime, to reflect lighting in the same way regular world surfaces do.
* Fixed unexpected error that crashed the core engine when no -basedir argument is specified to it.
* Fixed issue with the number parsing code that would cause data from custom maps to be incorrectly loaded due to extra whitespace, with unexpected side effects during gameplay.
* Fixed embarrassing typo when reporting an error regarding the protocol version of maps that cannot be loaded into the core engine.
* A few optimizations in the OXR version will allow previously unloadable custom maps to be playable as expected.

### Version **1.0.13**:

* Maintenance release with minor improvements in performance for large maps or mods.

### Version **1.0.12**:

* Fixed crash originated by game saves not being loaded properly when they were generated from large maps - the ones that required the BSP2 format to be played.
* Fixed issue with audio tracks not being properly loaded if a change in the track number is detected, but its respective file is not found - the engine wrongly thinks the track was not unloaded, and it refused to play any more tracks.
* Fixed issue with a few skybox images being loaded upside down - a special attribute in the TARGA file format that indicated the image pixel rows are inverted was not taken into account (thanks to johnfitz, of FitzQuake fame, for the fix).
* Other minor improvements in performance for large maps or mods as well.

### Version **1.0.11**:

* Fix to the Field-Of-View (FOV) angle specified in the OXR version (for VR), which wasn't fully covering the visible area within the headset.
* Disabled a big portion of the old software renderer when in VR, instead sending all rendering data directly to the device, but only if the level being played is small enough to not saturate the device. This helped remove some ugly visual artifacts when playing the core game and the original expansions.
* Fixed a crash that happened in large maps, when the app attempts to update lots of interrelated entities in the game (thanks to Quake user community member @Spike for the fix).
* Fixed sky not appearing in some very simple maps in the OXR version, due to the absence of "submodels" in them.
* Other, miscellaneous fixes and performance improvements as well.

### Version **1.0.10**:

* Additional fix applied to the game load mechanism, which wasn't setting correctly the number of entities if no significant changes from a previous game were found.
* Fix to the creation of a "no texture" entry in the core engine, which potentially could crash the app when loading maps that have surfaces with no textures.
* Other minor improvements in performance for large maps / mods.

### Version **1.0.9**:

* General improvements in performance, especially in the Oculus Quest version, for large maps / mods.
* Fix applied to the game load / save mechanism, which prevented it from working also in large maps / mods.
* Fix in the Oculus Quest version related to skyboxes not appearing correctly when viewed in screen mode (map loading, intermissions).
* Potential fix for loading of very large maps which could cause a crash in the core engine if left unchecked.
* Two fixes which were originally discovered and patched in the Quakespasm sourceport, related to "fat PVS" checks for visible surfaces, as well as a buffer overflow for maps with a large amount of lights.
* Fix to the light maps generation code that caused "banding" in previous versions.
* A few other minor fixes were applied, as well.