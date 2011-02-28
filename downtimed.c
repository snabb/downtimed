/*-
 * Copyright (c) 2009-2011 EPIPE Communications. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of EPIPE Communications.
 *
 *
 * Software web site:
 *   http://dist.epipe.com/downtimed/
 *
 * Author contact information:
 *   opensource@epipe.com
 */

/* Include config.h in case we use autoconf. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * _GNU_SOURCE is required to enable in asprintf() and vasprintf()
 * in <stdio.h> on GNU/Linux.
 *
 * _BSD_SOURCE is required to enable BSD style signal(3) facility
 * on GNU/Linux.
 */

#if defined(__linux__) || defined(__GLIBC__)
/* GNU/Linux or GNU/kFreeBSD */
#define	_GNU_SOURCE
#define	_BSD_SOURCE
#endif

/*
 * Pull in facilitynames array if <syslog.h> has it.
 */

#ifdef HAVE_SYSLOG_FACILITYNAMES
#define	SYSLOG_NAMES
#endif

/* Standard includes that we need */

#include <sys/file.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#include <sys/time.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>
#if defined(__SVR4) && defined(HAVE_UTMPX_H)
#include <utmpx.h>
#endif
#include <syslog.h>

#include "downtimedb.h"

/* Some global defines */

#define	PROGNAME "downtimed"

/* PACKAGE_VERSION is defined by autoconf, if used. */
#ifdef PACKAGE_VERSION
#define	PROGVERSION PACKAGE_VERSION
#else
#define	PROGVERSION "0.0undef"
#endif

/* We expect the following preprocessor macros to be defined */

/* from <paths.h> */

#ifndef _PATH_VARRUN
#define	_PATH_VARRUN	"/var/run/"
#endif

/* from <sys/stat.h> */

#ifndef DEFFILEMODE
#define	DEFFILEMODE 0666
#endif

/* Function prototypes */

int		main(int, char *[]);
static time_t	getboottime(void);
static void	updatedowntimedb(time_t, int, time_t);
static void	report(void);
static void	sighandler(int);
static void	touch(const char *, time_t);
static void	loginit(void);
static void	logdeinit(void);
static void	logwr(int, const char *, ...);
static void	version(void);
static void	usage(void);
static void	parseargs(int, char *[]);
static int	makepidfile(void);
static void	removepidfile(void);

/* Command line arguments with their defaults */

char *	cf_log = "daemon";  /* syslog facility or filename with a slash (/) */
char *	cf_pidfile = _PATH_VARRUN PROGNAME ".pid";
char *	cf_datadir = PATH_DOWNTIMEDBDIR;
long	cf_sleep = 15;                 /* update time stamp every 15 seconds */
int	cf_fsync = 1;	      /* if true, fsync() stamp files after touching */
int	cf_downtimedb = 1;		       /* if true, update downtimedb */
char *	cf_downtimedbfile = PATH_DOWNTIMEDBFILE;

/* Logging destination, determined from cf_log */

int	cf_logfacility = 0;
int	cf_logfd = -1;

/* Global variables */

char *	ts_stamp = NULL;
char *	ts_shutdown = NULL;
char *	ts_boot = NULL;
time_t	boottime = 0;
time_t	starttime = 0;

/* The following are set by the signal handler */

volatile sig_atomic_t	exiting = 0;
volatile sig_atomic_t	reopenlog = 0;

/*
 * downtimed: system downtime monitoring and reporting daemon.
 *
 * This daemon sits in the background, periodically updating a time stamp
 * on the disk. If the daemon is killed with a signal associated with a
 * normal system shutdown procedure, it will record the shutdown time on
 * the disk. When the daemon is restarted during the next boot process,
 * it will report how long the system was down and whether it was properly
 * shut down or crashed.
 */

