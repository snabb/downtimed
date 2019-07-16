downtimed - system downtime monitoring and reporting tool
=========================================================

Copyright (c) 2009-2016 Janne Snabb. All rights reserved.

This software is licensed under the terms and conditions of the Simplified
BSD License. You should have received a copy of that license along with
this software.

Software web site:

https://dist.epipe.com/downtimed/

Development version is available at GitHub:

https://github.com/snabb/downtimed

Use GitHub for reporting bugs, sending contributions etc.


## Portability

Compatible operating systems / distributions:
 - GNU/Linux
 - FreeBSD, NetBSD and OpenBSD
 - Mac OS X/Darwin
 - GNU/Hurd
 - Solaris / OpenSolaris / OpenIndiana

The only really non-portable part of the program is the getboottime()
function which uses an operating system specific interface for finding
the exact moment when the kernel started again. The function can be
easily altered to add support for additional operating systems. The
author would appreciate receiving any such patches to be integrated in
the main distribution.

Also for the sake of convenience the program uses some functions which
are available only in modern operating systems. These include for example
asprintf(3), vasprintf(3), snprintf(3), err(3) and errx(3).


## History

The author of this software had a Xen based virtual private server
(VPS) running FreeBSD operating system. Occasionally the owner of the
physical non-virtual machine would turn off the VPS to do some system
administration tasks on the host operating system, which the author did
not have access to. The author wanted to have some sort of record of when,
why and how long the VPS had been down. The standard FreeBSD system did
not provide such a facility.

After some looking around it seemed that there was only programs for
monitoring uptime but not downtime. Also most of these programs were
complicated and were intended to be run on a remote host and thus they
would be actually monitoring the system availability through the network
instead of the actual downtime of the FreeBSD operating system running
on the VPS. Also they would require an another server for monitoring
the primary server.

Therefore downtimed was needed.


## Features

downtimed was made to monitor operating system downtime, shutdowns and
crashes on the monitored host itself and to keep a record of such events.

downtimed(8) is a daemon process which is intended to be started
automatically from system boot scripts every time when the operating
system of a server starts. First the daemon logs its findings about the
previous downtime to a specified logging destination as well as in a
database file which can be displayed with downtimes(1) command.

Thereafter the downtimed(8) daemon just keeps waiting in the background
and periodically updates a time stamp file on the disk. The time stamp
is used to determine the approximate time when the system was last up
and running. In case of a graceful system shutdown it records a stamp
to another file on the disk. These files are used for reporting the next
time the daemon starts.

downtimes(1) is a command-line tool which can be used to inspect previous
downtime records recorded in the downtime database file.


## Installation

Installation should be preferably done through a port or a package
which is tailored to your specific operating system.

If one does not exist or if you yourself are making such a port or a
package, the basic GNU autotools based installation should be as follows:

If using a development version checked out from the source repository, begin
by bootstrapping GNU autotools in the usual way. This is not needed if using
a release tarball:
```
autoreconf
```

Proceed with the traditional configure + make build process:
```
./configure
make
make install
```

The above does NOT install any startup scripts which are REQUIRED for
proper function of downtimed. See the following chapter.

The program also needs a persistent data directory for the time stamp
and the downtime database files. It should be created on *BSD and
MacOS X as follows:

`mkdir /var/db/downtimed`

...or on GNU/Linux, Debian GNU/kFreeBSD, GNU/Hurd or Solaris as follows:

`mkdir /var/lib/downtimed`

Note that you can determine the default data directory location on
your system by issuing `downtimed -v` command or you can specify a
different directory with the `-d` option. Set the directory permissions
as appropriate in your environment. If you are installing a port or
package tailored for your system, this step is most likely taken care
for you automatically.


## Startup scripts

It seems that every different operating system and distribution has
invented their own ways of starting system daemons during the boot
process. That is a major pain in the ass.

The downtimed distribution includes the following operating system and
distribution specific startup script samples. They are located in the
"startup-scripts" directory. It is assumed that system administrators
or port/package maintainers will implement and configure the required
startup scripts. They are not installed by default.

