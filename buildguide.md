# Cubiomes Viewer Build Instructions

Cubiomes Viewer is a Qt5 application and requires at least Qt5.9 and a GNU C++ compiler (GCC or Clang).


# Get a Compiler

You should have either GCC or Clang.
If you are on Windows you can use MinGW, which can be installed together with Qt, using the Qt Installer.


# Get Qt5

Depending on your operating system you may have several options for installing Qt5,
but [Using the Qt Installer] should be the most general method. Many Linux distros
already provide Qt in their repositories and you can refer to
[Get Qt from Your Repository] instead if you wish.

Note: for a static build you will have to compile Qt yourself.


### Using the Qt Installer

If you have a Qt account or want to create one, you can use the [Qt Online Installer](https://www.qt.io/download-qt-installer).
It is also possible to use the [Qt Offline Installer](https://www.qt.io/offline-installers) without an account,
but you have to disconnect from the internet when you launch the installer, otherwise it will require a Qt account again, which is a little irritating.

Choose an installation path without spaces.

Only the Qt5 base install for your compiler is required.
(Make sure to select a compiler which supports GNU extensions, such as MinGW.)

I would also recommend installing QtCreator and MinGw on Windows.


### Get Qt from Your Repository

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


# Get the Sources

The cubiomes-viewer repository includes the cubiomes library as a submodule and requires a recursive clone:
```
$ git clone --recursive https://github.com/Cubitect/cubiomes-viewer.git
```
If you have QtCreator you can now just open the cubiome-viewer.pro file and configure the project.

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
With a MinGW it is minwg
If the commands are not found, make sure that the Qt `/bin` directory is in the `PATH` variable.