int
main(int argc, char *argv[])
{
	struct stat sb;
	time_t uptime;

	/* record daemon startup time for later use */
	starttime = time((time_t *)NULL);

	/* parse command line arguments */
	parseargs(argc, argv);

	/* protect our files from tampering by other users */
	umask(S_IWGRP | S_IWOTH);

	/* initialize logging subsystem */
	loginit();

	/* find out system boot time */
	boottime = getboottime();

	/* check if datadir exists */
	if (stat(cf_datadir, &sb) < 0 || !S_ISDIR(sb.st_mode)) {
		logwr(LOG_CRIT, "data directory %s does not exist", cf_datadir);
		errx(EX_CANTCREAT, "data directory %s does not exist",
		    cf_datadir);
	}

	/* set time stamp file names */
	if (asprintf(&ts_stamp, "%s/downtimed.stamp", cf_datadir) < 0 ||
	    asprintf(&ts_shutdown, "%s/downtimed.shutdown", cf_datadir) < 0
	    || asprintf(&ts_boot, "%s/downtimed.boot", cf_datadir) < 0) {
		logwr(LOG_CRIT, "asprintf failed, out of memory?");
		errx(EX_OSERR, "asprintf failed, out of memory?");
	}

#ifndef __APPLE__  /* under Mac OS X, use launchd(8) */
	/* run as daemon (fork and detach from controlling tty) */
	if (daemon(0, 0) < 0) {
		logwr(LOG_CRIT, "starting daemon failed: %s", strerror(errno));
		err(EX_OSERR, "starting daemon failed");
	}
#endif

	/* create pid file */
	if (makepidfile() < 0) {
		/* the error has been logged already in makepidfile() */
		exit(EX_UNAVAILABLE);
	}

	/* check & log startup status */
	report();

	/* set up the signal handlers */
	signal(SIGHUP, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	/* touch system boot time */
	touch(ts_boot, boottime);

	/*
	 * main loop: run until we receive a signal or system dies,
         * touching the time stamp file regularly
	 */
	while (exiting == 0) {
		touch(ts_stamp, 0);
		sleep(cf_sleep);

		if (reopenlog) {
			reopenlog = 0;
			logdeinit();
			loginit();
		}
	}

	/*
	 * Record normal shutdown. If using syslog for logging, this
	 * might fail because syslogd may have exited already.
	 */
	uptime = time((time_t *)NULL) - boottime;
	logwr(LOG_NOTICE, "shutting down, uptime %s (%d seconds)",
	    timestr_int(uptime), uptime);

	touch(ts_stamp, 0);
	touch(ts_shutdown, 0);

	/* We could write the downtime database shutdown record here
	 * in case of graceful shutdown, but we have chosen to update
	 * it consistently only at the program start.
	 */

	logdeinit();
	removepidfile();

	exit(EX_OK);
}

/* Find out system boot time */

static time_t
getboottime()
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(__DragonFly__) || defined(__APPLE__) \
    || defined(__FreeBSD_kernel__)
	/*
	 * BSDish systems have the boot time available through sysctl.
	 */
	int mib[2];
	struct timeval bt;
	size_t btsize;

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	btsize = sizeof(bt);

	if (sysctl(mib, 2, &bt, &btsize, (void *)NULL, 0) != -1 &&
	    bt.tv_sec != 0)
		return (bt.tv_sec);

#elif defined(__linux__)
	/*
	 * Linux systems have the boot time available through /proc filesystem.
	 */
	char str[1024];
	FILE *fp;
	time_t bt = 0;

	if ((fp = fopen("/proc/stat", "r")) != NULL) {
		while (fgets(str, sizeof(str), fp) != NULL)
			if (strncmp(str, "btime ", 6) == 0)
				if (sscanf(str, "btime %lu", &bt) == 1 &&
				    bt != 0) {
					fclose(fp);
					return (bt);
				}
		fclose(fp);
	}
#elif defined(__SVR4) && defined(HAVE_UTMPX_H)
	/*
	 * This should work on some SVR4 systems, such as Solaris
	 * and possibly HP-UX.
	 */
	struct utmpx ut, *utp;

	memset(&ut, 0, sizeof(ut));
	ut.ut_type = BOOT_TIME;
	if ((utp = getutxid(&ut)) != NULL) {
		time_t boottime;

		boottime = utp->ut_tv.tv_sec;
		endutxent();

		return (boottime);
	}
	endutxent();
#endif
	/*
	 * We fall through here if we do not have OS specific code
	 * implemented or if the OS specific code fails.
	 *
	 * The logic could be improved to stat() some files which are
	 * accessed or modified early at system startup (for example the
	 * system log, dmesg.boot, kernel, utmp, etc.) XXX
	 */

	logwr(LOG_ERR, "can not determine system boot time on this OS");

	return (starttime);	/* give up */
}

/* Update downtime database */

