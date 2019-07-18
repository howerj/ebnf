/** @file       bnf.c
 *  @brief      An Extended Backus-Naur Form Parser generator for C (work in progress)
 *  @author     Richard James Howe.
 *  @copyright  Copyright 2019 Richard James Howe.
 *  @license    MIT
 *  @email      howe.r.j.89@gmail.com 
 *  See <https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_Form>
 *  @todo Add in other parts of the EBNF standard, as well as
 *        escape characters, character classes and white-space
 *  @todo Allow parsing of binary files
 *  
 *  The hard coded parser and lexer should be disposed of once the VM works,
 *  a lot of code will be duplicated until this happens.  **/
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdarg.h>
#include <errno.h>

#define TOKEN_LEN	(256)

enum { 
	LEXER_ERROR,
	ID            =  'I',
	TERMINAL      =  'T',
	LSB           =  '[',  RSB   =  ']',
	LBR           =  '{',  RBR   =  '}',
	LPAR          =  '(',  RPAR  =  ')',
	LBB           =  '<',  RBB   =  '>',
	EQ            =  '=',
	PIPE          =  '|',
	PERIOD        =  '.',
	COMMA         =  ',',
	SEMI          =  ';',
	END           =  EOF
};

typedef struct parser_t {
	FILE *in;
	unsigned line, unget, idx, debug, jmpbuf_set;
	int token_id;
	char token[TOKEN_LEN];
	jmp_buf jb;
} parser_t;

/******************************** util ****************************************/

static char *pstrdup(const char *s) {
	assert(s);
	char *str;
	if (!(str = malloc(strlen(s) + 1)))
		return NULL;
	strcpy(str, s);
	return str;
}

static void pdepth(unsigned depth) {
	while (depth--)
		putchar('\t');
}

static void ptoken(parser_t *ps, FILE *out) {
	assert(ps);
	assert(out);
	int token = ps->token_id;
	fprintf(out, "%s ", ps->unget ? "unget-token" : "token");
	if (token == ID)
		fprintf(out, "<ID %s>", ps->token);
	else if (token == TERMINAL)
		fprintf(out, "<TERMINAL \"%s\">", ps->token);
	else if (token == END)
		fprintf(out, "<EOF>");
	else
		fputc(token, out);
	fputc('\n', out);
}

#define failed(PS, MSG) _failed((PS), (MSG))
static void *_failed(parser_t *ps, char *msg) {
	assert(ps);
	assert(msg);
	fprintf(stderr, "syntax error on line %u\nexpected\n\t%s\ngot\n\t", ps->line, msg);
	ptoken(ps, stderr);
	if (ps->jmpbuf_set)
		longjmp(ps->jb, 1);
	abort(); 
	return NULL; 
}

static void debug(parser_t *ps, char *msg, unsigned depth) { 
	assert(ps);
	assert(msg);
	if (ps->debug) {
		pdepth(depth);
		printf("%s\n", msg);
	}
}

/******************************** lexer ***************************************/

static void untoken(parser_t *ps, int token) {
	if (ps->unget)
		failed(ps, "token already pushed back");
	ps->unget = 1;
	ps->token_id = token;
}

static int digit(char c) { /* digit = "0" | ... | "9" */
	return isdigit(c); 
}

static int letter(char c) { /* digit = "A" | ... | "Z" | "a" ... "z" */
	return isalpha(c); 
}

static int symbol(char c) { /* symbol = ... */
	return !!strchr("[]{}()<>'\"=|.,;", c); 
}

static int character(char c) {	/* character = letter | digit | symbol | "_" ; */
	return digit(c) || letter(c) || symbol(c) || c == '_';
}

static int identifier(parser_t *ps, char c) { /* identifier = letter , { letter | digit | "_" } ; */
	assert(ps);
	ps->idx = 0;
	ps->token[0] = c;
	for (;;) {
		c = getc(ps->in);
		assert(ps->idx < TOKEN_LEN - 1);
		if (letter(c) || digit(c) || c == '_') {
			ps->token[++(ps->idx)] = c;
		} else {
			ungetc(c, ps->in);
			ps->token[++(ps->idx)] = 0;
			return ID;
		}
	}
}

