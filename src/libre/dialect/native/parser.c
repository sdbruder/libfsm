/*
 * Automatically generated from the files:
 *	src/libre/dialect/native/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 130 "src/libre/parser.act"


	#include <assert.h>
	#include <limits.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include <ctype.h>

	#include "libfsm/internal.h" /* XXX */

	#include <fsm/fsm.h>
	#include <fsm/bool.h>
	#include <fsm/pred.h>

	#include <re/re.h>

	#ifndef DIALECT
	#error DIALECT required
	#endif

	#define PASTE(a, b) a ## b
	#define CAT(a, b)   PASTE(a, b)

	#define LX_PREFIX CAT(lx_, DIALECT)

	#define LX_TOKEN   CAT(LX_PREFIX, _token)
	#define LX_STATE   CAT(LX_PREFIX, _lx)
	#define LX_NEXT    CAT(LX_PREFIX, _next)
	#define LX_INIT    CAT(LX_PREFIX, _init)

	#define DIALECT_COMP  CAT(comp_, DIALECT)

	/* XXX: get rid of this; use same %entry% for all grammars */
	#define DIALECT_ENTRY CAT(p_re__, DIALECT)

	#define TOK_CLASS__alnum  TOK_CLASS_ALNUM
	#define TOK_CLASS__alpha  TOK_CLASS_ALPHA
	#define TOK_CLASS__ascii  TOK_CLASS_ASCII
	#define TOK_CLASS__blank  TOK_CLASS_BLANK
	#define TOK_CLASS__cntrl  TOK_CLASS_CNTRL
	#define TOK_CLASS__digit  TOK_CLASS_DIGIT
	#define TOK_CLASS__graph  TOK_CLASS_GRAPH
	#define TOK_CLASS__lower  TOK_CLASS_LOWER
	#define TOK_CLASS__print  TOK_CLASS_PRINT
	#define TOK_CLASS__punct  TOK_CLASS_PUNCT
	#define TOK_CLASS__space  TOK_CLASS_SPACE
	#define TOK_CLASS__upper  TOK_CLASS_UPPER
	#define TOK_CLASS__word   TOK_CLASS_WORD
	#define TOK_CLASS__xdigit TOK_CLASS_XDIGIT

	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"
	#include "../../class.h"

	struct grp {
		struct fsm *set;
		struct fsm *dup;
	};

	typedef char         t_char;
	typedef const char * t_class;
	typedef unsigned     t_unsigned;
	typedef unsigned     t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef struct fsm * t_fsm;
	typedef struct fsm_state * t_fsm__state;
	typedef struct grp t_grp;

	struct act_state {
		enum LX_TOKEN lex_tok;
		enum LX_TOKEN lex_tok_save;
		int overlap; /* permit overlap in groups */

		/*
		 * Lexical position stored for syntax errors.
		 */
		struct re_pos synstart;
		struct re_pos synend;

		/*
		 * Lexical positions stored for errors which describe multiple tokens.
		 * We're able to store these without needing a stack, because these are
		 * non-recursive productions.
		 */
		struct re_pos groupstart; struct re_pos groupend;
		struct re_pos rangestart; struct re_pos rangeend;
		struct re_pos countstart; struct re_pos countend;
	};

	struct lex_state {
		struct LX_STATE lx;
		struct lx_dynbuf buf; /* XXX: unneccessary since we're lexing from a string */

		int (*f)(void *opaque);
		void *opaque;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   (TOK_ERROR)
	#define ADVANCE_LEXER    do { mark(&act_state->synstart, &lex_state->lx.start); \
	                              mark(&act_state->synend,   &lex_state->lx.end);   \
	                              act_state->lex_tok = LX_NEXT(&lex_state->lx); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

	static void
	mark(struct re_pos *r, const struct lx_pos *pos)
	{
		assert(r != NULL);
		assert(pos != NULL);

		r->byte = pos->byte;
	}

	/* TODO: centralise perhaps */
	static void
	snprintdots(char *s, size_t sz, const char *msg)
	{
		size_t n;

		assert(s != NULL);
		assert(sz > 3);
		assert(msg != NULL);

		n = sprintf(s, "%.*s", (int) sz - 3 - 1, msg);
		if (n == sz - 3 - 1) {
			strcpy(s + sz, "...");
		}
	}

	/* TODO: centralise */
	/* XXX: escaping really depends on dialect */
	static const char *
	escchar(char *s, size_t sz, int c)
	{
		size_t i;

		const struct {
			int c;
			const char *s;
		} a[] = {
			{ '\\', "\\\\" },

			{ '^',  "\\^"  },
			{ '-',  "\\-"  },
			{ ']',  "\\]"  },
			{ '[',  "\\["  },

			{ '\f', "\\f"  },
			{ '\n', "\\n"  },
			{ '\r', "\\r"  },
			{ '\t', "\\t"  },
			{ '\v', "\\v"  }
		};

		assert(s != NULL);
		assert(sz >= 5);

		(void) sz;

		for (i = 0; i < sizeof a / sizeof *a; i++) {
			if (a[i].c == c) {
				return a[i].s;
			}
		}

		if (!isprint((unsigned char) c)) {
			sprintf(s, "\\x%02X", (unsigned char) c);
			return s;
		}

		sprintf(s, "%c", c);
		return s;
	}

	static int
	escputc(int c, FILE *f)
	{
		char s[5];

		return fputs(escchar(s, sizeof s, c), f);
	}

	static int
	addedge_literal(struct fsm *fsm, enum re_flags flags,
		struct fsm_state *from, struct fsm_state *to, char c)
	{
		assert(fsm != NULL);
		assert(from != NULL);
		assert(to != NULL);

		if (flags & RE_ICASE) {
			if (!fsm_addedge_literal(fsm, from, to, tolower((unsigned char) c))) {
				return 0;
			}

			if (!fsm_addedge_literal(fsm, from, to, toupper((unsigned char) c))) {
				return 0;
			}
		} else {
			if (!fsm_addedge_literal(fsm, from, to, c)) {
				return 0;
			}
		}

		return 1;
	}

	static int
	group_add(struct grp *g, enum re_flags flags, char c)
	{
		const struct fsm_state *p;
		struct fsm_state *start, *end;
		struct fsm *fsm;
		char a[2];
		char *s = a;

		assert(g != NULL);

		a[0] = c;
		a[1] = '\0';

		errno = 0;
		p = fsm_exec(g->set, fsm_sgetc, &s);
		if (p == NULL && errno != 0) {
			return -1;
		}

		if (p == NULL) {
			fsm = g->set;
		} else {
			fsm = g->dup;
		}

		start = fsm_getstart(fsm);
		assert(start != NULL);

		end = fsm_addstate(fsm);
		if (end == NULL) {
			return -1;
		}

		fsm_setend(fsm, end, 1);

		if (!addedge_literal(fsm, flags, start, end, c)) {
			return -1;
		}

		return 0;
	}

	static struct fsm *
	fsm_new_blank(const struct fsm_options *opt)
	{
		struct fsm *new;
		struct fsm_state *start;

		new = fsm_new(opt);
		if (new == NULL) {
			return NULL;
		}

		start = fsm_addstate(new);
		if (start == NULL) {
			goto error;
		}

		fsm_setstart(new, start);

		return new;

	error:

		fsm_free(new);

		return NULL;
	}

	static struct fsm *
	fsm_new_any(const struct fsm_options *opt)
	{
		struct fsm_state *a, *b;
		struct fsm *new;

		assert(opt != NULL);

		new = fsm_new(opt);
		if (new == NULL) {
			return NULL;
		}

		a = fsm_addstate(new);
		if (a == NULL) {
			goto error;
		}

		b = fsm_addstate(new);
		if (b == NULL) {
			goto error;
		}

		/* TODO: provide as a class. for utf8, this would be /./ */
		if (!fsm_addedge_any(new, a, b)) {
			goto error;
		}

		fsm_setstart(new, a);
		fsm_setend(new, b, 1);

		return new;

	error:

		fsm_free(new);

		return NULL;
	}

	/* XXX: to go when dups show all spellings for group overlap */
	static const struct fsm_state *
	fsm_any(const struct fsm *fsm,
		int (*predicate)(const struct fsm *, const struct fsm_state *))
	{
		const struct fsm_state *s;

		assert(fsm != NULL);
		assert(predicate != NULL);

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (!predicate(fsm, s)) {
				return s;
			}
		}

		return NULL;
	}

