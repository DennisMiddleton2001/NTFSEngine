# FSReflector
NTFS File System Reflector

This DLL exports functions that allow low-level analysis of the NTFS file system without mounting the volume.  It works entirely on DASD and bypasses all file system security.  This also allows analysis of VSS differential shadow copies that wouldn't normally be accessible through conventional tools.  This is the equivalent of file system God Mode!

The DLL code included in this repo exports file system structures in a very organized format.  You could write a simple command line utility that is linked to it and export all of the functionality.  Here's an example of the output on a GUI application (batteries not included), but if you're interested in making any size donation, I would gladly send you the full application sources and a license.

https://www.youtube.com/watch?v=J6JS_sg71ok