void
updatedowntimedb(time_t up, int crashed, time_t down)
{
	struct downtimedb dbent;
	int fd;

	if ((fd = open(cf_downtimedbfile, O_WRONLY | O_CREAT | O_APPEND,
	    DEFFILEMODE)) < 0) {
		logwr(LOG_ERR, "can not open %s: %s", cf_downtimedbfile,
		    strerror(errno));
		return;
	}

	/* ensure that padding bytes are zero */
	memset(&dbent, 0, sizeof(struct downtimedb));

	dbent.what = crashed ?
	    DOWNTIMEDB_WHAT_CRASH : DOWNTIMEDB_WHAT_SHUTDOWN;
	dbent.when = (uint64_t) down;

	if (downtimedb_write(fd, &dbent) < 0)
		logwr(LOG_ERR, "can not write to %s: %s", cf_downtimedbfile,
		    strerror(errno));

	/* ensure again that padding bytes are zero */
	memset(&dbent, 0, sizeof(struct downtimedb));

	dbent.what = DOWNTIMEDB_WHAT_UP;
	dbent.when = (uint64_t) up;

	if (downtimedb_write(fd, &dbent) < 0)
		logwr(LOG_ERR, "can not write to %s: %s", cf_downtimedbfile,
		    strerror(errno));

	close(fd);
}

/* Report the downtime and shutdown reason when starting up */

static void
report()
{
	struct stat sb_stamp, sb_shutdown, sb_oldboot;
	int have_stamp = 0, have_shutdown = 0, have_oldboot = 0;
	time_t olduptime, downtime;

	if (stat(ts_stamp, &sb_stamp) == 0)
		have_stamp = 1;

	if (stat(ts_shutdown, &sb_shutdown) == 0)
		have_shutdown = 1;

	if (stat(ts_boot, &sb_oldboot) == 0)
		have_oldboot = 1;

	if (!have_stamp && !have_shutdown && !have_oldboot) {
		logwr(LOG_NOTICE, "starting up first time, "
		    "no knowledge of downtime");
		return;
	}
	if (!have_stamp) {
		logwr(LOG_ERR, "no old run-time stamp (%s)", ts_stamp);
		return;
	}
	if (!have_oldboot) {
		logwr(LOG_ERR, "no old boot-time stamp (%s)", ts_boot);
		return;
	}
	if (have_stamp && have_shutdown &&
	    sb_shutdown.st_mtime < sb_stamp.st_mtime)
		have_shutdown = 0;

	olduptime =
	    (have_shutdown ? sb_shutdown.st_mtime : sb_stamp.st_mtime) -
	    sb_oldboot.st_mtime;

	downtime = boottime -
	    (have_shutdown ? sb_shutdown.st_mtime : sb_stamp.st_mtime);

	if (downtime < 0) {
		/*
		 * This happens if we quit and re-start the process (we
		 * normally only exit when system goes down.
		 */
		logwr(LOG_NOTICE, "restarted, system was not down");
		return;
	}

	logwr(LOG_NOTICE, "started %d seconds after boot",
	    starttime - boottime);

	if (cf_downtimedb)
		updatedowntimedb(boottime, !have_shutdown,
		    (have_shutdown ?
		    sb_shutdown.st_mtime : sb_stamp.st_mtime));

	if (have_shutdown) {
		logwr(LOG_NOTICE, "system shutdown at %s",
		    timestr_abs(sb_shutdown.st_mtime));
	} else {
		logwr(LOG_NOTICE, "system crashed at %s",
		    timestr_abs(sb_stamp.st_mtime));
	}

	logwr(LOG_NOTICE, "previous uptime was %s (%d seconds)",
	    timestr_int(olduptime), olduptime);

	logwr(LOG_NOTICE, "downtime was %s (%d seconds)",
	    timestr_int(downtime), downtime);
}

/* Handle signals */

static void
sighandler(int signum)
{

	if (signum == SIGINT || signum == SIGTERM)
		exiting = 1;

	if (signum == SIGHUP)
		reopenlog = 1;
}

/* Update time-stamp of file */

static void
touch(const char *fn, time_t t)
{
	struct timeval tv[2];
	int fd;

	if (t != 0) {
		tv[0].tv_sec = t;
		tv[0].tv_usec = 0;
		tv[1].tv_sec = t;
		tv[1].tv_usec = 0;
	}

	if (cf_fsync) {
		/* we need to open the file so that we can do fsync() to it */
		if ((fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC,
		    DEFFILEMODE)) < 0) {
			logwr(LOG_ERR, "%s: %s", fn, strerror(errno));
			return;
		}
		if (futimes(fd, t == 0 ? (struct timeval *)NULL : tv) < 0) {
			logwr(LOG_ERR, "%s: %s", fn, strerror(errno));
		} else
			fsync(fd);

		if (close(fd) < 0)
			logwr(LOG_ERR, "%s: %s", fn, strerror(errno));
	} else {
		/* not doing fsync(), no need to open file */
		if (utimes(fn, t == 0 ? (struct timeval *)NULL : tv) < 0)
			logwr(LOG_ERR, "%s: %s", fn, strerror(errno));
	}
}

