# ZEQ2-Lite "Overhaul" Fork
This fork of ZEQ2-Lite was set up to make my changes available to everyone.

After about two years of removal, this fork is back online, although extremely messy and adding a few bugs here and there from unfinished changes. VM deployment has been dropped altogether in favor to dynamic libraries-only modules, and the code is expected to be compiled to C99 or later (eventually C11 or later). The tools folder with the integrated compile environment and various other tools were removed due to a bloat problem after trying to install MSYS2/MinGW-W64. Until a lightweight solution is found, it is required that you download a compiler suite and target the Makefile. If any problem is encountered while trying to compile, don't forget to open an issue ticket.
