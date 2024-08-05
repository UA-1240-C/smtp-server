# smtp-server

# Server Configuration

This repository provides configuration files for server settings in JSON format. These configuration files define various server parameters including server details, communication settings, logging options, timing intervals, and thread pool settings.

## Configuration File Structure

The configuration file is written in JSON format. Below is an example of a JSON configuration file with explanations for each property.

### Example JSON File

```json
{
  "root": {
    "Server": {
      "servername": "ServTest",
      "serverdisplayname": "ServTestserver",
      "listenerport": 25000,
      "ipaddress": "127.0.0.1"
    },
    "communicationsettings": {
      "blocking": 0,
      "socket_timeout": 5
    },
    "logging": {
      "filename": "serverlog.txt",
      "LogLevel": 2,
      "flush": 0
    },
    "time": {
      "Period_time": 30
    },
    "threadpool": {
      "maxworkingthreads": 10
    }
  }
}
```

## Configuration Details

### Server:

servername: Name that the server will use upon installation.
serverdisplayname: Display name for the server.
listenerport: Port on which the server listens for incoming connections.
ipaddress: IP address used by the server to receive client requests.
### communicationsettings:

blocking: Determines if the socket is blocking (1 to enable, 0 to disable).
socket_timeout: Timeout in seconds for the accept function (only relevant if blocking is 0).
### logging:

filename: Path to the log file.
LogLevel: Level of logging detail (0 for no logs, 1 for production logs, 2 for debug logs, 3 for trace logs).
flush: Whether to flush the log output (1 to enable, 0 to disable). Note that enabling flush can impact performance.
### time:

Period_time: Interval in seconds for calling threads.
### threadpool:

maxworkingthreads: Maximum number of working threads in the thread pool.