/* Compatibility for systems without facilitynames in <syslog.h> */

#ifndef HAVE_SYSLOG_FACILITYNAMES

static const struct {
	const char	*c_name;
	const int	c_val;
} facilitynames[] = {
#ifdef LOG_KERN
	{ "kern",	LOG_KERN	},
#endif
#ifdef LOG_USER
	{ "user",	LOG_USER	},
#endif
#ifdef LOG_MAIL
	{ "mail",	LOG_MAIL	},
#endif
#ifdef LOG_DAEMON
	{ "daemon",	LOG_DAEMON	},
#endif
#ifdef LOG_AUTH
	{ "auth",	LOG_AUTH	},
#endif
#ifdef LOG_SYSLOG
	{ "syslog",	LOG_SYSLOG	},
#endif
#ifdef LOG_LPR
	{ "lpr",	LOG_LPR		},
#endif
#ifdef LOG_NEWS
	{ "news",	LOG_NEWS	},
#endif
#ifdef LOG_AUDIT
	{ "audit",	LOG_AUDIT	},
#endif
#ifdef LOG_CRON
	{ "cron",	LOG_CRON	},
#endif
#ifdef LOG_LOCAL0
	{ "local0",	LOG_LOCAL0	},
#endif
#ifdef LOG_LOCAL1
	{ "local1",	LOG_LOCAL1	},
#endif
#ifdef LOG_LOCAL2
	{ "local2",	LOG_LOCAL2	},
#endif
#ifdef LOG_LOCAL3
	{ "local3",	LOG_LOCAL3	},
#endif
#ifdef LOG_LOCAL4
	{ "local4",	LOG_LOCAL4	},
#endif
#ifdef LOG_LOCAL5
	{ "local5",	LOG_LOCAL5	},
#endif
#ifdef LOG_LOCAL6
	{ "local6",	LOG_LOCAL6	},
#endif
#ifdef LOG_LOCAL7
	{ "local7",	LOG_LOCAL7	},
#endif
	{ NULL,		-1		}
};


#endif /* !defined(HAVE_SYSLOG_FACILITYNAMES) */

/* Determine log destination & initialize */

static void
loginit()
{
	int i;

	if (strchr(cf_log, '/') == NULL) {
		/* Logging to syslog if there is no slash in the name. */

		for(i = 0; facilitynames[i].c_name != NULL; i++)
			if (strcmp(facilitynames[i].c_name, cf_log) == 0)
			    cf_logfacility = facilitynames[i].c_val;

		if (cf_logfacility == 0)
			errx(EX_USAGE,
			    "-l argument is not syslog facility or file path");

		openlog(PROGNAME, LOG_PID, cf_logfacility);
	} else {
		/* We are logging to a file. */

		if ((cf_logfd = open(cf_log, O_WRONLY | O_APPEND | O_CREAT,
		    DEFFILEMODE)) < 0)
			err(EX_CANTCREAT, "%s", cf_log);
	}
}

/* De-initialize & close logging */

static void
logdeinit()
{

	if (cf_logfd < 0) {
		closelog();
	} else {
		close(cf_logfd);
		cf_logfd = -1;
	}
}

/* Log a message */

static void
logwr(int pri, const char *fmt, ...)
{
	char *str, *str2;

	va_list ap;

	va_start(ap, fmt);

	if (cf_logfd < 0) {
		vsyslog(pri, fmt, ap);
	} else {
		if (vasprintf(&str, fmt, ap) < 0)
			goto err;

		if (asprintf(&str2, "%s: %s\n",
		    timestr_abs(time((time_t *) NULL)), str) < 0) {
			free(str);
			goto err;
		}

		/*
		 * Write a log entry to log file. We do not do any error
		 * checking because there is not much we can do if logging
		 * fails (most likely due to disk full situation) as we
		 * can not log the error. Even if logging fails, it still
		 * makes sense to keep running and updating timestamps.
		 */
		if (write(cf_logfd, str2, strlen(str2)) < 0)
			/* do nothing but keep compiler happy */
			;

		free(str2);
		free(str);
	}
err:
	va_end(ap);
}

/* Usage help & exit */