static int terminal(parser_t *ps, char expect) { 
	assert(ps);
	ps->idx = 0;
	for (;;) {
		int c = getc(ps->in);
		assert(ps->idx < TOKEN_LEN - 1);
		if (c == expect) {
			ps->token[ps->idx] = 0;
			return TERMINAL;
		} else if (character(c)) { 
			ps->token[(ps->idx)++] = c;
		} else {
			failed(ps, "terminal = \"'\" character , { character } , \"'\""
				   " | '\"' , character , { character } , '\"' ;");
		}
	}
}

static int comment(parser_t *ps) {
	assert(ps);
	int c = getc(ps->in);
	if (c == '*') {
		for (;;) {
			c = getc(ps->in);
			if (c == '*') {
				c = getc(ps->in);
				if (c == ')')
					break;
				if (c == EOF)
					goto comment_error;
					   
			   }
			   if (c == EOF)
				goto comment_error;
		}
		return 1;
comment_error:
		failed(ps, "(* , any-character , *)");
	} else {
		ungetc(c, ps->in);
	}
	return 0;
}

static int lexer(parser_t *ps, unsigned depth) {
	assert(ps);
	int c = 0;
	if (ps->unget) {
		ps->unget = 0;
		if (ps->debug) {
			pdepth(depth);
			ptoken(ps, stdout);
		}
		return ps->token_id;
	}
again:	c = getc(ps->in);
	switch (c) {
	case '\n': ps->line++;
	case ' ':
	case '\t': goto again;	
	case '[':  ps->token_id = LSB; break;
	case ']':  ps->token_id = RSB; break;
	case '{':  ps->token_id = LBR; break;
	case '}':  ps->token_id = RBR; break;
	case '(':  if (comment(ps))
			   goto again;
		   ps->token_id = LPAR;	
		   break;
	case ')':  ps->token_id = RPAR; break;
	case '<':  ps->token_id = LBB;  break;
	case '>':  ps->token_id = RBB;  break;
	case '=':  ps->token_id = EQ;   break;
	case '|':  ps->token_id = PIPE;	break;
	case '.':  ps->token_id = PERIOD; break;
	case ',':  ps->token_id = COMMA; break;
	case ';':  ps->token_id = SEMI; break;
	case EOF:  ps->token_id = END;  break;
	case '\'': 
	case '"':  ps->token_id = terminal(ps, c); break;
	default:
		if (letter(c)) {
			ps->token_id = identifier(ps, c);
			break;
		}
		failed(ps, "identifier = letter , { letter | digit | '_' } ;");
	}
	if (ps->debug) {
		pdepth(depth);
		ptoken(ps, stdout);
	}
	return ps->token_id;
}

/******************************** parser **************************************/

enum { 
	FAIL,       GRAMMAR,     RULE,      LHS,  RHS,
	ALTERNATE,  TERM,        GROUPING,
	OPTIONAL,   REPETITION,  LAST
};

static char *names[] = {
	[FAIL]      =  "fail",      [GRAMMAR]     =  "grammar",    [RULE]  =  "rule",  [LHS]       =  "lhs",
	[RHS]       =  "rhs",       [ALTERNATE]   =  "alternate",  [TERM]  =  "term",  [GROUPING]  =  "grouping",
	[OPTIONAL]  =  "optional",  [REPETITION]  =  "repetition"
};

typedef struct node {
	char *sym;
	size_t len, allocated;
	int token_id, has_alternate;
	struct node *n[];
} node;

