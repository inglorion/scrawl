#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define WORD_SHIFT 2
typedef int32_t word_t;
typedef uint32_t uword_t;

/* Parser lookahead codes. */
#define C_ERROR -1
#define C_EOF -2
#define C_NONE -3

/* Error codes. */
#define E_UNKNOWN -1
#define E_NOTFOUND -2
#define E_PERM -3
#define E_EXIST -4
#define E_INVAL -5
#define E_NOSPC -6
#define E_IO -7
#define E_NFILE -8


/* Runtime. */

#define INITIAL_PROGSIZE 0x8000

typedef struct {
  unsigned char *ip;
  void *sp;
  void *locations[0x800];
  void *stack[0x80];
  unsigned char *program;
  char **argv;
  size_t progsize;
  word_t a;
  int argc;
  unsigned char imask;
} runtime;

#define OPS                                     \
  OP(exit)                                      \
  OP(write)                                     \
  OP(read)                                      \
  OP(getb)                                      \
  OP(getw)                                      \
  OP(setb)                                      \
  OP(setw)                                      \
  OP(call)                                      \
  OP(jmp)                                       \
  OP(ret)                                       \
  OP(beq)                                       \
  OP(bge)                                       \
  OP(blt)                                       \
  OP(bne)                                       \
  OP(add)                                       \
  OP(and)                                       \
  OP(asr)                                       \
  OP(bsr)                                       \
  OP(or)                                        \
  OP(shl)                                       \
  OP(sub)                                       \
  OP(xor)                                       \
  OP(open)                                      \
  OP(close)                                     \
  OP(flush)                                     \
  OP(locb)                                      \
  OP(locw)                                      \
  OP(argc)                                      \
  OP(argb)                                      \


static word_t load_word(void *addr) {
  unsigned char *p = addr;
  return ((word_t) p[0]) | ((word_t) p[1] << 8)
    | ((word_t) p[2] << 16) | ((word_t) p[3] << 24);
}

static word_t word_at_loc(const runtime *rt, word_t loc) {
  return load_word(rt->locations[loc]);
}

static unsigned char ipbyte(runtime *rt) {
  unsigned char c = *rt->ip;
  ++rt->ip;
  return c;
}

static word_t ipword(runtime *rt) {
  word_t x = load_word(rt->ip);
  rt->ip += sizeof(word_t);
  if (rt->imask & 1) {
    x = word_at_loc(rt, x);
  }
  rt->imask >>= 1;
  return x;
}

typedef void (*op_impl) (runtime *);

#define OP(X) void op_ ## X(runtime *);
OPS
#undef OP

op_impl opmap[] = {
#define OP(X) op_ ## X,
OPS
#undef OP
};

void run(runtime *rt) {
  unsigned char opcode;

  for (;;) {
    opcode = ipbyte(rt);
    opmap[opcode](rt);
  }
}

/* Operation Implementations. */

#define POP(X) do {                             \
  X = *((void**) rt->sp);                       \
  rt->sp += sizeof(void*);                      \
} while(0)

#define PUSH(X) do {                            \
  rt->sp -= sizeof(void*);                      \
  *((void**) rt->sp) = X;                       \
} while(0)

static word_t xlate_errno() {
  switch (errno) {
  case 0:      return 0;
  case EACCES: return E_PERM;
  case EEXIST: return E_EXIST;
  case EINVAL: return E_INVAL;
  case EIO:    return E_IO;
  case ENFILE: return E_NFILE;
  case ENOENT: return E_NOTFOUND;
  case ENOSPC: return E_NOSPC;
  default:     return E_UNKNOWN;
  }
}

static int xlate_oflags(word_t flags) {
  switch (flags) {
  case 1:  return O_WRONLY | O_CREAT | O_EXCL;
  case 2:  return O_WRONLY | O_TRUNC;
  case 3:  return O_WRONLY | O_CREAT | O_TRUNC;
  case 4:  return O_RDONLY;
  default: return 0;
  }
}

#define CALL(L) do {                            \
  PUSH(rt->ip);                                 \
  rt->ip = rt->locations[L];                    \
} while (0)

#define ERR(N) do {                               \
  *((word_t*) rt->locations[0xf]) = (word_t) (N); \
  CALL(0xe);                                      \
} while (0)  

#define CHECK0(X) if ((X) != 0) ERR(xlate_errno())

#define RET(X) { rt->a = X; return; } while (0)

void op_argb(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t argn = ipword(rt);
  word_t idx = ipword(rt);
  word_t i;

  if (argn >= rt->argc) ERR(E_INVAL);

  for (i = 0; i <= idx; ++i) if (rt->argv[argn][i] == 0) RET(-1);
  rt->a = rt->argv[argn][idx];
}

void op_argc(runtime *rt) {
  rt->imask = ipbyte(rt);
  rt->a = rt->argc;
}

void op_exit(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t x = ipword(rt);

  exit(x);
}