static void
usage()
{

	fputs("usage: " PROGNAME " [-DvS] [-d datadir] [-l log] [-p pidfile] "
	    "[-s sleep]\n", stderr);
	exit(EX_USAGE);
}

/* Output version information, default settings & exit */

static void
version()
{

	puts(PROGNAME " " PROGVERSION " - system downtime reporting daemon\n");

	puts("Copyright (c) 2009-2011 EPIPE Communications. "
	    "All rights reserved.");
	puts("This software is licensed under the terms and conditions of the "
	    "FreeBSD");
	puts("License which is also known as the Simplified BSD License. You "
	    "should have ");
	puts("received a copy of that license along with this software.\n");

	puts("Default settings:");
	printf("  log = %s\n", cf_log);
	printf("  pidfile = %s\n", cf_pidfile);
	printf("  datadir = %s\n", cf_datadir);
	printf("  downtimedbfile = %s\n", cf_downtimedbfile);
	printf("  sleep = %ld\n", cf_sleep);
	printf("  fsync = %d\n", cf_fsync);

#ifdef PACKAGE_URL
	puts("\nSee the following web site for more information and updates:");
	puts("  " PACKAGE_URL "\n");
#endif
	exit(EX_OK);
}

/* Handle command line arguments */

static void
parseargs(int argc, char *argv[])
{
	int c;
	char *p;

	while ((c = getopt(argc, argv, "Dd:l:p:s:Svh?")) != -1) {
		switch (c) {
		case 'D':
			cf_downtimedb = 0;
			break;
		case 'd':
			cf_datadir = optarg;
			break;
		case 'l':
			cf_log = optarg;
			break;
		case 'p':
			cf_pidfile = optarg;
			break;
		case 's':
			p = NULL;
			errno = 0;
			cf_sleep = strtol(optarg, &p, 10);
			if ((p != NULL && *p != '\0') || errno != 0)
				errx(EX_USAGE, "-s argument is not a number");
			break;
		case 'S':
			cf_fsync = 0;
			break;
		case 'v':
			version();
			/* NOTREACHED */
			break;
		case 'h':
		case '?':
		default:
			usage();
			/* NOTREACHED */
			break;
		}
	}
	if (argc != optind)
		usage();
}

/*
 * Create a pid file, return -1 on error. This is loosely based on
 * FreeBSD flopen.c.
 */

static int
makepidfile()
{
	char str[100];
	struct stat sb, sb2;
	int fd;

retry:
	if ((fd = open(cf_pidfile, O_WRONLY | O_CREAT, DEFFILEMODE)) < 0) {
		logwr(LOG_ERR, "can not open pid file %s: %s",
		    cf_pidfile, strerror(errno));
		return (-1);
	}
	if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)
			logwr(LOG_ERR, "another process is running already");
		else
			logwr(LOG_ERR, "can not flock pid file %s: %s",
			    cf_pidfile, strerror(errno));
		close(fd);
		return (-1);
	}
	if (stat(cf_pidfile, &sb) < 0) {
		/* disappeared from under our feet */
		close(fd);
		goto retry;
	}
	if (fstat(fd, &sb2) < 0) {
		/* can not happen :) */
		logwr(LOG_ERR, "can not fstat pid file %s: %s",
		    cf_pidfile, strerror(errno));
		close(fd);
		return (-1);
	}
	if (sb.st_dev != sb2.st_dev || sb.st_ino != sb2.st_ino) {
		/* changed under our feet */
		close(fd);
		goto retry;
	}
	if (ftruncate(fd, 0) < 0) {
		/* should not happen */
		logwr(LOG_ERR, "can not ftruncate pid file %s: %s",
			cf_pidfile, strerror(errno));
		close(fd);
		return (-1);
	}
	snprintf(str, sizeof(str), "%d\n", getpid());
	if (write(fd, str, strlen(str)) != strlen(str)) {
		logwr(LOG_ERR, "can not write pid file %s: %s",
		    cf_pidfile, strerror(errno));
		close(fd);
		return (-1);
	}

	/*
	 * NOTE: we leave the file open (although we can not access the
	 * file descriptor any more) so that we can retain the lock as
	 * long as the process is alive.
	 */

	return (0);
}

/* Remove pid file */

static void
removepidfile()
{

	/*
	 * We are not releasing the lock. We still have an open
	 * file descriptor to the unlink()ed file, but that is sorted
	 * out when we exit shortly after calling this function.
	 */

	unlink(cf_pidfile);
}

/* eof */
