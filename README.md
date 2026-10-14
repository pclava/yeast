# yeast : a MIPS kernel
**a WIP (work in progress) !!!!!**

---
yeast is a small kernel built to run on my MIPS emulator. It heavily follows [xv6](https://github.com/mit-pdos/xv6-riscv), but is in no way a "reimplementation" of xv6 or any Unix system. I have taken many creative and lazy liberties.

I am building yeast as an accompaniment to my Operating Systems course, and as such the project is constantly being built on top of itself.

yeast will _not_ run on the most recent release of MIPSEmulator. Many changes had to be made to support a kernel, and I will publish the modified emulator as soon as I get the file system implemented in yeast. In the meantime, you'll have to trust me that the kernel works. When I release a working emulator, I'll also add the Makefile and shell script I use to build the project.

Eventually I'll write some documentation, maybe. In my defense the code is heavily commented! Write to me if you would like anything explained.