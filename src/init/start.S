#if defined(__x86_64__)
    .global _start
    .type _start,@function
_start:
    xor %rbp, %rbp
    andq $-16, %rsp
    call nova_main
    mov %eax, %edi
    mov $60, %eax
    syscall
1:  hlt
    jmp 1b
#elif defined(__aarch64__)
    .global _start
    .type _start,%function
_start:
    mov x29, xzr
    mov x30, xzr
    bl nova_main
    mov x8, #93
    svc #0
1:  wfe
    b 1b
#else
#error unsupported architecture
#endif