static node *mk(int token_id, char *sym, size_t len, ...) {
	node *ret = calloc(1, sizeof(*ret) + (len * sizeof(ret)));
	va_list ap;
	if (!ret) {
		perror("calloc"); 
		abort();
	}
	ret->token_id = token_id; 
	ret->sym = sym ? pstrdup(sym) : NULL;
	ret->len = len;
	ret->allocated = len;
	va_start(ap, len);
	for (size_t i = 0; i < len; i++)
		ret->n[i] = va_arg(ap, node*);
	va_end(ap);
	return ret;
}

static node *grow(node *n, size_t new_len) {
	assert(n);
	node *ret = n;
	if (new_len > n->allocated) {
		assert(new_len * 2 > n->allocated);
		if (!(ret = realloc(n, sizeof(*ret) + (new_len * 2 * sizeof(ret))))) {
			perror("realloc"); 
			abort();
		}
		ret->allocated = new_len * 2;
	} 
	return ret;
}

static void unmk(node *n) {
	if (!n)
		return;
	free(n->sym);
	for (size_t i = 0; i < n->len; i++)
		unmk(n->n[i]);
	free(n);
}

static void print(node *n, unsigned depth) {
	if (!n)
		return;
	pdepth(depth);
	if (names[n->token_id])
		printf("%s %s\n", names[n->token_id], n->sym ? n->sym : "");
	else if (n->token_id != EOF)
		printf("%c %s\n", n->token_id, n->sym ? n->sym : "");
	else /*special case*/
		printf("EOF\n");
	for (size_t i = 0; i < n->len; i++)
		print(n->n[i], depth+1);
}

static int accept(parser_t *ps, int token_id, unsigned depth) {
	if (ps->token_id == token_id) {
		lexer(ps, depth);
		return 1;
	}
	return 0;
}

static node *lhs(parser_t *ps, unsigned depth) {
	assert(ps);
	/* lhs = identifier */
	debug(ps, "lhs", depth);
	if (ps->token_id == ID)
		return mk(LHS, ps->token, 0); 
	return failed(ps, "lhs = identifier ;");
}

static node *rhs(parser_t *ps, unsigned depth);

static node *grouping(parser_t *ps, unsigned depth) {
	assert(ps);
	if (!accept(ps, LPAR, depth))
		return NULL;
	debug(ps, "grouping", depth);
	node *n = mk(GROUPING, NULL, 1, NULL);	
	if ((n->n[0] = rhs(ps, depth+1)))
		lexer(ps, depth);
	else
		goto fail;
	if (ps->token_id == RPAR)
		return n;
fail:	return failed(ps, "grouping = '(' , rhs , ')' ;");
}

static node *optional(parser_t *ps, unsigned depth) { 
	assert(ps);
	if (!accept(ps, LSB, depth))
		return NULL;
	debug(ps, "optional", depth);
	node *n = mk(OPTIONAL, NULL, 1, NULL);	
	if ((n->n[0] = rhs(ps, depth+1)))
		lexer(ps, depth);
	else
		goto fail;
	if (ps->token_id == RSB)
		return n;
fail:	return failed(ps, "optional = '[' , rhs , ']' ;");
}

static node *repetition(parser_t *ps, unsigned depth) {
	assert(ps);
	if (!accept(ps, LBR, depth))
		return NULL;
	debug(ps, "repetition", depth);
	node *n = mk(REPETITION, NULL, 1, NULL);	
	if ((n->n[0] = rhs(ps, depth+1)))
		lexer(ps, depth);
	else
		goto fail;
	if (ps->token_id == RBR)
		return n;
fail:	return failed(ps, "repetition = '{' , rhs , '}' ;");
}

static node *term(parser_t *ps, unsigned depth) {
	assert(ps);
	node *n = mk(TERM, NULL, 1, NULL);	
	debug(ps, "term", depth);
	if (ps->token_id == ID || ps->token_id == TERMINAL) {
		n->n[0] = mk(ps->token_id, ps->token, 0);
		return n;
	}
	if ((n->n[0] = grouping(ps, depth+1)))
		return n;
	if ((n->n[0] = optional(ps, depth+1)))
		return n;
	if ((n->n[0] = repetition(ps, depth+1)))
		return n;
	return failed(ps, "term = identifier | grouping | optional | repetition ;");
}

