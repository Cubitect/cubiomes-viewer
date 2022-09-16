# Cubiomes Viewer

Cubiomes Viewer provides a graphical interface for the efficient and flexible seed-finding utilities provided by [cubiomes](https://github.com/Cubitect/cubiomes) and a map viewer for the Minecraft biomes and structure generation.

The tool is designed for high performance and supports Minecraft Java Edition main releases 1.0 - 1.19.


## Download

Precompiled binaries can be found for Linux and Windows under [Releases on Github](https://github.com/Cubitect/cubiomes-viewer/releases). The builds are statically linked against [Qt](https://www.qt.io) and should run as-is on most newer distributions. For the Linux build, you will probably have to add the executable flags to the binary.

A Flatpak for the tool is available on [Flathub](https://flathub.org/apps/details/com.github.cubitect.cubiomes-viewer).

For Arch Linux users, the tool may be found in the [AUR](https://aur.archlinux.org/packages/cubiomes-viewer) thanks to [JakobDev](https://github.com/JakobDev).

Non-PC platforms, such as macOS, are not formally supported, but you can check [here](https://github.com/Cubitect/cubiomes-viewer/issues/107) for more information on this issue.


## Build from source

Build instructions can be found in the [buildguide](buildguide.md).


## Basic feature overview

The tool features a map viewer that outlines the biomes of the Overworld, Nether and End dimensions, with a wide zoom range and with toggles for each supported structure type. It is simple to cycle through a list of seeds or to adjust the Minecraft version on the fly.

The integrated seed finder is highly customizable using a hierarchical condition system allowing the user to look for features that are relative to one another. Conditions can be derived from a large selection of criteria among structure placement, world spawnpoint and from the available biomes in an area. Filters can use logic gates in the form of helper conditions for even more control, as well as Quad-Hut and Quad-Monument seed generators to quickly look for seeds that include extremely rare structure constellations.

![maingui](etc/screenshot_maingui.png)


## Known issues

Desert Pyramids, Jungle Temples and, to a lesser extent, Woodland Mansions and Villages can fail to generate in 1.18+ due to unsuitable terrain. Cubiomes Viewer will make an attempt to estimate the terrain based on the biomes and climate noise; however, expect some inaccurate results.

The world spawnpoint for pre-1.18 versions can sometimes be off due to its dependence on the presence of a grass block, which Cubiomes Viewer cannot test for.


