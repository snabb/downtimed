#! /bin/sh
#
# Copyright (c) 1995-2001 SuSE GmbH Nuernberg, Germany.
# Copyright (c) 2002 SuSE Linux AG, Nuernberg, Germany.
# Copyright (c) 2002--2008  Klaus Singvogel, SUSE / Novell Inc.
# Copyright (c) 2010 Federico Lucifredi, SUSE / Novell Inc.
#
# Author: Kurt Garloff <feedback@suse.de>, 2000
#	  Klaus Singvogel <feedback@suse.de>, 2002--2008
#         Federico Lucifredi <feedback@suse.de>, 2010
#
# /etc/init.d/downtimed
#
# System startup script for the downtimed daemon
#
### BEGIN INIT INFO
# Provides:            downtimed
# Required-Start:      
# Required-Stop:       
# Should-Start:        
# Should-Stop:         
# Default-Start:       2 3 5
# Default-Stop:        0 1 6
# Short-Description:   downtime monitoring daemon
# Description:         Start downtimed to provide monitoring of uptime and
#	downtime intervals.
#
### END INIT INFO

# Source SuSE config, only if exists with size greater zero
test -s /etc/rc.config && \
    . /etc/rc.config

# Shell functions sourced from /etc/rc.status:
#      rc_check         check and set local and overall rc status
#      rc_status        check and set local and overall rc status
#      rc_status -v     ditto but be verbose in local rc status
#      rc_status -v -r  ditto and clear the local rc status
#      rc_failed        set local and overall rc status to failed
#      rc_failed <num>  set local and overall rc status to <num><num>
#      rc_reset         clear local rc status (overall remains)
#      rc_exit          exit appropriate to overall rc status

DOWNTIMED_BIN=/usr/local/sbin/downtimed

test -s /etc/rc.status && \
     . /etc/rc.status

test -x $DOWNTIMED_BIN || exit 5

# First reset status of this service
rc_reset

# Return values acc. to LSB for all commands but status:
# 0 - success
# 1 - generic or unspecified error
# 2 - invalid or excess argument(s)
# 3 - unimplemented feature (e.g. "reload")
# 4 - insufficient privilege
# 5 - program is not installed
# 6 - program is not configured
# 7 - program is not running
# 
# Note that starting an already running service, stopping
# or restarting a not-running service as well as the restart
# with force-reload (in case signalling is not supported) are
# considered a success.

case "$1" in
    start)
	echo -n "Starting downtimed"
	## Start daemon with startproc(8). If this fails
	## the echo return value is set appropriate.

	# NOTE: startproc return 0, even if service is 
	# already running to match LSB spec.
	startproc $DOWNTIMED_BIN $DOWNTIMED_OPTIONS

	# Remember status and be verbose
	rc_status -v
	;;
    stop)
	echo -n "Shutting down downtimed"
	## Stop daemon with killproc(8) and if this fails
	## set echo the echo return value.

	killproc -TERM $DOWNTIMED_BIN

	# Remember status and be verbose
	rc_status -v
	;;
    try-restart)
	## Stop the service and if this succeeds (i.e. the 
	## service was running before), start it again.
	## Note: try-restart is not (yet) part of LSB (as of 0.7.5)
	$0 status >/dev/null &&  $0 restart

	# Remember status and be quiet
	rc_status
	;;
    restart)
	## Stop the service and regardless of whether it was
	## running or not, start it again.
	$0 stop
	$0 start

	# Remember status and be quiet
	rc_status
	;;
    force-reload)
	## Signal the daemon to reload its config. Most daemons
	## do this on signal 1 (SIGHUP).
	## If it does not support it, restart.

	if ps -C downtimed -o user | grep -q '^root$'; then
	    echo -n "Reload service downtimed"
	    killproc -HUP $DOWNTIMED_BIN
	    rc_status -v
	else
	    $0 restart
	fi
	;;
    reload)
	## Like force-reload, but if daemon does not support
	## signalling, do nothing (!)

	# If it supports signalling:
	if ps -C downtimed -o user | grep -q '^root$'; then
	    echo -n "Reload service downtimed"
	    killproc -HUP $DOWNTIMED_BIN
	    rc_status -v
	else
	    echo -n '"reload" not possible in RunAsUser mode - use "restart" instead'
	    rc_status -s
	fi
	;;
    status)
	echo -n "Checking for downtimed: "
	## Check status with checkproc(8), if process is running
	## checkproc will return with exit status 0.

	# Status has a slightly different for the status command:
	# 0 - service running
	# 1 - service dead, but /var/run/  pid  file exists
	# 2 - service dead, but /var/lock/ lock file exists
	# 3 - service not running

	# NOTE: checkproc returns LSB compliant status values.
	checkproc $DOWNTIMED_BIN
	rc_status -v
	;;
    probe)
	## Optional: Probe for the necessity of a reload,
	## give out the argument which is required for a reload.

	rc_failed 3
	;;
    *)
	echo "Usage: $0 {start|stop|status|try-restart|restart|force-reload|reload|probe}"
	exit 1
	;;
esac
rc_exit
