# FileSync

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23) [![CMake](https://img.shields.io/badge/CMake-3.20%2B-064F8C.svg)](https://cmake.org/) [![Build](https://github.com/null-valence/FileSync/actions/workflows/build.yml/badge.svg)](https://github.com/null-valence/FileSync/actions/workflows/build.yml)

A C++ file synchronization tool designed to synchronize directories, detect changes using file metadata and SHA-256 hashes, and maintain persistent synchronization state.

> **Status:** Active development

## Overview

FileSync is a command-line file synchronization tool written in C++.

The project is being developed incrementally, starting with local filesystem synchronization and gradually moving toward a networked synchronization system.

The current implementation supports local synchronization between two directories, persistent synchronization state, change detection, and conflict detection.

## Features

### Current

- Recursive directory scanning
- File metadata tracking
  - File size
  - Last modification time
- SHA-256 content hashing
- Synchronization planning
- File operations
  - Copy
  - Update
  - Delete
- Persistent synchronization state
- Change detection using previous synchronization state
- Conflict detection
- Structured error handling using `std::expected`
- Automated tests using GoogleTest
- Linux-first development

### Planned

- Robust two-way synchronization
- TCP-based synchronization
- Incremental/block-based file transfer
- Resumable transfers
- Concurrent file transfers
- Transfer progress reporting
- Performance optimizations
- Improved conflict resolution

## Architecture

The project is divided into several components:

```text
                  ┌─────────────┐
                  │   Scanner   │
                  └──────┬──────┘
                         │
                         ▼
                    ┌─────────┐
                    │ Planner │
                    └────┬────┘
                         │
                    SyncActions
                         │
                         ▼
                   ┌──────────┐
                   │ Executor │
                   └─────┬────┘
                         │
                         ▼
                    Filesystem
```
                    
### Supporting components:
```text
Hasher
  │
  └── SHA-256 file content hashing

State
  │
  ├── Load previous synchronization state
  ├── Build current state
  └── Persist synchronization state
```

### Synchronization Pipeline
```text
Scan directories
       │
       ▼
Load previous state
       │
       ▼
Create synchronization plan
       │
       ▼
Check for conflicts
       │
       ▼
Execute synchronization actions
       │
       ▼
Rescan filesystem
       │
       ▼
Build new state
       │
       ▼
Save state
```

## Project Structure
```text
FileSync/
├── src/
│   ├── main.cpp
│   ├── scanner.h
│   ├── scanner.cpp
│   ├── sync.h
│   ├── sync.cpp
│   ├── hasher.h
│   ├── hasher.cpp
│   ├── state.h
│   └── state.cpp
│
├── test/
│   └── sync_test.cpp
│
├── CMakeLists.txt
├── .gitignore
├── LICENSE
└── README.md
```

## Requirements

- C++23 compiler
- CMake 3.20 or newer
- OpenSSL development libraries
- GoogleTest

### Linux dependencies

On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install cmake g++ libssl-dev
```

On Fedora:
```bash
sudo dnf install cmake gcc-c++ openssl-devel
```

The project is currently developed and tested primarily on Linux.

## Building

Clone the repository:

```bash
git clone https://github.com/null-valence/FileSync.git
cd FileSync
```

Create a build directory:
```bash
cmake -S . -B build
```

Build:
```bash
cmake --build build
```

The executable will be available at: `build/filesync`

## Usage
FileSync currently accepts two directories: `./build/filesync <directory_A> <directory_B>`

For example: `./build/filesync ~/folderA ~/folderB`

The synchronization state is stored in: `<directory_A>/.filesync/state`

The `.filesync` directory is internal FileSync metadata and is excluded from synchronization.
### Synchronization Behavior
FileSync maintains information about previously synchronized files.
For example:

```text
Directory A          Directory B

file.txt    ───────► file.txt
```

If a file changes in A while B remains unchanged, FileSync plans an update from A to B. If a file is deleted from A while B remains unchanged, FileSync plans a deletion from B.

When both sides have changed a file, FileSync can detect the situation as a conflict rather than silently overwriting one side.

### State
FileSync stores persistent state containing information about synchronized files:
```text
File
├── relative path
├── size
├── last modification time
└── SHA-256 hash
```

This allows subsequent executions to distinguish between unchanged files and files that have changed since the previous synchronization.

### Testing
Build the project: `cmake --build build`

Run the test suite: `ctest --test-dir build --output-on-failure`

The current test suite covers:
- Synchronization planning
- Change detection
- Conflict detection
- State serialization
- State deserialization
- State corruption handling
- File hashing
- Copy operations
- Delete operations
- Update operations
- Executor error handling
- State construction
## Roadmap
The project is being developed in stages:
- [x] Filesystem scanner
- [x] File metadata tracking
- [x] SHA-256 content hashing
- [x] Synchronization planner
- [x] Copy / Update / Delete executor
- [x] Persistent synchronization state
- [x] Conflict detection
- [x] Automated test suite
- [x] Robust two-way synchronization
- [ ] TCP networking
- [ ] Incremental/block-based transfer
- [ ] Resumable transfers
- [ ] Concurrent transfers
- [ ] Transfer progress reporting
- [ ] Performance optimization

## Design Goals
The project focuses on:
- Correctness
- Clear separation of responsibilities
- Explicit error handling
- Testability
- Efficient change detection
- Incremental development
- Understanding the underlying systems concepts rather than relying on high-level synchronization libraries

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
