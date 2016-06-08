This directory contains a set of POSIX-compatible header files.
A quick hack to compile Linux-oriented software.
It was used to succesfully compile Nano-X and DirectFB.

have to inlude-path ./posix/uos - to override some functionality of [std]libc/m : 
	math/cmath takes from libm by default, and need override to use from libuos