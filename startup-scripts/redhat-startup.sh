#!/bin/bash
#
#	/etc/rc.d/init.d/downtimed
#
#	downtimed: system downtime monitoring and reporting tool
#
#	This init script should work on Red Hat related distributions
#	such as RHEL/Fedora/CentOS. Based on:
#	http://fedoraproject.org/wiki/Packaging:SysVInitScript
#
# chkconfig:	2345 99 99
# description:	downtimed is a program that monitors operating system \
#		downtime, uptime, shutdowns and crashes and keeps records \
#		of those events.
# processname:	downtimed
# pidfile:	/var/run/downtimed.pid

### BEGIN INIT INFO
# Provides: downtimed
# Required-Start: $remote_fs $time
# Required-Stop: $local_fs
# Should-Start: $syslog $all
# Should-Stop: $syslog
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: system downtime monitoring and reporting tool
# Description: downtimed is a program that monitors operating system
#		downtime, uptime, shutdowns and crashes and keeps records
#		of those events.
### END INIT INFO

# Source function library.
. /etc/rc.d/init.d/functions

prog="downtimed"
exec="/usr/local/sbin/$prog"

[ -e /etc/sysconfig/$prog ] && . /etc/sysconfig/$prog

lockfile=/var/lock/subsys/$prog

start() {
	[ -x $exec ] || exit 5
	echo -n $"Starting $prog: "
	daemon $exec
	retval=$?
	echo
	[ $retval -eq 0 ] && touch $lockfile
	return $retval
}

stop() {
	echo -n $"Stopping $prog: "
	killproc $prog
	retval=$?
	echo
	[ $retval -eq 0 ] && rm -f $lockfile
	return $retval
}

restart() {
	stop
	start
}

reload() {
	restart
}

force_reload() {
	restart
}

rh_status() {
	status $prog
}

rh_status_q() {
	rh_status >/dev/null 2>&1
}


case "$1" in
start)
	rh_status_q && exit 0
	$1
	;;
stop)
	rh_status_q || exit 0
	$1
	;;
restart)
	$1
	;;
reload)
	rh_status_q || exit 7
	$1
	;;
force-reload)
	force_reload
	;;
status)
	rh_status
	;;
condrestart|try-restart)
	rh_status_q || exit 0
	restart
	;;
*)
	echo $"Usage: $0 {start|stop|status|restart|condrestart|try-restart|reload|force-reload}"
	exit 2
esac
exit $?

# eof
