Energy Usage Monitor
====================

This application is a real world example of a web application built on [libzeep](https://github.com/mhekkel/libzeep). It can be used to enter the current value of multiple usage meters and display the usage of energy over time.

Additionally, this software can monitor the status of one or more [Sessy batteries](https://www.sessy.nl/).

Installation
------------

To build and install energyd, you first need to install various other packages. Of course first of all you need [libzeep](https://github.com/mhekkel/libzeep). If you successfully installed libzeep, most of the requirements are already met, the remaining requirements are

* [mrc](https://github.com/mhekkel/mrc.git), a resource compiler
* Postgresql
* The development version of [libpqxx](https://pqxx.org/development/libpqxx/)
* The javascript package manager [yarn](https://yarnpkg.com/)

Building the software should start with running `yarn` without arguments in the source directory followed by the standard cmake commands:

```console
git clone https://github.com/mhekkel/energyd.git
cd energyd
cmake -S . -B build
cmake --build build
sudo cmake --install build
```

This should give you an executable called energyd installed in `/usr/local`.

The following commands are executed as a user that has admin rights on your postgres installation. First create a new user for this database,
this command will ask for a password.

```console
createuser -P energie-admin
```

Then create the database with the newly created user as owner:

```console
createdb -O energie-admin energie
```

Finally fill the database with tables and some data:

```console
psql -h localhost -f db-schemal.sql energie energie-admin
```

Configuring
-----------

Command line arguments can also be specified using a configuration file called `energyd.conf`. A
file with this name is searched at startup time in the current working directory and in `/etc`.

All command line arguments can be specified in this file in the usual ini file format.

A minimal config file might look like this:

```
databank=postgresql://energie-admin:geheim@db-server.local/energie
address=localhost
sessy-1=http://192.168.178.25/api/v1/power/status
p1-device arg=/dev/ttyUSB0
```

Running
-------

Now that the database is ready, we can start the application. There are several options you can give:

```console
$ ./energyd --help
energyd [options] command
  -h [ --help ]                    Display help message
  -v [ --verbose ]                 Verbose output
  --version                        Show version information
  --address arg (=0.0.0.0)         External address
  --port arg (=10336)              Port to listen to
  -F [ --no-daemon ]               Do not fork into background
  -u [ --user ] arg (=www-data)    User to run the daemon
  --databank arg                   The Postgresql connection string
  --p1-device arg (=/dev/ttyUSB0)  The name of the device used to communicate with the P1 port
  --sessy-1 arg                    URL to fetch the status of sessy number 1
  --sessy-2 arg                    URL to fetch the status of sessy number 2
  --sessy-3 arg                    URL to fetch the status of sessy number 3
  --sessy-4 arg                    URL to fetch the status of sessy number 4
  --sessy-5 arg                    URL to fetch the status of sessy number 5
  --sessy-6 arg                    URL to fetch the status of sessy number 6
  --read-only                      Do not write data into the database (debug option)


Command should be either:

    start     start a new server
    stop      stop a running server
    status    get the status of a running server
    reload    restart a running server with new options
```

The option `--databank` contains the connection string to connect to postgresql in the form of a URL.

The default is to run the application in the background. In that case a daemon process is forked off. This daemon process opens the log files in `/var/log/energyd` and writes a process ID in `/var/run/energyd`. Then it starts to listen on the specified address and port.

In case you want to run the server in the foreground for debugging purposes you can use the `--no-daemon` flag.

Usage
-----

The meters are hard coded, so this is perhaps not very useful for others. To complicate matters further, all text in the user interface is in Dutch. The idea is that you regularly enter (_Voeg toe_ button, or _Invoer_) the current values for the various meters. The graph (_Grafieken_) will then display your usage over time. The list of current values (_Opnames_) are displayed on the home page.
