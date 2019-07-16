/* Author:  Richard James Howe
 * Email:   howe.r.j.89@gmail.com
 * License: MIT
 * Project: An EBNF compiler-compiler and virtual machine */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

typedef struct {
	union { unsigned u; int d; char *s; char c; } data;
	int type;
} ebnf_node_t;

typedef struct {
	void *(*malloc)(void *arena, size_t length);
	int (*free)(void *arena);
	void *(*realloc)(void *arena, size_t length);
} ebnf_allocator_t;

typedef struct {
	char error[512];
	ebnf_node_t *ast;
	uint8_t *m;
	int initialized, depth, cycles;
	int max_depth, max_cycles;
	size_t length;
	unsigned stk[64];
	unsigned sp;
} ebnf_t;

/* Instructions:
 * - Get Char
 * - Get Token
 * - Accept
 * - Expect
 * - Reject
 * - While
 * - Return
 * - If
 *
 *
 * Put Char? */

enum {
	OP_ACCEPT,
	OP_EXPECT,
	OP_WHILE,
	OP_IF,
	OP_UNGETC,
};

/* Take a lexer function? */
int ebnf_vm(ebnf_t *e, char *input, size_t length) {
	assert(e);
	assert(e->m);

	if (!(e->initialized)) {
	}

	const unsigned cycles = e->cycles;
	const unsigned core   = e->length;
	const unsigned forevs = !!cycles;
	uint8_t *m = e->m;

	for (unsigned pc = 0, count = 0; count < cycles || forevs; count++) {
		if (pc >= core) {
			snprintf(e->error, sizeof e->error, "pc out of range: %u", pc);
			return -1;
		}

		const int op = m[pc];
		switch (op) {
		default:
			snprintf(e->error, sizeof e->error, "invalid op %d at %u", op, pc);
			return -1;
		}
	}

	return 0;
}

int main(void) {
	return 0;
}

