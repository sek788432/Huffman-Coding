# Huffman-Coding

A client-server implementation of file compression using Huffman coding algorithm.

## Overview

This project implements a file compression system where:

- Client compresses files using Huffman coding and sends them to the server
- Server receives compressed files and decompresses them using the provided Huffman codes
- Both components use shared utilities for logging and configuration

## Project Structure

```bash
.
├── utils.h              # Shared utilities (Logger, constants)
├── utils.cpp            # Utilities implementation
├── client_place/        # Client component
│   ├── client.cpp      # Client implementation
│   └── Makefile        # Client build rules
└── server_place/       # Server component
    ├── server.cpp      # Server implementation
    └── Makefile        # Server build rules
```

## Building and Running

### Prerequisites

- C++17 compiler
- POSIX-compliant system (Linux, macOS)
- Make build system

### Server

```sh
cd server_place
make
./server
```

The server will:

1. Listen for incoming connections
2. Accept compressed files and Huffman codes
3. Decompress files using the provided codes
4. Log operations to server.log

### Client

```sh
cd client_place
make
./client
```

The client will:

1. Read the input file (test.jpg by default)
2. Generate Huffman codes and compress the file
3. Send compressed data and codes to the server
4. Log operations to client.log

### Cleanup

To remove executables and generated files:

```sh
make clean  # Run in either client_place/ or server_place/
```

## Implementation Features

### Shared Components

- Custom Logger class with file and console output
- Network configuration constants
- File path definitions

### Client Features

- Huffman tree construction
- File compression
- Network communication

### Server Features

- Multi-process client handling
- File decompression
- Concurrent processing

## Generated Files

- `*.log`: Operation logs
- `compressed.jpeg`: Compressed image
- `code.txt`: Huffman codes
- `decoded.jpeg`: Decompressed image
- `for_decompressed.txt`: Additional decompression data
