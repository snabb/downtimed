#!/bin/sh

# PROVIDE: downtimed
# REQUIRE: LOGIN syslogd
# KEYWORD: shutdown

# This file should be installed as /usr/local/etc/rc.d/downtimed.
#
# Add the following lines to /etc/rc.conf to enable `downtimed':
#
# downtimed_enable="YES"
# downtimed_flags="<set as needed>"
#
# See downtimed(8) for possible downtimed_flags
#

. "/etc/rc.subr"

name="downtimed"
rcvar=`set_rcvar`

command="/usr/local/sbin/$name"
pidfile="/var/run/$name.pid"

# read configuration and set defaults
load_rc_config "$name"
: ${downtimed_enable="NO"}
: ${downtimed_flags=""}

run_rc_command "$1"
