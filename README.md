# SMTP Server

An SMTP (Simple Mail Transfer Protocol) server receives, sends, and relays email messages between mail clients and servers.

## Class diagram

The class diagram is too large to be included as a screenshot, so you can view it via this [link](https://lucid.app/lucidchart/b109abd4-9765-43d5-8761-981ac61ed9fe/edit?useCachedRole=false&docId=b109abd4-9765-43d5-8761-981ac61ed9fe&shared=true&page=0_0#). This diagram covers all parts of our system, including the database, client, and server.

A simplified version of the class diagram is presented in the image below. It maintains all the logical connections of the full diagram but does not include the class fields.

![image](https://github.com/user-attachments/assets/6ec5e391-61c2-4075-83a9-018c93945c3a)

## Requirments

```
Gnu/Linux OS (Tested on Arch 2024.08.01; Ubuntu 24.04)
CMake 3.16+
C++20
OpenSSL
Boost
PQXX
```

## Setup Guide

### For Ubuntu and Debian

To install the required libraries on Ubuntu or Debian, you can use the `apt` package manager. Run the following commands:

```bash
sudo apt update
sudo apt install cmake g++ libssl-dev libboost-all-dev libpqxx-dev
```
* CMake 3.16+: cmake package (may need to add a PPA or manually download if the version is lower).
* C++20: Ensure you have the latest g++ package to support C++20 features.
* OpenSSL: libssl-dev
* Boost: libboost-all-dev
* pqxx: libpqxx-dev

### For Arch Linux and Manjaro
To install the required libraries on Arch Linux or Manjaro, you can use the `pacman` package manager or the `yay` AUR helper. Run the following commands:

* Using pacman:
```bash
sudo pacman -Syu cmake gcc openssl boost libpqxx
```

* Using yay (for AUR packages): 
```bash
yay -S cmake gcc openssl boost libpqxx
```

* CMake 3.16+: cmake package
* C++20: Ensure you have the latest gcc package to support C++20 features.
* OpenSSL: openssl package
* Boost: boost package
* pqxx: libpqxx package

In the `scripts` directory, there is a file named `start.sh.` This script builds your project and launches a Linux daemon for the server.

## Modules

In the `include` directory, you will find individual modules, which are C++ classes with separate header files. These modules can be linked as libraries or interfaces.

The available modules include:
* CommandHandler
* DataBase
* Logger
* MailMessage
* Parser
* Server
* SignalHandler
* SocketWrapper
* Utils

### CommandHandler

The CommandHandler module intercepts and processes all SMTP commands on the server, such as HELO, MAIL FROM, RCPT TO, DATA, QUIT, NOOP, RSET, HELP, STARTTLS, AUTH PLAIN, and REGISTER.

### DataBase

The DataBase module handles database operations. More details are available at [this link](https://github.com/UA-1240-C/DataBase).

### Logger

The Logger module manages message logging at various levels. More details are available at [this link](https://github.com/UA-1240-C/smtp-server/tree/SCRUM-160/Logger-Documentation-and-Integration).

### MailMessage

The MailMessage module simplifies the creation of email messages, allowing you to set the subject, body, sender, and recipient.

### Parser

The Parser module reads the configuration file located in the root directory of the project. It sets various necessary variables for the server, logger, and other components. More information can be found [here](https://github.com/UA-1240-C/smtp-server/tree/SCRUM-38-JSON-Parser-implementation).

### Server

The Server module is responsible for starting the server, connecting new clients, and reading and sending messages to clients.

### SignalHandler

The SignalHandler module is used to handle console commands like ^C, ensuring they are processed correctly and the application is shut down properly.

### SocketWrapper

The SocketWrapper module manages socket operations, implementing methods for initializing both TCP and SSL sockets, as well as timers for timeouts and methods for safely closing the socket.

### Utils

The Utils directory includes a `ThreadPool`, which is essential for the server's operation. When a new client connects, the thread pool creates a new task and delegates its execution to a separate thread.

