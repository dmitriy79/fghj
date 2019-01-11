
Debian
====================
This directory contains files used to package helpforcancerd/helpforcancer-qt
for Debian-based Linux systems. If you compile helpforcancerd/helpforcancer-qt yourself, there are some useful files here.

## helpforcancer: URI support ##


helpforcancer-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install helpforcancer-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your helpforcancer-qt binary to `/usr/bin`
and the `../../share/pixmaps/helpforcancer128.png` to `/usr/share/pixmaps`

helpforcancer-qt.protocol (KDE)

