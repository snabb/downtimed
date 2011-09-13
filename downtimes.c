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

/* Standard includes that we need */

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

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
static void	report(int64_t, int, int64_t);
static void	version(void);
static void	usage(void);
static void	parseargs(int, char *[]);

/* Command line arguments with their defaults */

static long	cf_sleep = 0; /* adjust crash time according to sleep value */
static char *	cf_downtimedbfile = PATH_DOWNTIMEDBFILE;
static long	cf_n = -1;         /* number of downtime records to display */
static char *	cf_timefmt = FMT_DATETIME;
static int	cf_utc = 0;                  /* set to display times in UTC */

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

	if ((fd = open(cf_downtimedbfile, O_RDONLY)) < 0) {
		fputs("Maybe the system has not been down yet?\n", stderr);
		err(EX_NOINPUT, "can not open %s", cf_downtimedbfile);
	}

	if (fstat(fd, &sb) < 0)
		err(EX_NOINPUT, "can not stat %s", cf_downtimedbfile);

	if (sb.st_size % sizeof(struct downtimedb) != 0)
		errx(EX_DATAERR, "%s is corrupted", cf_downtimedbfile);

	if ((cf_n == -1)
	    || (cf_n > sb.st_size / sizeof(struct downtimedb) / 2))
		cf_n = sb.st_size / sizeof(struct downtimedb) / 2;

	if (lseek(fd, sb.st_size - (cf_n * sizeof(struct downtimedb) * 2),
	    SEEK_SET) < 0)
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

/* Output one line of downtime report */

static void
report(int64_t td, int crashed, int64_t tu)
{
	printf("%s %s -> ", crashed ? "crash" : "down ", 
	    timestr_abs((time_t) td, cf_timefmt, cf_utc));

	/* Note that the printf() above and below is intentionally split
	   into two parts because timestr_abs() clobbers the returned
	   static string on each call. */

	printf("up %s ", timestr_abs((time_t) tu, cf_timefmt, cf_utc));

	/* timestr_int() returns string representing a relative time (time
	   period) such as 21+06:11:38 or 06:11:38 */

	if (tu != 0 && td != 0)
		printf("= %11s (%"PRIu64" s)\n",
		    timestr_int((time_t)(tu - td)), tu - td);
	else
		printf("= %11s (? s)\n", "unknown");
}

/* Usage help & exit */

static void
usage()
{

	fputs("usage: " PROGNAME " [-v] [-d downtimedbfile] [-n num] "
	    "[-s sleep] [-u]\n", stderr);
	exit(EX_USAGE);
}

/* Output version information, default settings & exit */

static void
version()
{

	puts(PROGNAME " " PROGVERSION " - display system downtime records "
	    "made by downtimed(8)\n");

	puts("Copyright (c) 2009-2011 EPIPE Communications. "
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
	printf("  timefmt = %s\n", cf_timefmt);
	printf("  utc = %d\n", cf_utc);

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

	while ((c = getopt(argc, argv, "d:f:n:s:uvh?")) != -1) {
		switch (c) {
		case 'd':
			cf_downtimedbfile = optarg;
			break;
		case 'f':
			cf_timefmt = optarg;
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
		case 'u':
			cf_utc = 1;
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
