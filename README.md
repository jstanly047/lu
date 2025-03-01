# Lu - High Performance Event-Driven Platform Library

## Overview

Lu is a high-performance event-driven platform library designed for Linux-based operating systems. It provides a robust backend framework for developers to efficiently implement their business logic while meeting high-performance requirements.

Lu abstracts complex operations such as thread management, socket handling, and timer management into a decoupled layer, allowing developers to focus on their business logic without worrying about shared variables and concurrency challenges.

## Key Components

### 1. Threads

Lu provides multiple threading models to handle various use cases:

- **ServerThread**: Manages multiple threads to handle multiple clients.
- **SingleServerThread**: A single-threaded server for handling clients.
- **ConnectionThread**: Handles client-side connections.
- **WorkerThread**: A non-I/O thread to process work requests.

### 2. Sockets

Lu includes multiple socket modules for handling TCP and WebSocket communication:

- **ConnectSocket**: Handles client-side connections.
- **DataSocket**: Manages TCP data communication.
- **WebSocket**: Implements WebSocket protocol support.
- **HBDataSocket**: Supports both TCP and WebSocket.
- **HBSocketClient**: Client-side handler for TCP/WebSocket communication.
- **SSLSocket**: Wrapper for SSL/TLS support.

These modules can be combined with the threading models based on the client’s requirements to build efficient server and client applications.

## Usage

Refer to the unit test cases for usage examples and implementation details.

## Contact

For the latest version, more information and to contribute to the repo, contact **[jstanly047@gmail.com](mailto:jstanly047@gmail.com)**.
