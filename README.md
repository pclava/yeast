# yeast
**a WIP (work in progress)**

yeast (all lowercase – this is important) is a small kernel built to run on my MIPS emulator. It _heavily_ follows [xv6](https://github.com/mit-pdos/xv6-riscv), but is in no way a "reimplementation" of xv6 or any Unix system. I have taken many creative and lazy liberties.

I am building yeast as an accompaniment to my Operating Systems course, and as such the project is constantly being built on top of itself.

yeast will _not_ run on the most recent release of MIPSEmulator. Be patient!

There is some documentation and general OS discussion in the Wiki section of this repo. It is cursed to perpetual incompletion, so write to me if you would like anything explained.
# Building
To build, you need:
- A clang instance supporting the **mipsel** target (MIPS little-endian). The default in the Makefile is currently the one on my own machine. To build, set `CC` to your own compiler when you run `make` (eg. `make CC=clang` if your native clang instance supports mipsel).
  - `clang -print-targets | grep mipsel` to check if your clang instance supports mipsel
- My [assembler and linker](https://github.com/pclava/MIPSAssembler) with the names `MIPSAssembler` and `MIPSLinker` in the same path as `build.sh`
- The **kernel** version of my [emulator](https://github.com/pclava/MIPSEmulator) with the name `MIPSEmulator`

The Makefile compiles the files into MIPS assembly, then calls `build.sh`. This script runs the assembler and links to create executables for the kernel (in `kernel.out`) and all user programs. These are in `/userroot`, prefixed with `_` to avoid confusion with host programs. The prefix is stripped during loading.

The build script finally runs `mkfs` on the directory `/userroot` to build the file system.

## Running
To run the kernel:

``
./MIPSEmulator kernel.out disk.img
``

replacing `kernel.out` if you've changed the name of the kernel executable and `disk.img` if you've changed the name of the disk image.

## mkfs
The emulator uses a disk image file to emulate the hard disk. `mkfs` creates a disk image pre-loaded with host files from a given directory. The build script calls mkfs with:

``
./mkfs disk.img userroot
``

which will create a file system rooted at `userroot` (**on the host machine, not the emulated kernel**), and will recursively travel the entire directory, loading every file and directory therein onto the disk image `disk.img`.

One could imagine placing the `/etc`, `/bin`, and other such directories in `/userroot` if one wanted to build out a more complete operating system.

---
Every line of code in this repository was written by a human, though sometimes not me (some standard library code is adapted).
