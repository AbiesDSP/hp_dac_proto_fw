# Installation Instructions

## Dependencies
[PSoC Creator](https://www.cypress.com/products/psoc-creator-integrated-design-environment-ide)

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
Install dependencies if you don't have them. Use the typical installation of PSoC Creator 4.3 or later for this project.
``` bash
gem install ceedling
git lfs install
```
## Project Setup
Perform these commands in a Linux environment on Windows. This has been tested on WSL (Ubuntu).
``` bash
cd PSoC\ Creator/
git clone repo@url hp_dac_proto
ceedling new --local --no_configs hp_dac_proto
```
```git lfs pull``` may be required if this is your first time using git lfs.
``` bash
cd hp_dac_proto
ceedling test:all
```
