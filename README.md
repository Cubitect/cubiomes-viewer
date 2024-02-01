# Cubiomes Viewer

Cubiomes Viewer provides a graphical interface for the efficient and flexible
seed-finding utilities provided by [cubiomes](https://github.com/Cubitect/cubiomes)
and a map viewer for the Minecraft biomes and structure generation.

The tool is designed for high performance and supports Minecraft Java Edition
main releases up to 1.20.


## Download

Check the [Releases](https://github.com/Cubitect/cubiomes-viewer/releases)
section on GitHub for Linux and Windows builds.

A Flatpak for the tool is available on
[Flathub](https://flathub.org/apps/details/com.github.cubitect.cubiomes-viewer).

For Arch Linux users, the tool may be found in the
[AUR](https://aur.archlinux.org/packages/cubiomes-viewer) thanks to
[JakobDev](https://github.com/JakobDev).

Non-PC platforms, such as macOS, are not formally supported, but you can check
[here](https://github.com/Cubitect/cubiomes-viewer/issues/107) for more
information on this issue.


## Build from source

Build instructions can be found in the [buildguide](buildguide.md).


## Basic feature overview

The tool features a map viewer that outlines the biomes of the Overworld,
Nether and End dimensions, with a wide zoom range and with toggles for each
supported structure type. The active game version and seed can be changed
on the fly while a matching seeds list stores a working buffer of seeds for
examination.

The integrated seed finder is highly customizable, utilizing a hierarchical
condition system that allows the user to look for features that are relative to
one another. Conditions can be based on a varity of criteria, including
structure placement, world spawn point and requirements for the biomes of an
area. The search supports Quad-Hut and Quad-Monument seed generators, which can
quickly look for seeds that include extremely rare structure constellations.
For more complex searches, the tool provides logic gates in the form of helper
conditions and can integrate Lua scripts to create custom filters that can be
edited right inside the tool.

It is also possible to find Locations in a fixed seed. In this mode, the 
conditions are checked against a list of trial positions instead of the
world origin. Each location that passes the conditions is then collected
with additional information on where each individual condition was triggered.

An analysis of the biomes and structures can be performed in their respective
tabs. This provides information on the amount of biomes and structures that
are available in an area, as well as their size and positions.


## Screenshots

Screenshots were taken of Cubiomes Viewer v4.0.

![seeds](etc/screenshot_seeds-fs8.png
"Searching for a quad-hut near a stronghold with a good biome variety")

![locations](etc/screenshot_locations-fs8.png
"Locations in a given seed while viewing the world's height map")

![structures](etc/screenshot_structures-fs8.png
"Examining structures in the nether")


## Known issues

Desert Pyramids, Jungle Temples and, to a lesser extent, Woodland Mansions can
fail to generate in 1.18+ due to unsuitable terrain. Cubiomes will make an
attempt to estimate the terrain based on the biomes and climate noise. However,
expect some inaccurate results.

The World Spawn point for pre-1.18 versions can sometimes be off because it
depends on the presence of a grass block, that cubiomes cannot test for.


## Legal information

The main code is under the GPLv3, see [LICENSE](LICENSE), while other
components are released under their respective author licenses:

- Biome and structure generation from cubiomes, licensed under MIT.
- Cross platform [Qt](https://www.qt.io/licensing) GUI toolkit, available under (L)GPLv3.
- Dark Qt theme derived from [QDarkStyleSheet](https://github.com/ColinDuquesnoy/QDarkStyleSheet), licensed under MIT.
- Biome colors and icons are inspired by [Amidst](https://github.com/toolbox4minecraft/amidst), licensed under GPLv3.
- [Lua](https://www.lua.org/license.html) is distributed under the terms of the MIT license.

NOT AN OFFICIAL MINECRAFT PRODUCT.
NOT APPROVED BY OR ASSOCIATED WITH MOJANG OR MICROSOFT.


