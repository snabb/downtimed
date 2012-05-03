#!/bin/sh

### BEGIN INIT INFO
# Provides:          downtimed
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $local_fs
# Default-Start:     S
# Default-Stop:      0 6
# Short-Description: downtime record keeper
# Description:       Downtimed is a daemon which keeps records of
#		     the periods when the operating system has been
#                    out of service.
### END INIT INFO
#
#  Written 2011 by Mats Erik Andersson.
#  Licensed using two-clause FreeBSD statement.
#
###

DESC="downtime bookkeeper"
NAME="downtimed"

DAEMON="/usr/local/sbin/$NAME"
PIDFILE=/var/run/$NAME.pid

PATH=/sbin:/usr/sbin:/bin:/usr/bin
SCRIPT=/etc/init.d/$NAME

DOWNTIMED_OPTS=""

# Are we making sense?
[ -x "$DAEMON" ] || exit 0

# Read configuration variable file if it is present
[ -r /etc/default/$NAME ] && . /etc/default/$NAME

# Detect VERBOSE mode and load any rcS variables.
. /lib/init/vars.sh

# Define all LSB log_* functions.
. /lib/lsb/init-functions

# Start downtimed.
do_start()
{
	start-stop-daemon --start --quiet --pidfile $PIDFILE \
			--exec $DAEMON --test > /dev/null \
		|| return 1
	start-stop-daemon --start --quiet --pidfile $PIDFILE \
			--exec $DAEMON -- $DOWNTIMED_OPTS \
		|| return 2

	# Return status:
	#   0    daemon has been started
	#   1    daemon was already running
	#   2    daemon could not be started
}

# Halt downtimed.
do_stop()
{
	start-stop-daemon --stop --quiet --retry=TERM/30/KILL/5 \
			--pidfile $PIDFILE --exec $DAEMON
	RETVAL="$?"
	[ "$RETVAL" = 2 ] && return 2

	rm -f $PIDFILE
	return "$RETVAL"

	# Return status:
	#   0      daemon has been stopped
	#   1      daemon was already stopped
	#   2      daemon could not be stopped
	#   other  daemon reported some other failure
}

# Reloading is a no-do in the standard case, when logging is
# done via the syslog service. Only if $DOWNTIMED_OPTS states
# a separate logging file, then a SIGHUP will result in the
# daemon releasing the file for rotation, or a similar action.

do_reload() {
	start-stop-daemon --stop --signal 1 --quiet \
			--pidfile $PIDFILE --exec $DAEMON
	return 0
}

case "$1" in
  start)
	[ "$VERBOSE" != no ] && log_daemon_msg "Starting $DESC" "$NAME"
	do_start
	case "$?" in
		0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
		2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
	esac
	;;
  stop)
	[ "$VERBOSE" != no ] && log_daemon_msg "Stopping $DESC" "$NAME"
	do_stop
	case "$?" in
		0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
		2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
	esac
	;;
  status)
	status_of_proc "$DAEMON" "$NAME" && exit 0 || exit $?
	;;
  reload|force-reload)
	log_daemon_msg "Reloading $DESC" "$NAME"
	do_reload
	log_end_msg $?
	;;
  restart)
	log_daemon_msg "Restarting $DESC" "$NAME"
	do_stop
	case "$?" in
	  0|1)
		do_start
		case "$?" in
			0) log_end_msg 0 ;;
			1) log_end_msg 1 ;; # Still running.
			*) log_end_msg 1 ;; # Failure in starting.
		esac
		;;
	  *)
	  	# Failure in ending service.
		log_end_msg 1
		;;
	esac
	;;
  *)
	echo "Usage: $SCRIPTNAME {start|stop|restart|reload|force-reload|status}" >&2
	exit 3
	;;
esac

:
