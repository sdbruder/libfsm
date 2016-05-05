/* $Id$ */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/graph.h>
#include <fsm/out.h>
#include <fsm/exec.h>

#include "parser.h"

extern int optind;
extern char *optarg;

static void
usage(void)
{
	printf("usage: fsm [-acdhgdmrp] [-l <language>] [-n <prefix>]\n"
	       "           [-t <transformation>] [-e <execution> | -q <query>]\n");
}

static enum fsm_out
language(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum fsm_out format;
	} a[] = {
		{ "c",    FSM_OUT_C    },
		{ "csv",  FSM_OUT_CSV  },
		{ "dot",  FSM_OUT_DOT  },
		{ "fsm",  FSM_OUT_FSM  },
		{ "json", FSM_OUT_JSON }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].format;
		}
	}

	fprintf(stderr, "unrecognised output language; valid languages are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static int
query(struct fsm *fsm, const char *name)
{
	size_t i;

	struct {
		const char *name;
		int (*predicate)(const struct fsm *, const struct fsm_state *);
	} a[] = {
		{ "isdfa",      fsm_isdfa      },
		{ "dfa",        fsm_isdfa      }
/* XXX:
		{ "iscomplete", fsm_iscomplete },
		{ "hasend",     fsm_hasend     },
		{ "end",        fsm_hasend     },
		{ "accept",     fsm_hasend     },
		{ "hasaccept",  fsm_hasend     }
*/
	};

	assert(fsm != NULL);
	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return fsm_all(fsm, a[i].predicate);
		}
	}

	fprintf(stderr, "unrecognised query; valid queries are: "
		"iscomplete, isdfa, hasend\n");
	exit(EXIT_FAILURE);
}

static void
transform(struct fsm *fsm, const char *name)
{
	size_t i;

	struct {
		const char *name;
		int (*f)(struct fsm *);
	} a[] = {
/* XXX: needs predicate
		{ "complete",   fsm_complete    },
*/
		{ "complement",  fsm_complement  },
		{ "invert",      fsm_complement  },
		{ "reverse",     fsm_reverse     },
		{ "rev",         fsm_reverse     },
		{ "determinise", fsm_determinise },
		{ "dfa",         fsm_determinise },
		{ "todfa",       fsm_determinise },
		{ "min",         fsm_minimise    },
		{ "minimise",    fsm_minimise    }
	};

	assert(fsm != NULL);
	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			if (!a[i].f(fsm)) {
				fprintf(stderr, "couldn't transform\n");
				exit(EXIT_FAILURE);
			}
			return;
		}
	}

	fprintf(stderr, "unrecognised transformation; valid transformations are: "
		"complete, complement, reverse, determinise, minimise\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	enum fsm_out format;
	struct fsm *fsm;
	int print;

	static const struct fsm_outoptions outoptions_defaults;
	struct fsm_outoptions out_options = outoptions_defaults;

	format = FSM_OUT_FSM;
	print  = 0;

	/* XXX: makes no sense for e.g. fsm -h */
	fsm = fsm_parse(stdin);
	if (fsm == NULL) {
		exit(EXIT_FAILURE);
	}

	{
		int c;

		while (c = getopt(argc, argv, "hagn:l:de:cmrt:pq:"), c != -1) {
			switch (c) {
			case 'a': out_options.anonymous_states  = 1;      break;
			case 'g': out_options.fragment          = 1;      break;
			case 'c': out_options.consolidate_edges = 1;      break;
			case 'n': out_options.prefix            = optarg; break;

			case 'e': return NULL == fsm_exec(fsm, fsm_sgetc, &optarg);

			case 'p':
				print = 1;
				break;

			case 'l': format = language(optarg);     break;

			case 'd': transform(fsm, "determinise"); break;
			case 'm': transform(fsm, "minimise");    break;
			case 'r': transform(fsm, "reverse");     break;
			case 't': transform(fsm, optarg);        break;

			case 'q': exit(query(fsm, optarg));      break;

			case 'h':
				usage();
				exit(EXIT_SUCCESS);

			case '?':
			default:
				usage();
				exit(EXIT_FAILURE);
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc != 0) {
		usage();
		exit(EXIT_FAILURE);
	}

	if (print) {
		fsm_print(fsm, stdout, format, &out_options);
	}

	fsm_free(fsm);

	return 0;
}

