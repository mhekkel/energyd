Energy Usage Monitor
====================

This application is a real world example of a web application built on [libzeep](https://github.com/mhekkel/libzeep). It can be used to enter the current value of multiple usage meters and display the usage of energy over time.

Installation
------------

To build and install energyd, you first need to install various other packages. Of course first of all you need [libzeep](https://github.com/mhekkel/libzeep). If you successfully installed libzeep, most of the requirements are already met, the remaining requirements are

 * Postgresql
 * The development version of [libpqxx](https://pqxx.org/development/libpqxx/)
 * The javascript package manager [yarn](https://yarnpkg.com/)

Building the software should start with running `yarn` without arguments in the source directory followed by `configure` and `make`.

To summarize:

```
git clone https://github.com/mhekkel/energyd.git
cd energyd
yarn
./configure
make
```

This should give you an executable called energyd. The next thing to do is create a postgres database. There's a dump of an old version of my database included. The steps to create your copy are:

The following commands are executed as a user that has admin rights on your postgres installation. First create a new user for this database,
this command will ask for a password.

```
createuser -P energie-admin
```
Then create the database with the newly created user as owner:

```
createdb -O energie-admin energie
```
Finally fill the database with tables and some data:

```
psql -h localhost -f energie.sql energie energie-admin
```

Running
-------

Now that the database is ready, we can start the application. There are several options you can give:

```
$ ./energyd --help
./energyd [options] command:
  -h [ --help ]         Display help message
  -v [ --verbose ]      Verbose output
  --address arg         External address, default is 0.0.0.0
  --port arg            Port to listen to, default is 10336
  -F [ --no-daemon ]    Do not fork into background
  -u [ --user ] arg     User to run the daemon
  --db-host arg         Database host
  --db-port arg         Database port
  --db-dbname arg       Database name
  --db-user arg         Database user name
  --db-password arg     Database password


Command should be either:

start     start a new server
stop      start a running server
status    get the status of a running server
reload    restart a running server with new options
```

The options starting with `-db-` are the ones that describe the access to the postgresql database you just created. The `--address` and `--port` options are for the web interface.

Then there're the `--no-daemon` option and the command. The default is to run the application in the background. In that case a daemon process is forked off. This daemon process opens the log files in `/var/log/energyd` and writes a process ID in `/var/run/energyd`. Then it starts to listen on the specified address and port.

When the first request comes in, a new process is forked and the priviliges of this process are dropped and the new user becomes the one specified in `--user` or, if not provided, `www-data`. This request and all following requests are then passed on to this child process for processing.

For debugging it might be useful to run the application in the foreground without forking. For that there is the option `--no-daemon`.

Usage
-----

The meters are hard coded, so this is perhaps not very useful for others. To complicate matters further, all text in the user interface is in Dutch. The idea is that you regularly enter (_Voeg toe_ button, or _Invoer_) the current values for the various meters. The graph (_Grafieken_) will then display your usage over time. The list of current values (_Opnames_) are displayed on the home page.