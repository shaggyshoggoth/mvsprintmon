MVSPRINTMON
===========

Monitors a directory for creation of PDF files and
opens them in a PDF viewer.

Default behavior is to check `/tmp/mvsprintmon.pid` and
if a process is already running at that pid the new process exits.
If otherwise it will create/overwrite the pid file and background itself.

Requirements
------------
inotify support

Usage
-----
A typical launch would be:
```
$ mvsprintmon /usr/bin/zathura ${HOME}/dira ${HOME}/dirb
```
which will monitor `${HOME}/dira` and `${HOME}/dirb` and open
any newly created pdfs with zathura.

TODO
----
* Add option to change location of pid file.
* Perhaps add a dotfile and SIGHUP handler.