#line 361 "src/libre/dialect/native/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_anchor(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_expr_C_Clist_Hof_Halts_C_Calt(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_173(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_group(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_178(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_group_C_Cgroup_Hbody(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_190(fsm, flags, lex_state, act_state, err, t_grp *, t_char *, t_pos *);
static void p_expr(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_194(fsm, flags, lex_state, act_state, err, t_fsm__state *, t_fsm__state *, t_pos *, t_unsigned *);
extern void p_re__native(fsm, flags, lex_state, act_state, err);
static void p_group_C_Clist_Hof_Hterms(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_expr_C_Clist_Hof_Hatoms(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_group_C_Cgroup_Hbm(fsm, flags, lex_state, act_state, err, t_grp *);
static void p_expr_C_Clist_Hof_Halts(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_expr_C_Catom(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state *);
static void p_literal(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_anchor(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pred ZIp;

		/* BEGINNING OF INLINE: 142 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_END):
				{
					/* BEGINNING OF EXTRACT: END */
					{
#line 528 "src/libre/parser.act"

		switch (flags & RE_ANCHOR) {
		case RE_TEXT:  ZIp = RE_EOT; break;
		case RE_MULTI: ZIp = RE_EOL; break;
		case RE_ZONE:  ZIp = RE_EOZ; break;

		default:
			/* TODO: raise error */
			ZIp = 0U;
		}
	
#line 420 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: END */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_START):
				{
					/* BEGINNING OF EXTRACT: START */
					{
#line 516 "src/libre/parser.act"

		switch (flags & RE_ANCHOR) {
		case RE_TEXT:  ZIp = RE_SOT; break;
		case RE_MULTI: ZIp = RE_SOL; break;
		case RE_ZONE:  ZIp = RE_SOZ; break;

		default:
			/* TODO: raise error */
			ZIp = 0U;
		}
	
#line 442 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: START */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 142 */
		/* BEGINNING OF ACTION: add-pred */
		{
#line 812 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

/* TODO:
		if (!fsm_addedge_predicate(fsm, (ZIx), (ZIy), (ZIp))) {
			goto ZL1;
		}
*/
	
#line 466 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: add-pred */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Clist_Hof_Halts_C_Calt(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 799 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 494 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF ACTION: add-epsilon */
		{
#line 806 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIz))) {
			goto ZL1;
		}
	
#line 505 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: add-epsilon */
		p_expr_C_Clist_Hof_Hatoms (fsm, flags, lex_state, act_state, err, ZIz, ZIy);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_173(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZI169, t_fsm__state ZI170, t_fsm__state *ZO171, t_fsm__state *ZO172)
{
	t_fsm__state ZI171;
	t_fsm__state ZI172;

ZL2_173:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			ADVANCE_LEXER;
			p_expr_C_Clist_Hof_Halts_C_Calt (fsm, flags, lex_state, act_state, err, ZI169, ZI170);
			/* BEGINNING OF INLINE: 173 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_173;
			}
			/* END OF INLINE: 173 */
		}
		/* UNREACHED */
	default:
		{
			ZI171 = ZI169;
			ZI172 = ZI170;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO171 = ZI171;
	*ZO172 = ZI172;
}

static void
p_group(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZIg;

		/* BEGINNING OF ACTION: make-group */
		{
#line 585 "src/libre/parser.act"

		(ZIg).set = fsm_new_blank(fsm->opt);
		if ((ZIg).set == NULL) {
			goto ZL1;
		}

		(ZIg).dup = fsm_new_blank(fsm->opt);
		if ((ZIg).dup == NULL) {
			fsm_free((ZIg).set);
			goto ZL1;
		}
	
#line 584 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: make-group */
		p_group_C_Cgroup_Hbm (fsm, flags, lex_state, act_state, err, &ZIg);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: group-to-states */
		{
#line 710 "src/libre/parser.act"

		int r;

		r = fsm_empty((&ZIg)->dup);
		if (r == -1) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		if (!r) {
			const struct fsm_state *end;

			/* TODO: would like to show the original spelling verbatim, too */

			/* XXX: this is just one example; really I want to show the entire set */
			end = fsm_any((&ZIg)->dup, fsm_isend);
			assert(end != NULL);

			if (-1 == fsm_example((&ZIg)->dup, end, err->dup, sizeof err->dup)) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			/* XXX: to return when we can show minimal coverage again */
			strcpy(err->set, err->dup);

			err->e  = RE_EOVERLAP;
			goto ZL1;
		}

		if (!fsm_minimise((&ZIg)->set)) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		/* TODO: centralise as fsm_unionxy() perhaps */
		{
			struct fsm_state *a, *b;
			struct fsm_state *end;

			a = fsm_getstart(fsm);
			b = fsm_getstart((&ZIg)->set);

			end = fsm_collate((&ZIg)->set, fsm_isend);
			if (end == NULL) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			if (!fsm_merge(fsm, (&ZIg)->set)) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			fsm_setstart(fsm, a);

			if (!fsm_addedge_epsilon(fsm, (ZIx), b)) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			fsm_setend(fsm, end, 0);

			if (!fsm_addedge_epsilon(fsm, end, (ZIy))) {
				err->e = RE_EERRNO;
				goto ZL1;
			}

			fsm_free((&ZIg)->dup);
		}
	
#line 666 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: group-to-states */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_178(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;
			t_pos ZI122;
			t_pos ZI123;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 390 "src/libre/parser.act"

		ZIb = '-';
		ZI122 = lex_state->lx.start;
		ZI123   = lex_state->lx.end;
	
#line 694 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 619 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags, (ZIb))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 707 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: group-add-char */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Cgroup_Hbody(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 115 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				{
					t_char ZI175;
					t_pos ZI176;
					t_pos ZI177;

					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 401 "src/libre/parser.act"

		ZI175 = ']';
		ZI176 = lex_state->lx.start;
		ZI177   = lex_state->lx.end;
	
#line 747 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 619 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags, (ZI175))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 760 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: group-add-char */
					p_178 (fsm, flags, lex_state, act_state, err, ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_RANGE):
				{
					t_char ZIc;
					t_pos ZI118;
					t_pos ZI119;

					/* BEGINNING OF EXTRACT: RANGE */
					{
#line 390 "src/libre/parser.act"

		ZIc = '-';
		ZI118 = lex_state->lx.start;
		ZI119   = lex_state->lx.end;
	
#line 784 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: RANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 619 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags, (ZIc))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 797 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: group-add-char */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 115 */
		p_group_C_Clist_Hof_Hterms (fsm, flags, lex_state, act_state, err, ZIg);
		/* BEGINNING OF INLINE: 124 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 124 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_190(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg, t_char *ZI187, t_pos *ZI188)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;
			t_pos ZIend;

			/* BEGINNING OF INLINE: 98 */
			{
				{
					t_char ZI99;
					t_pos ZI100;
					t_pos ZI101;

					switch (CURRENT_TERMINAL) {
					case (TOK_RANGE):
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 390 "src/libre/parser.act"

		ZI99 = '-';
		ZI100 = lex_state->lx.start;
		ZI101   = lex_state->lx.end;
	
#line 849 "src/libre/dialect/native/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
				}
				goto ZL2;
			ZL3:;
				{
					/* BEGINNING OF ACTION: err-expected-range */
					{
#line 996 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXRANGE;
		}
	
#line 869 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: err-expected-range */
				}
			ZL2:;
			}
			/* END OF INLINE: 98 */
			/* BEGINNING OF INLINE: 102 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						t_pos ZI106;

						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 508 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZI106 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIb = lex_state->buf.a[0];
	
#line 895 "src/libre/dialect/native/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_HEX):
					{
						t_pos ZI105;

						/* BEGINNING OF EXTRACT: HEX */
						{
#line 483 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI105 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		errno = 0;

		u = strtoul(lex_state->buf.a + 2, &e, 16);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIb = (char) (unsigned char) u;
	
#line 935 "src/libre/dialect/native/parser.c"
						}
						/* END OF EXTRACT: HEX */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_OCT):
					{
						t_pos ZI103;

						/* BEGINNING OF EXTRACT: OCT */
						{
#line 455 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI103 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		errno = 0;

		u = strtoul(lex_state->buf.a + 1, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIb = (char) (unsigned char) u;
	
#line 975 "src/libre/dialect/native/parser.c"
						}
						/* END OF EXTRACT: OCT */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_RANGE):
					{
						t_pos ZI107;

						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 390 "src/libre/parser.act"

		ZIb = '-';
		ZI107 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 993 "src/libre/dialect/native/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 102 */
			/* BEGINNING OF ACTION: mark-range */
			{
#line 1022 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI188));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 1011 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: group-add-range */
			{
#line 688 "src/libre/parser.act"

		int i;

		if ((unsigned char) (ZIb) < (unsigned char) (*ZI187)) {
			char a[5], b[5];

			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);

			sprintf(err->set, "%s-%s",
				escchar(a, sizeof a, (*ZI187)), escchar(b, sizeof b, (ZIb)));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		for (i = (unsigned char) (*ZI187); i <= (unsigned char) (ZIb); i++) {
			if (-1 == group_add((ZIg), flags, (char) i)) {
				err->e = RE_EERRNO;
				goto ZL1;
			}
		}
	
#line 1038 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: group-add-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 619 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags, (*ZI187))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 1054 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: group-add-char */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr_C_Clist_Hof_Halts (fsm, flags, lex_state, act_state, err, ZIx, ZIy);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_194(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state *ZIx, t_fsm__state *ZIy, t_pos *ZI191, t_unsigned *ZI193)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI151;
			t_pos ZIend;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 412 "src/libre/parser.act"

		ZI151 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1103 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 1027 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI191));
		mark(&act_state->countend,   &(ZIend));
	
#line 1114 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 868 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI193) < (*ZI193)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZI193);
			err->n = (*ZI193);
			goto ZL1;
		}

		if ((*ZI193) == 0) {
			if (!fsm_addedge_epsilon(fsm, (*ZIx), (*ZIy))) {
				goto ZL1;
			}
		}

		b = (*ZIy);

		for (i = 1; i < (*ZI193); i++) {
			a = fsm_state_duplicatesubgraphx(fsm, (*ZIx), &b);
			if (a == NULL) {
				goto ZL1;
			}

			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			if (!fsm_addedge_epsilon(fsm, (*ZIy), a)) {
				goto ZL1;
			}

			if (i >= (*ZI193)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 1162 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: count-m-to-n */
		}
		break;
	case (TOK_SEP):
		{
			t_unsigned ZIn;
			t_pos ZIend;
			t_pos ZI154;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 547 "src/libre/parser.act"

		unsigned long u;
		char *e;

		u = strtoul(lex_state->buf.a, &e, 10);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UINT_MAX) {
			err->e = RE_ECOUNTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL1;
		}

		ZIn = (unsigned int) u;
	
#line 1198 "src/libre/dialect/native/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSECOUNT):
				/* BEGINNING OF EXTRACT: CLOSECOUNT */
				{
#line 412 "src/libre/parser.act"

		ZIend = lex_state->lx.start;
		ZI154   = lex_state->lx.end;
	
#line 1215 "src/libre/dialect/native/parser.c"
				}
				/* END OF EXTRACT: CLOSECOUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 1027 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI191));
		mark(&act_state->countend,   &(ZIend));
	
#line 1230 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 868 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((ZIn) < (*ZI193)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZI193);
			err->n = (ZIn);
			goto ZL1;
		}

		if ((*ZI193) == 0) {
			if (!fsm_addedge_epsilon(fsm, (*ZIx), (*ZIy))) {
				goto ZL1;
			}
		}

		b = (*ZIy);

		for (i = 1; i < (ZIn); i++) {
			a = fsm_state_duplicatesubgraphx(fsm, (*ZIx), &b);
			if (a == NULL) {
				goto ZL1;
			}

			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			if (!fsm_addedge_epsilon(fsm, (*ZIy), a)) {
				goto ZL1;
			}

			if (i >= (*ZI193)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 1278 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: count-m-to-n */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

void
p_re__native(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIx;
		t_fsm__state ZIy;

		/* BEGINNING OF ACTION: make-states */
		{
#line 572 "src/libre/parser.act"

		assert(fsm != NULL);
		/* TODO: assert fsm is empty */

		(ZIx) = fsm_getstart(fsm);
		assert((ZIx) != NULL);

		(ZIy) = fsm_addstate(fsm);
		if ((ZIy) == NULL) {
			goto ZL1;
		}

		fsm_setend(fsm, (ZIy), 1);
	
#line 1321 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 165 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR): case (TOK_START):
			case (TOK_END):
				{
					p_expr (fsm, flags, lex_state, act_state, err, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL3;
					}
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 806 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 1348 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-alts */
				{
#line 990 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
	
#line 1365 "src/libre/dialect/native/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 165 */
		/* BEGINNING OF INLINE: 166 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_EOF):
					break;
				default:
					goto ZL5;
				}
				ADVANCE_LEXER;
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-eof */
				{
#line 1014 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
	
#line 1394 "src/libre/dialect/native/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 166 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Clist_Hof_Hterms(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_group_C_Clist_Hof_Hterms:;
	{
		/* BEGINNING OF INLINE: 111 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_char ZI187;
					t_pos ZI188;
					t_pos ZI189;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 508 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZI188 = lex_state->lx.start;
		ZI189   = lex_state->lx.end;

		ZI187 = lex_state->buf.a[0];
	
#line 1437 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
					p_190 (fsm, flags, lex_state, act_state, err, ZIg, &ZI187, &ZI188);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_ESC):
				{
					t_char ZIc;

					/* BEGINNING OF EXTRACT: ESC */
					{
#line 436 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}
	
#line 1471 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 619 "src/libre/parser.act"

		if (-1 == group_add((ZIg), flags, (ZIc))) {
			err->e = RE_EERRNO;
			goto ZL4;
		}
	
#line 1484 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: group-add-char */
				}
				break;
			case (TOK_HEX):
				{
					t_char ZI183;
					t_pos ZI184;
					t_pos ZI185;

					/* BEGINNING OF EXTRACT: HEX */
					{
#line 483 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI184 = lex_state->lx.start;
		ZI185   = lex_state->lx.end;

		errno = 0;

		u = strtoul(lex_state->buf.a + 2, &e, 16);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL4;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL4;
		}

		ZI183 = (char) (unsigned char) u;
	
#line 1525 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
					p_190 (fsm, flags, lex_state, act_state, err, ZIg, &ZI183, &ZI184);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_OCT):
				{
					t_char ZI179;
					t_pos ZI180;
					t_pos ZI181;

					/* BEGINNING OF EXTRACT: OCT */
					{
#line 455 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI180 = lex_state->lx.start;
		ZI181   = lex_state->lx.end;

		errno = 0;

		u = strtoul(lex_state->buf.a + 1, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL4;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL4;
		}

		ZI179 = (char) (unsigned char) u;
	
#line 1572 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
					p_190 (fsm, flags, lex_state, act_state, err, ZIg, &ZI179, &ZI180);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_CLASS__alnum): case (TOK_CLASS__alpha): case (TOK_CLASS__ascii): case (TOK_CLASS__blank):
			case (TOK_CLASS__cntrl): case (TOK_CLASS__digit): case (TOK_CLASS__graph): case (TOK_CLASS__lower):
			case (TOK_CLASS__print): case (TOK_CLASS__punct): case (TOK_CLASS__space): case (TOK_CLASS__upper):
			case (TOK_CLASS__word): case (TOK_CLASS__xdigit):
				{
					t_fsm ZIk;

					/* BEGINNING OF INLINE: group::class */
					{
						switch (CURRENT_TERMINAL) {
						case (TOK_CLASS__alnum):
							{
								/* BEGINNING OF EXTRACT: CLASS_alnum */
								{
#line 416 "src/libre/parser.act"
 ZIk = class_alnum_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1599 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_alnum */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__alpha):
							{
								/* BEGINNING OF EXTRACT: CLASS_alpha */
								{
#line 417 "src/libre/parser.act"
 ZIk = class_alpha_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1611 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_alpha */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__ascii):
							{
								/* BEGINNING OF EXTRACT: CLASS_ascii */
								{
#line 418 "src/libre/parser.act"
 ZIk = class_ascii_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1623 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_ascii */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__blank):
							{
								/* BEGINNING OF EXTRACT: CLASS_blank */
								{
#line 419 "src/libre/parser.act"
 ZIk = class_blank_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1635 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_blank */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__cntrl):
							{
								/* BEGINNING OF EXTRACT: CLASS_cntrl */
								{
#line 420 "src/libre/parser.act"
 ZIk = class_cntrl_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1647 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_cntrl */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__digit):
							{
								/* BEGINNING OF EXTRACT: CLASS_digit */
								{
#line 421 "src/libre/parser.act"
 ZIk = class_digit_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1659 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_digit */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__graph):
							{
								/* BEGINNING OF EXTRACT: CLASS_graph */
								{
#line 422 "src/libre/parser.act"
 ZIk = class_graph_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1671 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_graph */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__lower):
							{
								/* BEGINNING OF EXTRACT: CLASS_lower */
								{
#line 423 "src/libre/parser.act"
 ZIk = class_lower_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1683 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_lower */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__print):
							{
								/* BEGINNING OF EXTRACT: CLASS_print */
								{
#line 424 "src/libre/parser.act"
 ZIk = class_print_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1695 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_print */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__punct):
							{
								/* BEGINNING OF EXTRACT: CLASS_punct */
								{
#line 425 "src/libre/parser.act"
 ZIk = class_punct_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1707 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_punct */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__space):
							{
								/* BEGINNING OF EXTRACT: CLASS_space */
								{
#line 426 "src/libre/parser.act"
 ZIk = class_space_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1719 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_space */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__upper):
							{
								/* BEGINNING OF EXTRACT: CLASS_upper */
								{
#line 427 "src/libre/parser.act"
 ZIk = class_upper_fsm(fsm->opt);  if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1731 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_upper */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__word):
							{
								/* BEGINNING OF EXTRACT: CLASS_word */
								{
#line 428 "src/libre/parser.act"
 ZIk = class_word_fsm(fsm->opt);   if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1743 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_word */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_CLASS__xdigit):
							{
								/* BEGINNING OF EXTRACT: CLASS_xdigit */
								{
#line 429 "src/libre/parser.act"
 ZIk = class_xdigit_fsm(fsm->opt); if (ZIk == NULL) { err->e = RE_EERRNO; goto ZL4; } 
#line 1755 "src/libre/dialect/native/parser.c"
								}
								/* END OF EXTRACT: CLASS_xdigit */
								ADVANCE_LEXER;
							}
							break;
						default:
							goto ZL4;
						}
					}
					/* END OF INLINE: group::class */
					/* BEGINNING OF ACTION: group-add-class */
					{
#line 633 "src/libre/parser.act"

		struct fsm *q;
		int r;

		/* TODO: maybe it is worth using carryopaque, after the entire group is constructed */
		{
			struct fsm *a, *b;

			a = fsm_clone((ZIg)->set);
			if (a == NULL) {
				err->e = RE_EERRNO;
				goto ZL4;
			}

			b = fsm_clone((ZIk));
			if (b == NULL) {
				fsm_free(a);
				err->e = RE_EERRNO;
				goto ZL4;
			}

			q = fsm_intersect(a, b);
			if (q == NULL) {
				fsm_free(a);
				fsm_free(b);
				err->e = RE_EERRNO;
				goto ZL4;
			}

			r = fsm_empty(q);

			if (r == -1) {
				err->e = RE_EERRNO;
				goto ZL4;
			}
		}

		if (!r) {
			(ZIg)->dup = fsm_union((ZIg)->dup, q);
			if ((ZIg)->dup == NULL) {
				err->e = RE_EERRNO;
				goto ZL4;
			}
		} else {
			fsm_free(q);

			(ZIg)->set = fsm_union((ZIg)->set, (ZIk));
			if ((ZIg)->set == NULL) {
				err->e = RE_EERRNO;
				goto ZL4;
			}

			/* we need a DFA here for sake of fsm_exec() identifying duplicates */
			if (!fsm_determinise((ZIg)->set)) {
				err->e = RE_EERRNO;
				goto ZL4;
			}
		}
	
#line 1828 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: group-add-class */
				}
				break;
			default:
				goto ZL4;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-term */
				{
#line 972 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXTERM;
		}
	
#line 1847 "src/libre/dialect/native/parser.c"
				}
				/* END OF ACTION: err-expected-term */
			}
		ZL3:;
		}
		/* END OF INLINE: 111 */
		/* BEGINNING OF INLINE: 112 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLASS__alnum): case (TOK_CLASS__alpha): case (TOK_CLASS__ascii): case (TOK_CLASS__blank):
			case (TOK_CLASS__cntrl): case (TOK_CLASS__digit): case (TOK_CLASS__graph): case (TOK_CLASS__lower):
			case (TOK_CLASS__print): case (TOK_CLASS__punct): case (TOK_CLASS__space): case (TOK_CLASS__upper):
			case (TOK_CLASS__word): case (TOK_CLASS__xdigit): case (TOK_ESC): case (TOK_OCT):
			case (TOK_HEX): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: group::list-of-terms */
					goto ZL2_group_C_Clist_Hof_Hterms;
					/* END OF INLINE: group::list-of-terms */
				}
				/* UNREACHED */
			default:
				break;
			}
		}
		/* END OF INLINE: 112 */
	}
}

static void
p_expr_C_Clist_Hof_Hatoms(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Clist_Hof_Hatoms:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 799 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 1895 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF INLINE: 158 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_START): case (TOK_END):
				{
					p_anchor (fsm, flags, lex_state, act_state, err, ZIx, ZIz);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_ANY): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
				{
					p_expr_C_Catom (fsm, flags, lex_state, act_state, err, ZIx, &ZIz);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 158 */
		/* BEGINNING OF INLINE: 159 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR): case (TOK_START):
			case (TOK_END):
				{
					/* BEGINNING OF INLINE: expr::list-of-atoms */
					ZIx = ZIz;
					goto ZL2_expr_C_Clist_Hof_Hatoms;
					/* END OF INLINE: expr::list-of-atoms */
				}
				/* UNREACHED */
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 806 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 1948 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
		}
		/* END OF INLINE: 159 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Cgroup_Hbm(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_grp *ZIg)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pos ZIstart;
		t_pos ZI127;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			/* BEGINNING OF EXTRACT: OPENGROUP */
			{
#line 396 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI127   = lex_state->lx.end;
	
#line 1982 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: OPENGROUP */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF INLINE: 128 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_INVERT):
				{
					t_char ZI129;

					/* BEGINNING OF EXTRACT: INVERT */
					{
#line 386 "src/libre/parser.act"

		ZI129 = '^';
	
#line 2003 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: INVERT */
					ADVANCE_LEXER;
					p_group_C_Cgroup_Hbody (fsm, flags, lex_state, act_state, err, ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: invert-group */
					{
#line 603 "src/libre/parser.act"

		struct fsm *any;

		any = fsm_new_any(fsm->opt);
		if (any == NULL) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		(ZIg)->set = fsm_subtract(any, (ZIg)->set);
		if ((ZIg)->set == NULL) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		/*
		 * Note we don't invert the dup set here; duplicates are always
		 * kept in the positive.
		 */
	
#line 2035 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: invert-group */
				}
				break;
			case (TOK_CLOSEGROUP): case (TOK_RANGE): case (TOK_CLASS__alnum): case (TOK_CLASS__alpha):
			case (TOK_CLASS__ascii): case (TOK_CLASS__blank): case (TOK_CLASS__cntrl): case (TOK_CLASS__digit):
			case (TOK_CLASS__graph): case (TOK_CLASS__lower): case (TOK_CLASS__print): case (TOK_CLASS__punct):
			case (TOK_CLASS__space): case (TOK_CLASS__upper): case (TOK_CLASS__word): case (TOK_CLASS__xdigit):
			case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
				{
					p_group_C_Cgroup_Hbody (fsm, flags, lex_state, act_state, err, ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 128 */
		/* BEGINNING OF INLINE: 130 */
		{
			{
				t_char ZI131;
				t_pos ZI132;
				t_pos ZIend;

				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEGROUP):
					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 401 "src/libre/parser.act"

		ZI131 = ']';
		ZI132 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2075 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					break;
				default:
					goto ZL4;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: mark-group */
				{
#line 1017 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 2090 "src/libre/dialect/native/parser.c"
				}
				/* END OF ACTION: mark-group */
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-closegroup */
				{
#line 1002 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
	
#line 2105 "src/libre/dialect/native/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
			}
		ZL3:;
		}
		/* END OF INLINE: 130 */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-groupbody */
		{
#line 1008 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXGROUPBODY;
		}
	
#line 2124 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: err-expected-groupbody */
	}
}

static void
p_expr_C_Clist_Hof_Halts(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZI169, t_fsm__state ZI170)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZI171;
		t_fsm__state ZI172;

		p_expr_C_Clist_Hof_Halts_C_Calt (fsm, flags, lex_state, act_state, err, ZI169, ZI170);
		p_173 (fsm, flags, lex_state, act_state, err, ZI169, ZI170, &ZI171, &ZI172);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Catom(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 147 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-any */
					{
#line 834 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((*ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}
	
#line 2177 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: add-any */
				}
				break;
			case (TOK_OPENSUB):
				{
					ADVANCE_LEXER;
					p_expr (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
					switch (CURRENT_TERMINAL) {
					case (TOK_CLOSESUB):
						break;
					case (ERROR_TERMINAL):
						RESTORE_LEXER;
						goto ZL1;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OPENGROUP):
				{
					p_group (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
				{
					p_literal (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 147 */
		/* BEGINNING OF INLINE: 148 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENCOUNT):
				{
					t_pos ZI191;
					t_pos ZI192;
					t_unsigned ZI193;

					/* BEGINNING OF EXTRACT: OPENCOUNT */
					{
#line 407 "src/libre/parser.act"

		ZI191 = lex_state->lx.start;
		ZI192   = lex_state->lx.end;
	
#line 2237 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: OPENCOUNT */
					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_COUNT):
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 547 "src/libre/parser.act"

		unsigned long u;
		char *e;

		u = strtoul(lex_state->buf.a, &e, 10);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UINT_MAX) {
			err->e = RE_ECOUNTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL4;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL4;
		}

		ZI193 = (unsigned int) u;
	
#line 2265 "src/libre/dialect/native/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL4;
					}
					ADVANCE_LEXER;
					p_194 (fsm, flags, lex_state, act_state, err, &ZIx, ZIy, &ZI191, &ZI193);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_OPT):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-1 */
					{
#line 907 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL4;
		}
	
#line 2291 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: count-0-or-1 */
				}
				break;
			case (TOK_PLUS):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-1-or-many */
					{
#line 940 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL4;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL4;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL4;
			}

			(*ZIy) = z;
		}
	
#line 2324 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: count-1-or-many */
				}
				break;
			case (TOK_STAR):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 913 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL4;
		}

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL4;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL4;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL4;
			}

			(*ZIy) = z;
		}
	
#line 2361 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: count-1 */
					{
#line 963 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 2375 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: count-1 */
				}
				break;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-count */
				{
#line 978 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
	
#line 2392 "src/libre/dialect/native/parser.c"
				}
				/* END OF ACTION: err-expected-count */
			}
		ZL3:;
		}
		/* END OF INLINE: 148 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_literal(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 134 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_pos ZI139;
					t_pos ZI140;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 508 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZI139 = lex_state->lx.start;
		ZI140   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 2435 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					/* BEGINNING OF EXTRACT: ESC */
					{
#line 436 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}
	
#line 2462 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					t_pos ZI137;
					t_pos ZI138;

					/* BEGINNING OF EXTRACT: HEX */
					{
#line 483 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI137 = lex_state->lx.start;
		ZI138   = lex_state->lx.end;

		errno = 0;

		u = strtoul(lex_state->buf.a + 2, &e, 16);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 2503 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					t_pos ZI135;
					t_pos ZI136;

					/* BEGINNING OF EXTRACT: OCT */
					{
#line 455 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI135 = lex_state->lx.start;
		ZI136   = lex_state->lx.end;

		errno = 0;

		u = strtoul(lex_state->buf.a + 1, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 2544 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 134 */
		/* BEGINNING OF ACTION: add-literal */
		{
#line 823 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!addedge_literal(fsm, flags, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 2568 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 1183 "src/libre/parser.act"


	static int
	lgetc(struct LX_STATE *lx)
	{
		struct lex_state *lex_state;

		assert(lx != NULL);
		assert(lx->opaque != NULL);

		lex_state = lx->opaque;

		assert(lex_state->f != NULL);

		return lex_state->f(lex_state->opaque);
	}

	static int
	parse(int (*f)(void *opaque), void *opaque,
		void (*entry)(struct fsm *, flags, lex_state, act_state, err),
		enum re_flags flags, int overlap,
		struct fsm *new, struct re_err *err)
	{
		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;
		struct re_err dummy;

		struct LX_STATE *lx;

		assert(f != NULL);
		assert(entry != NULL);

		if (err == NULL) {
			err = &dummy;
		}

		lex_state    = &lex_state_s;
		lex_state->p = lex_state->a;

		lx = &lex_state->lx;

		LX_INIT(lx);

		lx->lgetc  = lgetc;
		lx->opaque = lex_state;

		lex_state->f       = f;
		lex_state->opaque  = opaque;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		/* XXX: unneccessary since we're lexing from a string */
		/* (except for pushing "[" and "]" around ::group-$dialect) */
		lx->buf   = &lex_state->buf;
		lx->push  = CAT(LX_PREFIX, _dynpush);
		lx->pop   = CAT(LX_PREFIX, _dynpop);
		lx->clear = CAT(LX_PREFIX, _dynclear);
		lx->free  = CAT(LX_PREFIX, _dynfree);

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		act_state->overlap = overlap;

		err->e = RE_ESUCCESS;

		ADVANCE_LEXER;
		entry(new, flags, lex_state, act_state, err);

		lx->free(lx);

		if (err->e != RE_ESUCCESS) {
			/* TODO: free internals allocated during parsing (are there any?) */
			goto error;
		}

		return 0;

	error:

		/*
		 * Some errors describe multiple tokens; for these, the start and end
		 * positions belong to potentially different tokens, and therefore need
		 * to be stored statefully (in act_state). These are all from
		 * non-recursive productions by design, and so a stack isn't needed.
		 *
		 * Lexical errors describe a problem with a single token; for these,
		 * the start and end positions belong to that token.
		 *
		 * Syntax errors occur at the first point the order of tokens is known
		 * to be incorrect, rather than describing a span of bytes. For these,
		 * the start of the next token is most relevant.
		 */

		switch (err->e) {
		case RE_EOVERLAP:  err->start = act_state->groupstart; err->end = act_state->groupend; break;
		case RE_ENEGRANGE: err->start = act_state->rangestart; err->end = act_state->rangeend; break;
		case RE_ENEGCOUNT: err->start = act_state->countstart; err->end = act_state->countend; break;

		case RE_EHEXRANGE:
		case RE_EOCTRANGE:
		case RE_ECOUNTRANGE:
			/*
			 * Lexical errors: These are always generated for the current token,
			 * so lx->start/end here is correct because ADVANCE_LEXER has
			 * not been called.
			 */
			mark(&err->start, &lx->start);
			mark(&err->end,   &lx->end);
			break;

		default:
			/*
			 * Due to LL(1) lookahead, lx->start/end is the next token.
			 * This is approximately correct as the position of an error,
			 * but to be exactly correct, we store the pos for the previous token.
			 * This is more visible when whitespace exists.
			 */
			err->start = act_state->synstart;
			err->end   = act_state->synstart; /* single point */
			break;
		}

		return -1;
	}

	struct fsm *
	DIALECT_COMP(int (*f)(void *opaque), void *opaque,
		const struct fsm_options *opt,
		enum re_flags flags, int overlap,
		struct re_err *err)
	{
		struct fsm *new;

		assert(f != NULL);

		new = fsm_new_blank(opt);
		if (new == NULL) {
			return NULL;
		}

		if (-1 == parse(f, opaque, DIALECT_ENTRY, flags, overlap, new, err)) {
			fsm_free(new);
			return NULL;
		}

		return new;
	}

#line 2733 "src/libre/dialect/native/parser.c"

/* END OF FILE */
