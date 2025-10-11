# mrg_helm Control System

This package includes a driver and firmware for compatible Marine Robotics Group devices.

## Installation

For standard installation, clone this repository:
```
uv tool install ./mrg-helm
```

For developers:

1. Install VSCode and the PlatformIO extension.
2. Clone this repository and open it as a PlatformIO project.
3. Install `uv`.
4. Install the mrg_helm Python program with `uv tool install -e ./mrg-helm --reinstall`.

## Introduction

This project is divided into 3 main components
- mrg_helm - Python package containing drivers
- firmware - source files for the PlatformIO project
- proto - protobuf message definitions which are automatically compiled and populate folders in mrg_helm and firmware upon install

## Quick Start

After installation, run:
```
mrg-helm --help
```