static node *rhs(parser_t *ps, unsigned depth) {
	assert(ps);
	node *t = NULL, *n = mk(RHS, NULL, 1, NULL);
	for (size_t i = 0;;) {
		if ((t = term(ps, depth+1))) {
			n = grow(n, i+1);
			n->n[i] = t;
			n->len = i + 1;
			lexer(ps, depth);
		} else {
			goto fail;
		}
		if (ps->token_id == PIPE || ps->token_id == COMMA) {
			if (ps->token_id == PIPE) {
				n = grow(n, i + 2);
				n->n[i+1] = mk(ALTERNATE, NULL, 0);
				n->len = i + 2;
				n->has_alternate = 1;
				i++;
			}
			/* COMMA is the default operation */
			i++;
			lexer(ps, depth);
			continue;
		}
		untoken(ps, ps->token_id);
		break;
	}
	return n;
fail:	return failed(ps, "rhs = term | term , '|' rhs | term , ',' rhs ;");
}

static node *rule(parser_t *ps, unsigned depth) {
	assert(ps);
	node *n = mk(RULE, NULL, 2, NULL, NULL);
	debug(ps, "rule", depth);
	if ((n->n[0] = lhs(ps, depth+1)))
		lexer(ps, depth);
	else
		goto fail;
	if (!accept(ps, EQ, depth))
		goto fail;
	if ((n->n[1] = rhs(ps, depth+1)))
		lexer(ps, depth);
	else
		goto fail;
	if (ps->token_id == SEMI)
		return n;
fail:	return failed(ps, "rule = lhs , '=' , rhs , ';' ;");
}

static node *grammar(parser_t *ps, unsigned depth) {
	assert(ps);
	node *t = NULL, *n = mk(GRAMMAR, 0, 1, NULL);
	for (size_t i = 0;;i++) {
		lexer(ps, depth);
		if (ps->token_id == END)
			return n;
		if ((t = rule(ps, depth + 1))) {
			n = grow(n, i+1);
			n->len = i + 1;
			n->n[i] = t;
		} else {
			goto fail;
		}
	}
fail:	return failed(ps, "grammar = EOF | { rule } ; EOF");
}

node *parse_ebnf(parser_t *ps, FILE *in, int debug_on) {
	assert(ps && in);
	memset(ps, 0, sizeof(*ps));
	ps->in = in;
	ps->debug = debug_on;
	ps->line = 1;
	if (setjmp(ps->jb)) {
		ps->jmpbuf_set = 0;
		return NULL;
	}
	ps->jmpbuf_set = 1;
	return grammar(ps, 0);
}

/****************************** C code generation *****************************/

/*************************** VM code generation *******************************/

enum { NOP, PUSH, POP, IF, ACCEPT, EXPECT, CALL, RETURN, UNTOKEN, DONE, JZ, JMP };

static void print_error_number(const char *msg, unsigned line) {
	assert(msg);
	fprintf(stderr, "%s %s %d\n", msg, strerror(errno), line);
	abort();
}

#define perrno(MSG) print_error_number((MSG), __LINE__)
#define MAX_CODE_LENGTH (4096u)

typedef struct {
	int instruction;
	int token; /**< token to look for*/
	char *id;  /**< id or terminal to match for ID or TERMINAL*/
} operation;

typedef struct {
	int cycles, until;
	size_t pc, here, stkp;
	size_t stack[64];
	unsigned char code[MAX_CODE_LENGTH];
} vm_t;

