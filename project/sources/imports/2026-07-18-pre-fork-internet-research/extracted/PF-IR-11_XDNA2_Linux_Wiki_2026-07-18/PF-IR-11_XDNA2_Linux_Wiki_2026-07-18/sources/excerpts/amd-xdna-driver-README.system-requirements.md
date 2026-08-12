## Introduction
This repository is for supporting XRT on AMD XDNA devices. From this repository, you can build a XRT plugin DEB package.
On a machine with XDNA device, with both XRT and XRT plugin packages installed, user can start using XDNA device on Linux.

## System Requirements
To run AI applications, your system needs
* Processor:
  - To run AI applications (test machine): RyzenAI processor
  - To build this repository (build machine): Any x86 processors, but recommend AMD processor :wink:
* Operating System:
  - Ubuntu >= 22.04
  - Arch Linux
* Linux Kernel: v6.10 or above. (See [Linux compilation and installation](#linux-compilation-and-installation))
  - Due to Linux API change, XDNA driver doesn't always keep supporting old version.
* Installed XRT base package (or you can install it along the
  following recipe)
  - To make sure the XRT base package works with the plug-in package, better build it from `xrt` submodule in this repo (`<root-of-source-tree>/xrt`)
  - Refer to https://github.com/Xilinx/XRT for more detailed information.