This program is not really useful unless there is a proper startup script
in place. Refer to your operating system or distribution manual on how
to create and manage daemon startup scripts.

### systemd: Debian, Ubuntu, RHEL / CentOS

Debian 8, Ubuntu 15.04, RHEL / CentOS 7 and later versions are using systemd
for managing system services.

systemd unit file is included as downtimed.service. It should be installed
as /etc/systemd/system/downtimed.service. Issue the following commands as
root to enable and start the service:
```
systemctl enable downtimed
systemctl start downtimed

```

### Arch Linux

Arch Linux startup script is included as archlinux-startup.sh. It should
be installed as /etc/rc.d/downtimed and added to the DAEMONS setting in
/etc/rc.conf.

### FreeBSD

A sample startup script for FreeBSD is included as freebsd-startup.sh.
It should be installed as /usr/local/etc/rc.d/downtimed.

Add `downtimed_enable="YES"` in one of the following files to enable boot time
startup: /etc/rc.conf /etc/rc.conf.local /etc/rc.conf.d/downtimed

Add the following if you want to configure downtimed(8) command line
options: `downtimed_flags="<set as needed>"`

### Mac OS X

Mac OS X/Darwin launchd(8) configuration file com.epipe.downtimed.plist is
also included. This file should be installed to /Library/LaunchDaemons.

### OpenIndiana / OpenSolaris / Solaris

SMF (Service Management Facility) manifest for OpenIndiana, OpenSolaris
and Solaris 10 & 11 is in downtimed.smf.xml. It is not usable on Solaris
9 and older as they need a SysV style init script instead.

### openSUSE

A startup script for openSUSE is included as opensuse-startup.sh. It
should be installed as /etc/init.d/downtimed. Running the command
`innserv /etc/init.d/downtimed` enables starting the service at the
system startup.

### old Debian

A startup script for GNU/Debian and related distributions with SysV style
init scripts is included as debian-startup.sh. It should be installed as
/etc/init.d/downtimed. Running the command `update-rc.d downtimed defaults`
enables starting the service at the system startup.

Users of Debian 8 and later should look at the systemd instructions above.

### old RHEL based distributions (CentOS, Scientific Linux, Oracle Linux)

A startup script for RHEL 5 and 6 and related distributions is included as
redhat-startup.sh. It should be installed as /etc/rc.d/init.d/downtimed.
Running the command `chkconfig --add downtimed` enables starting the
service at the system startup.

Users of RHEL/CentOS 7 and later should look at the systemd instructions above.

### old Ubuntu

A startup script for GNU/Linux distributions using upstart(8) to bring
up system daemons, such as the Ubuntu distribution until version 14.10, is
included in upstart-startup.conf. It should be installed as
/etc/init/downtimed.conf.

Users of Ubuntu 15.04 and later should look at the systemd instructions above.


## Usage documentation

Have a look at downtimed(8) manual page:

`man downtimed`

... as well as the downtimes(1) manual page:

`man downtimes`

Alternatively you can find a PDF version of the manual pages at:

https://dist.epipe.com/downtimed/


## Contributions

If you port this software to a new operating system, find bugs or
implement new features, it would be nice if you could send your patches
to the author either through Github or by e-mail. See the top of this
document for contact information.


## Acknowledgements

The following people have contributed patches or other improvements to
this software:

Henrik Ahlgren <pablo at seestieto.com>
 - Mac OS X patches

Mats Erik Andersson <openbsd at gisladisker.se>
 - OpenBSD, Debian GNU/kFreeBSD and other patches

Federico Lucifredi <flucifredi at acm.org>
 - openSUSE startup script

Jason Melton <jason.melton at gmail.com>
 - Arch Linux startup script

Douglas Thrift <douglas at douglasthrift.net>
 - FreeBSD 9 compatibility fix

Others who I may have forgotten. (Sorry!)

Thank You!