/* From: Compiler Construction, by Niklaus Wirth

	:      K                      :    Pr(K)                               :
	: --------------------------- : -------------------------------------- :
	:     "x"                     : IF sym = "x" THEN next ELSE error END  :
	:    (exp)                    : Pr(exp)                                :
	:    [exp]                    : IF sym IN first(exp) THEN Pr(exp) END  :
	:    {exp}                    : WHILE sym IN first(exp) DO Pr(exp) END :
	: fac0 fac0 ... facn          : Pr(fac0); Pr(fac1); ... Pr(facn)       :
	: term0 | term1 | ... | termn : CASE sym OF                            :
	:                             :   first(term0): Pr(term0);             :
	:                             :   first(term1): Pr(term1);             :
	:                             :   ...                                  :
	:                             :   first(termn): Pr(termn);             :
	:                             : END                                    :

*/

static int code(vm_t *cs, node *n) {
	assert(cs);
	assert(n);
	switch (n->token_id) {
	case TERMINAL: /*EXPECT ID*/
		break;
	case GRAMMAR: /*Set up
			Generate code for RULEs
			Patch up CALLs*/
		break;
	case RULE: /* LHS : RHS */
		break;
	case LHS: /*Add label to list that can be CALLED*/
		break;
	case RHS: /*Generate code for rule
			CALL IDs and EXPECT
			EXPECT TERMINAL
			EXPECT TERM
			Handle ALTERNATE
			*/
		break;
	case ALTERNATE: /* ACCEPT first
			   	else JMP End
			   EXPECT rest*/
		break;
	case TERM: /*pass through to OPTIONAL, GROUPING or REPETITION*/
		break;
	case OPTIONAL: /*ACCEPT {RHS}*/
		break;
	case GROUPING: /*??*/
		break;
	case REPETITION: /*WHILE{ EXPECT RHS }*/
		break;
	case FAIL: 
		break;
	case LAST:
		break;
	default:
		fprintf(stderr, "code generation error\n");
		return -1;
	}
	return 0;
}

vm_t *generate_code(node *n) {
	assert(n);
	vm_t *cs = calloc(1, sizeof(*cs));
	if (!cs) 
		perrno("calloc");
	if (code(cs, n) < 0) {
		free(cs);
		return NULL;
	}
	return cs;
}

void destroy_code(vm_t *cs) { 
	free(cs); 
}

/******************************* EBNF VM **************************************/

/*The VM is passed a vm_t and a special VM lexer*/

int vm(vm_t *v, node *n) {
	assert(v);
	assert(n);
	for (; !(v->until) || v->cycles < v->until ; v->cycles++) {
		const int inst = v->code[v->pc++];
		switch (inst) {
		case NOP: break;
		case PUSH: 
			break;
		case POP: 
			break;
		case IF: 
			break;
		case ACCEPT: 
			break;
		case EXPECT: 
			break;
		case CALL: 
			break;
		case RETURN: 
			break;
		case UNTOKEN: 
			break;
		case DONE:
			if (v->pc > v->here)
				return -1;
			return v->code[v->pc];
		case JZ: 
			break;
		case JMP: { 
			if (v->pc > (v->here - 1))
				return -1;
			v->pc = (v->code[v->pc + 0] << 8) | (v->code[v->pc + 1] << 0);
			break;
		}
		default:
			return -1;
		}
	}
	return 0;
}

/******************************* driver ***************************************/

static parser_t ps;

char *astrcat(char *a, char *b) {
	assert(a);
	assert(b);
	size_t la = strlen(a), lb = strlen(b);
	char *r = calloc(la + lb + 1, 1);
	memcpy(r, a, la);
	memcpy(r + la, b, lb);
	return r;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s files\n", argv[0]);
		return 1;
	}

	assert(((int)TERMINAL > LAST));

	while (++argv, --argc) { 
		FILE *in = fopen(argv[0], "rb"), *headfile, *bodyfile;
		if (!in) {
			perror(argv[0]);
			return 1;
		}
		node *n = parse_ebnf(&ps, in, 1);
		print(n, 0);
		unmk(n);
		fclose(in);
	}
        return 0;
}

