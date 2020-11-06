## KiCad Project
https://github.com/AbiesDSP/headphone_dac_proto

# Installation Instructions

## Required Dependencies for Building the Project
[PSoC Creator](https://www.cypress.com/products/psoc-creator-integrated-design-environment-ide)

Just clone and build in PSoC Creator! You can do all development in PSoC Creator if you like.
If you wish to contribute to this project, please consider writing tests for your code using Ceedling.

## Required Dependencies for Contributing
[WSL](https://docs.microsoft.com/en-us/windows/wsl/install-win10)

[Ceedling](https://github.com/ThrowTheSwitch/Ceedling)

[Git LFS](https://git-lfs.github.com/)

## Git Config
This project uses a git filter to scrub the timestamp from ```.cyprj``` files.
Modify your ```~/.gitconfig``` to have these lines:
``` git
[filter "cyprjfilter"]
        smudge = cat
        clean = sed \"s/<current_generation v=\\\"[0-9]*\\\" \\/>/<current_generation v=\\\"0\\\" \\/>/\"
```
## Dependency Installation
Use the typical installation of PSoC Creator 4.3 or later for this project.
Everything else should be installed in a Linux environment. This project was developed using WSL (Ubuntu).
``` bash
gem install ceedling
sudo apt-get install git-lfs
```
## Project Setup
Perform these commands in a Linux environment on Windows. This has been tested on WSL (Ubuntu).
``` bash
cd PSoC\ Creator/
git clone https://github.com/half-spin/hp_dac_proto_fw.git hp_dac_proto
ceedling new --local --no_configs hp_dac_proto
```
```git lfs pull``` may be required if this is your first time using git lfs.
Run the tests.
``` bash
cd hp_dac_proto
ceedling test:all
```