#define BINOP(NAME, SYM)                        \
  void op_ ## NAME(runtime *rt) {               \
    rt->imask = ipbyte(rt);                     \
    word_t x = ipword(rt);                      \
                                                \
    rt->a = rt->a SYM x;                        \
  }

BINOP(add, +)
BINOP(and, &)
BINOP(asr, >>)
BINOP(or, |)
BINOP(shl, <<)
BINOP(sub, -)
BINOP(xor, ^)

void op_bsr(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t x = ipword(rt);

  rt->a = (word_t) ((uword_t) rt->a >> x);
}

void op_close(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t fd = ipword(rt);

  CHECK0(close((int) fd) != 0);
}

void op_flush(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t fd = ipword(rt);

  CHECK0(fsync((int) fd));
}

void op_getb(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t base = ipword(rt);
  word_t off = ipword(rt);

  rt->a = ((unsigned char*) rt->locations[base])[off];
}

void op_getw(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t base = ipword(rt);
  word_t off = ipword(rt);

  rt->a = ((word_t*) rt->locations[base])[off];
}

void op_call(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t loc = ipword(rt);

  CALL(loc);
}

void op_jmp(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t loc = ipword(rt);

  rt->ip = rt->locations[loc];
}

#define BRANCH(COND, SYM)                          \
  void op_b ## COND(runtime *rt) {                 \
    rt->imask = ipbyte(rt);                        \
    word_t b = ipword(rt);                         \
    word_t loc = ipword(rt);                       \
                                                   \
    if (rt->a SYM b) rt->ip = rt->locations[loc];  \
  }

BRANCH(eq, ==)
BRANCH(ge, >=)
BRANCH(lt, <)
BRANCH(ne, !=)

void op_locb(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t dst = ipword(rt);
  word_t base = ipword(rt);
  word_t offset = ipword(rt);
  
  rt->locations[dst] = (char*) rt->locations[base] + offset;
}

void op_locw(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t dst = ipword(rt);
  word_t base = ipword(rt);
  word_t offset = ipword(rt);
  
  rt->locations[dst] = (word_t*) rt->locations[base] + offset;
}

void op_open(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t loc = ipword(rt);
  word_t len = ipword(rt);
  word_t flags = ipword(rt);
  int fd, oflags = xlate_oflags(flags);
  char buf[256];

  memcpy(buf, rt->locations[loc], len);
  fd = open(buf, oflags);
  if (fd < 0) ERR(xlate_errno());
  rt->a = fd;
}

void op_read(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t fd = ipword(rt);
  word_t loc = ipword(rt);
  word_t n = ipword(rt);
  int r;

  r = read(fd, rt->locations[loc], n);
  if (r < 0) ERR(xlate_errno());
  rt->a = r;
}

void op_ret(runtime *rt) {
  rt->imask = ipbyte(rt);
  POP(rt->ip);
}

void op_setb(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t base = ipword(rt);
  word_t off = ipword(rt);

  ((unsigned char*) rt->locations[base])[off] = (unsigned char) rt->a;
}

void op_setw(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t base = ipword(rt);
  word_t off = ipword(rt);

  ((word_t*) rt->locations[base])[off] = rt->a;
}

void op_write(runtime *rt) {
  rt->imask = ipbyte(rt);
  word_t fd = ipword(rt);
  word_t loc = ipword(rt);
  word_t n = ipword(rt);

  if (write(fd, rt->locations[loc], n) < 0) ERR(xlate_errno());
}

/* Parser. */

typedef struct {
  FILE *input;
  int lookahead;
} parser;

int lookahead(parser *parser) {
  if (parser->lookahead == C_NONE) {
    parser->lookahead = fgetc(parser->input);
    if (parser->lookahead == EOF) {
      if (ferror(parser->input)) parser->lookahead = C_ERROR;
      else parser->lookahead = C_EOF;
    }
  }
  return parser->lookahead;
}

void consume(parser *parser) {
  parser->lookahead = C_NONE;
}

void skip_whitespace(parser *parser) {
  int c;
  for (;;) {
    c = lookahead(parser);
    if (c != '\r' && c != ' ') break;
    consume(parser);
  }
}

int sym_char(int c) {
  return c >= 'a' && c <= 'z';
}

int seeing_num(parser *parser) {
  return lookahead(parser) == '$';
}

unsigned parse_num(parser *parser) {
  unsigned val = 0;
  int c;

  consume(parser);              /* consume leading '$' */
  for (;;) {
    c = lookahead(parser);
    if (c >= '0' && c <= '9') val = (val << 4) | (c - '0');
    else if (c >= 'a' && c <= 'f') val = (val << 4) | (c + 10 - 'a');
    else break;
    consume(parser);
  }

  return val;
}

size_t parse_sym(parser *parser, char *buf, size_t size) {
  int c;
  size_t i;

  for (i = 0; i < size; ++i) {
    c = lookahead(parser);
    if (!sym_char(c)) break;
    buf[i] = c;
    consume(parser);
  }

  return i;
}

