#if 0
exec cc -Wall -Wextra $@ -o $(dirname $0)/jrp $0
#endif

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <unistd.h>

typedef unsigned long long op;
typedef long long value;
typedef unsigned long long uvalue;
typedef value *(*fptr)(value *);

static void rt_print_ascii(value val) {
    printf("%c\n", (char)val);
}
static void rt_print_bin(value val) {
    static char buf[sizeof(value) * 8 + 1];
    uvalue uval = val;
    int i = sizeof(buf);
    do {
        buf[--i] = '0' + (uval & 1);
    } while (uval >>= 1);
    printf("%s\n", &buf[i]);
}
static void rt_print_oct(value val) {
    printf("%llo\n", val);
}
static void rt_print_dec(value val) {
    printf("%lld\n", val);
}
static void rt_print_hex(value val) {
    printf("%llx\n", val);
}

enum {
    HOP_NOP  = 0x90666666, // nop
    HOP_DROP = 0x9066665f, // pop rdi
    HOP_DUP  = 0x90666657, // push rdi
    HOP_SWAP = 0x244c8748, // xchg rdi, [rsp]
    HOP_NEG  = 0x90dff748, // neg rdi
    HOP_ADD  = 0xc7014858, // pop rax; add rdi, rax
    HOP_QUO  = 0x90c78948, // mov rdi, rax
    HOP_REM  = 0x90d78948, // mov rdi, rdx
    HOP_NOT  = 0x90d7f748, // not rdi
    HOP_AND  = 0xc7214858, // pop rax; and rdi, rax
    HOP_OR   = 0xc7904858, // pop rax; or rdi, rax
    HOP_XOR  = 0xc7314858, // pop rax; xor rdi, rax
};

enum {
    FOP_PROL = 0x5ffc8948e5894855, // push rbp; mov rbp, rsp; mov rsp, rdi; pop rdi
    FOP_EPIL = 0x5dec8948e0894857, // push rdi; mov rax, rsp; mov rsp, rbp; pop rbp
    FOP_RET  = 0x90666690666666c3, // ret
    FOP_CRT  = 0xb848906666e58748, // xchg rsp, rbp; mov rax, strict qword 0
    FOP_CALL = 0x90665fe58748d0ff, // call rax; xchg rsp, rbp; pop rdi
    FOP_PUSH = 0xbf48909066666657, // push rdi; mov rdi, strict qword 0
    FOP_SUB  = 0x9066665f243c2948, // sub [rsp], rdi; pop rdi
    FOP_MUL  = 0x906666f8af0f4858, // pop rax; imul rdi, rax
    FOP_DIV  = 0x9066fff748994858, // pop rax; cqo; idiv rdi
    FOP_SHL  = 0x5f2424d348f98948, // mov rcx, rdi; shl qword [rsp], cl; pop rdi
    FOP_SHR  = 0x5f242cd348f98948, // mov rcx, rdi; shr qword [rsp], cl; pop rdi
};

static int page;

static int radix = 10;

static value *stack;

static struct {
    op *base;
    op *ptr;
    op hop;
} code;

static void jit_hop(op hop) {
    if (code.hop) {
        *code.ptr++ = hop << 32 | code.hop;
        code.hop = 0;
    } else {
        code.hop = hop;
    }
}

static void jit_fop(op fop) {
    if (code.hop) jit_hop(HOP_NOP);
    *code.ptr++ = fop;
}

static void jit(char *src) {
    int error;

    code.ptr = code.base;
    jit_fop(FOP_PROL);

    jit_fop(FOP_PUSH);
    jit_fop(7);
    jit_fop(FOP_PUSH);
    jit_fop(10);
    jit_fop(FOP_PUSH);
    jit_fop(9);
    jit_fop(FOP_MUL);
    jit_hop(HOP_ADD);
    jit_hop(HOP_DUP);
    jit_hop(HOP_DUP);
    jit_hop(HOP_DUP);
    jit_hop(HOP_DUP);
    jit_fop(FOP_CRT);
    jit_fop((op)rt_print_ascii);
    jit_fop(FOP_CALL);
    jit_fop(FOP_CRT);
    jit_fop((op)rt_print_bin);
    jit_fop(FOP_CALL);
    jit_fop(FOP_CRT);
    jit_fop((op)rt_print_oct);
    jit_fop(FOP_CALL);
    jit_fop(FOP_CRT);
    jit_fop((op)rt_print_dec);
    jit_fop(FOP_CALL);
    jit_fop(FOP_CRT);
    jit_fop((op)rt_print_hex);
    jit_fop(FOP_CALL);

    jit_fop(FOP_EPIL);
    jit_fop(FOP_RET);

    error = mprotect(code.base, page, PROT_READ | PROT_EXEC);
    if (error) err(EX_OSERR, "mprotect");

    stack = ((fptr)code.base)(stack);

    error = mprotect(code.base, page, PROT_READ | PROT_WRITE);
    if (error) err(EX_OSERR, "mprotect");
}

int main() {
    page = getpagesize();

    code.base = mmap(0, page, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    if (code.base == MAP_FAILED) err(EX_OSERR, "mmap");

    stack = mmap(0, page, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    if (stack == MAP_FAILED) err(EX_OSERR, "mmap");
    stack += page / sizeof(value) - 1;

    jit("7 10 9 * + :::: , b. o. d. h.");

    return EX_OK;
}
