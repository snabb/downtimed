/*-
 * Copyright (c) 2009-2010 EPIPE Communications. All rights reserved.
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

/* Standard includes that we need */

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <paths.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

#include "downtimedb.h"

/* Some global defines */

#define	PROGNAME "downtimes"

/* PACKAGE_VERSION is defined by autoconf, if used. */
#ifdef PACKAGE_VERSION
#define	PROGVERSION PACKAGE_VERSION
#else
#define	PROGVERSION "0.0undef"
#endif

/* Function prototypes */

int		main(int, char *[]);
static char *	timestr(time_t);
static void	report(int64_t, int, int64_t);
static char *	prettytime(time_t);
static void	version(void);
static void	usage(void);
static void	parseargs(int, char *[]);

/* Command line arguments with their defaults */

long	cf_sleep = 0;         /* adjust crash time according to sleep value */
char *	cf_downtimedbfile = PATH_DOWNTIMEDBFILE;
long	cf_n = -1;                 /* number of downtime records to display */

/* 
 * downtimes: display system downtime records made by downtimed(8)
 */

int
main(int argc, char *argv[])
{
	struct downtimedb dbent;
	struct stat sb;
	int64_t tdown, tadjust;
	int fd, ret, crashed;

	/* parse command line arguments */
	parseargs(argc, argv);

	if ((fd = open(cf_downtimedbfile, O_RDONLY)) < 0)
		err(EX_NOINPUT, "can not open %s", cf_downtimedbfile);

	if (fstat(fd, &sb) < 0)
		err(EX_NOINPUT, "can not stat %s", cf_downtimedbfile);

	if (sb.st_size % sizeof(struct downtimedb) != 0)
		errx(EX_DATAERR, "%s is corrupted", cf_downtimedbfile);

	if (cf_n == -1 || cf_n > sb.st_size / sizeof(struct downtimedb) / 2)
		cf_n = sb.st_size / sizeof(struct downtimedb) / 2;

	if (lseek(fd, -(cf_n * sizeof(struct downtimedb) * 2), SEEK_END) < 0)
		err(EX_DATAERR, "can not seek %s", cf_downtimedbfile);

	tdown = 0;
	tadjust = cf_sleep / 2;

	while ((ret = downtimedb_read(fd, &dbent)) > 0) {
		switch (dbent.what) {
		case DOWNTIMEDB_WHAT_SHUTDOWN:
			if (tdown != 0)
				report(dbent.when, 0, 0);
			tdown = dbent.when;
			crashed = 0;
			break;
		case DOWNTIMEDB_WHAT_CRASH:
			if (tdown != 0)
				report(dbent.when + tadjust, 1, 0);
			tdown = dbent.when;
			crashed = 1;
			break;
		case DOWNTIMEDB_WHAT_UP:
			report(crashed ? tdown + tadjust : tdown, 
			    crashed, dbent.when);
			tdown = 0;
			break;
		case DOWNTIMEDB_WHAT_NONE:
		default:
			break;
		}
	}

	if (tdown != 0)
		report(tdown, crashed, 0);

	if (ret < 0)
		err(EX_DATAERR, "error reading %s", cf_downtimedbfile);

	close(fd);
	exit(EX_OK);
}

char *
timestr(time_t t)
{
	static char str[100];
	struct tm *lt;

	if (t != 0) {
		if ((lt = localtime(&t)) == NULL)
			goto err;
		if (strftime(str, sizeof(str), "%F %T", lt) == 0)
			goto err;

		return (str);
	}
err:
	/* we have the backslashes here to avoid interpretation as trigraphs */
	return ("?\?\?\?-?\?-?\? ?\?:?\?:?\?");
}

/* Output one line of downtime report */

static void
report(int64_t td, int crashed, int64_t tu)
{
	printf("%s %s -> ", crashed ? "crash" : "down ", timestr((time_t) td));
	printf("up %s ", timestr((time_t) tu));

	if (tu != 0 && td != 0)
		printf("= %s (%Ld s)\n", 
		    prettytime((time_t)(tu - td)), tu - td);
	else
		printf("= unknown (? s)\n");
}

/* Stolen from top.c */

char *
prettytime(time_t t)
{
	int days, hrs, mins, secs;
	static char str[100];

#if 0
	t += 30;	/* WHY ? XXX */
#endif
	days = t / 86400;
	t %= 86400;
	hrs = t / 3600;
	t %= 3600;
	mins = t / 60;
	secs = t % 60;

	if (days > 0)
		snprintf(str, sizeof(str), "%d+%02d:%02d:%02d", 
		    days, hrs, mins, secs);
	else
		snprintf(str, sizeof(str), "%02d:%02d:%02d", 
		    hrs, mins, secs);

	return (str);
}

/* Usage help & exit */

static void
usage()
{

	fputs("usage: " PROGNAME " [-v] [-d downtimedbfile] [-n num] "
	    "[-s sleep]\n", stderr);
	exit(EX_USAGE);
}

/* Output version information, default settings & exit */

static void
version()
{

	puts(PROGNAME " " PROGVERSION " - display system downtime records "
	    "made by downtimed(8)\n");

	puts("Copyright (c) 2009-2010 EPIPE Communications. "
	    "All rights reserved.");
	puts("This software is licensed under the terms and conditions of the "
	    "FreeBSD");
	puts("License which is also known as the Simplified BSD License. You "
	    "should have ");
	puts("received a copy of that license along with this software.\n");

	puts("Default settings:");
	printf("  downtimedbfile = %s\n", cf_downtimedbfile);
	printf("  num = %ld\n", cf_n);
	printf("  sleep = %ld\n", cf_sleep);

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

	if (strlen(argv[0]) > 0 && argv[0][strlen(argv[0])-1] != 's')
		cf_n = 1;

	while ((c = getopt(argc, argv, "d:n:s:vh?")) != -1) {
		switch (c) {
		case 'd':
			cf_downtimedbfile = optarg;
			break;
		case 'n':
			p = NULL;
			errno = 0;
			cf_n = strtol(optarg, &p, 10);
			if ((p != NULL && *p != '\0') || errno != 0)
				errx(EX_USAGE, "-n argument is not a number");
			break;
		case 's':
			p = NULL;
			errno = 0;
			cf_sleep = strtol(optarg, &p, 10);
			if ((p != NULL && *p != '\0') || errno != 0)
				errx(EX_USAGE, "-s argument is not a number");
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

/* eof */