word_t require_num(parser *parser, const char *prefix) {
  skip_whitespace(parser);
  if (!seeing_num(parser)) {
    fprintf(stderr, "%s: need number, got %c\n",
            prefix, lookahead(parser));
    exit(1);
  }
  return parse_num(parser);
}

/* Compiler. */

char *commands[] = {
#define OP(X) #X,
OPS
#undef OP
  NULL,
};

int sym_to_opcode(const char *buf, size_t size) {
  size_t i;

  for (i = 0; commands[i]; ++i) {
    if (strncmp(buf, commands[i], size) == 0) return i;
  }

  return -1;
}

#define emit_byte(X)                                     \
  if (rt->ip < rt->program + rt->progsize) {             \
    *rt->ip = (unsigned char) (X); ++rt->ip;             \
  }

#define emit_word(X) {                                    \
    word_t emit_word_x = X;                               \
    emit_byte(emit_word_x & 0xff);                        \
    emit_byte((emit_word_x >> 8) & 0xff);                 \
    emit_byte((emit_word_x >> 16) & 0xff);                \
    emit_byte((emit_word_x >> 24) & 0xff);                \
  }

static int compile_bytes(parser *parser, runtime *rt) {
  size_t x;
  for (;;) {
    skip_whitespace(parser);
    if (!seeing_num(parser)) break;
    x = parse_num(parser);
    emit_byte(x);
  }
  return 0;
}

static int compile_resb(parser *parser, runtime *rt) {
  word_t n;

  for (n = require_num(parser, "resb"); n > 0; --n) emit_byte(0xff);
  return 0;
}

static int compile_resw(parser *parser, runtime *rt) {
  word_t n;

  for (n = require_num(parser, "resw"); n > 0; --n) emit_word(0xffffffff);
  return 0;
}

static int compile_words(parser *parser, runtime *rt) {
  for (;;) {
    skip_whitespace(parser);
    if (!seeing_num(parser)) break;
    emit_word(parse_num(parser));
  }
  return 0;
}

int compile(parser *parser, runtime *rt) {
  char buf[256];
  int c, op, r;
  size_t n;
  unsigned char *indir_pos = NULL;
  unsigned indir = 0, indir_mask = 1;

  rt->progsize = INITIAL_PROGSIZE;
  rt->program = malloc(INITIAL_PROGSIZE);
  rt->ip = rt->program;
  rt->sp = rt->stack + 256;

  for (;;) {
    skip_whitespace(parser);
    /* If a number occurs as the first token on a line, it's
     * a location number. */
    if (seeing_num(parser)) {
      n = parse_num(parser);
      rt->locations[n] = rt->ip;
    }

    for (;;) {
      skip_whitespace(parser);
      c = lookahead(parser);
      if (sym_char(c)) {
        n = parse_sym(parser, buf, sizeof(buf));
        if (strncmp(buf, "byte", 4) == 0) {
          r = compile_bytes(parser, rt);
          if (r != 0) return r;
          continue;
        }
        if (strncmp(buf, "resb", 4) == 0) {
          r = compile_resb(parser, rt);
          if (r != 0) return r;
          continue;
        }
        if (strncmp(buf, "resw", 4) == 0) {
          r = compile_resw(parser, rt);
          if (r != 0) return r;
          continue;
        }
        if (strncmp(buf, "word", 4) == 0) {
          r = compile_words(parser, rt);
          if (r != 0) return r;
          continue;
        }
        op = sym_to_opcode(buf, n);
        if (op < 0) {
          fprintf(stderr, "unknown command: ");
          fwrite(buf, 1, n, stderr);
          fputc('\n', stderr);
          return -1;
        }
        emit_byte(op);
        indir = 0;
        indir_mask = 1;
        indir_pos = rt->ip;
        ++rt->ip;
      } else if (seeing_num(parser)) {
        emit_word(parse_num(parser));
        indir_mask <<= 1;
      } else if (c == '\n') {
        break;
      } else if (c == ':') {
        consume(parser);
        if (indir_pos != NULL) *indir_pos = indir;
        indir_pos = NULL;
      } else if (c == '@') {
        consume(parser);
        indir |= indir_mask;
      } else if (c == C_EOF || c == C_ERROR) {
        break;
      } else {
        fprintf(stderr, "unexepected character: %c ($%02x)\n", c, c);
        return -1;
      }
    }

    if (indir_pos != NULL) *indir_pos = indir;
    indir_pos = NULL;

    /* Only if we saw a newline should we continue here. */
    if (lookahead(parser) != '\n') break;
    consume(parser);
  }

  emit_byte(0);
  rt->ip = rt->program;

  return 0;
}

#undef emit_byte

int main(int argc, char *argv[]) {
  parser parser_, *parser = &parser_;
  runtime rt_, *rt = &rt_;
  int i;

  rt->argv = &argv[1];
  rt->argc = argc - 1;

  parser->input = fopen(argv[1], "r");
  if (!parser->input) {
    perror(argv[1]);
    return 2;
  }
  parser->lookahead = C_NONE;

  i = compile(parser, rt);
  if (i != 0) return 1;
  run(rt);

  return 0;
}
