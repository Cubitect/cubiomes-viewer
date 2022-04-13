# Cubiomes Viewer Build Instructions

Cubiomes Viewer is a Qt5 application and requires:
* Qt5.9 or newer (Qt6 is not supported) and a
* GNU C++ compiler (GCC or Clang).

The cubiomes library is included as a submodule to this repository.


## The Compiler

You should have either GCC or Clang installed.

Windows users can use MinGW, which can be installed together with Qt, using the Qt Installer.


## Get Qt5

Depending on your operating system you may have several options for installing Qt5,
but [Using the Qt Installer](buildguide.md#using-the-qt-installer) should be the most general method.
Many Linux distros already provide Qt in their repositories and you can also
[Get Qt With Your Package Manager](buildguide.md#get-qt-with-your-package-manager) instead if you wish.

Note: for a static build you will have to compile Qt yourself.


### Using the Qt Installer

You can use the [Qt Online Installer](https://www.qt.io/download-qt-installer) if you have a Qt account or don't mind creating one.
It is also possible to use the [Qt Offline Installer](https://www.qt.io/offline-installers) without an account,
but you have to disconnect from the internet when you launch the installer, otherwise it will require a Qt account again, which is a little irritating.

Choose an installation path without spaces.

The only required component is the Qt5 base install for your compiler.
(Make sure to select a compiler which supports GNU extensions, such as MinGW.)

I would also recommend installing QtCreator, as well as MinGW on Windows from the Developer Tools.


### Get Qt With Your Package Manager

##### Debian
```
$ sudo apt install build-essential qt5-default
```
##### Fedora
```
$ sudo dnf install qt5-qtbase-devel
```
##### macOS
```
$ xcode-select --install
$ brew install qt@5
$ brew link qt@5
```


## Get the Sources

The cubiomes-viewer repository includes the cubiomes library as a submodule and requires a recursive clone:
```
$ git clone --recursive https://github.com/Cubitect/cubiomes-viewer.git
```
If you have QtCreator you can now just open the `cubiome-viewer.pro` file and configure the project.

Alternatively you can manually prepare a build directory:
```
$ cd cubiomes-viewer
$ mkdir build
$ cd build
```
and build cubiomes-viewer:
```
$ qmake ..
$ make
```

#### Notes:

With some Qt installs it is `qmake-qt5` instead.
With MinGW the make command is something like `minwgw32-make` instead.
If the commands are not found, make sure that the Qt `bin` directory is in the `PATH` variable.
(The same applies to `C:/Qt/Qt15.12.12/Tools/mingw730_64/bin` or wherever your compiler is installed.)


