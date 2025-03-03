Energy Usage Monitor
====================

This application is a real world example of a web application built on [libzeep](https://github.com/mhekkel/libzeep). It can be used to enter the current value of multiple usage meters and display the usage of energy over time.

Additionally, this software can monitor the status of one or more [Sessy batteries](https://www.sessy.nl/) and it will
read data from a P1 port of your smart meter, if configured. When you have a Sessy, you should use a splitter and
a P1 to USB converter like [the one from Cedel](https://webshop.cedel.nl/nl/Slimme-meter-kabel-P1-naar-USB).

Installation
------------

To build and install energyd, you first need to install various other packages. Of course first of all you need [libzeep](https://github.com/mhekkel/libzeep). If you successfully installed libzeep, most of the requirements are already met, the remaining requirements are:

* Postgresql
* The development version of [libpqxx](https://pqxx.org/development/libpqxx/)
* The javascript package manager [yarn](https://yarnpkg.com/)
* [mrc](https://github.com/mhekkel/mrc.git), a resource compiler
* [libmcfp](https://github.com/mhekkel/libmcfp), a library for parsing command line arguments

Building the software is done by entering the following commands:

```console
git clone https://github.com/mhekkel/energyd.git
cd energyd
cmake -S . -B build
cmake --build build
sudo cmake --install build
```

This should give you an executable called energyd installed in `/usr/local`.

Database
--------

The software uses a postgresql database to store data. The following commands are executed as a user
that has admin rights on your postgres installation. First create a new user for this database,
this command will ask for a password which you will have to add to the connect URL for the database.

```console
createuser -P energie-admin
```

Then create the database with the newly created user as owner:

```console
createdb -O energie-admin energie
```

Finally fill the database with tables:

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
databank=postgresql://energie-admin:geheim@localhost/energie
address=localhost
sessy-1=http://192.168.178.25/api/v1/power/status
p1-device=/dev/ttyUSB0
```

Running
-------

Now that the database is ready, we can start the application. There are several options you can give:

```console
$ energyd --help
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

This software actually combines two different things. Originally it was used to manually enter the readings of a smart meter and stadsverwarmingmeter.
I've done this for many years and this software can render nice graphs for electricity and warmth usage showing the current year in the context of history.

The meters are hard coded, so this is perhaps not very useful for others. To complicate matters further, all text in the user interface is in Dutch. The idea is that you regularly enter (_Voeg toe_ button, or _Invoer_) the current values for the various meters. The graph (_Grafieken_) will then display your usage over time.

The second part was bolted on later when a Sessy battery entered the home. To monitor the loading and unloading of the battery a new graph
was added using a new data table. This data is stored every two minutes automatically. This new page is now the home page of the application.
