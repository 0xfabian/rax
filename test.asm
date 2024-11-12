global main
extern printf
extern puts

length:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
        mov     qword [rbp-8], rdi
        cmp     qword [rbp-8], 9
        ja      .L2
        mov     eax, 1
        jmp     .L3
.L2:
        mov     rax, qword [rbp-8]

        db      0x48, 0xba, 0xcd, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc

        mul     rdx
        mov     rax, rdx
        shr     rax, 3
        mov     rdi, rax
        call    length
        add     rax, 1
.L3:
        leave
        ret
npow:
        push    rbp
        mov     rbp, rsp
        mov     qword [rbp-24], rdi
        mov     qword [rbp-32], rsi
        mov     qword [rbp-8], 1
        mov     qword [rbp-16], 0
        jmp     .L5
.L6:
        mov     rax, qword [rbp-8]
        imul    rax, qword [rbp-24]
        mov     qword [rbp-8], rax
        add     qword [rbp-16], 1
.L5:
        mov     rax, qword [rbp-16]
        cmp     rax, qword [rbp-32]
        jb      .L6
        mov     rax, qword [rbp-8]
        pop     rbp
        ret
find:
        push    rbp
        mov     rbp, rsp
        push    rbx
        sub     rsp, 40
        mov     qword [rbp-40], rdi
        mov     qword [rbp-48], rsi
        mov     qword [rbp-24], 1
        jmp     .L9
.L12:
        mov     rcx, qword [rbp-24]

        db      0x48, 0xba, 0xcd, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc

        mov     rax, rcx
        mul     rdx
        mov     rbx, rdx
        shr     rbx, 3
        mov     rax, rbx
        shl     rax, 2
        add     rax, rbx
        add     rax, rax
        sub     rcx, rax
        mov     rbx, rcx
        mov     rax, qword [rbp-24]
        mov     rdi, rax
        call    length
        sub     rax, 1
        mov     rsi, rax
        mov     edi, 10
        call    npow
        mov     rcx, rbx
        imul    rcx, rax
        mov     rax, qword [rbp-24]

        db      0x48, 0xba, 0xcd, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc

        mul     rdx
        mov     rax, rdx
        shr     rax, 3
        add     rax, rcx
        mov     qword [rbp-32], rax
        mov     rax, qword [rbp-24]
        add     rax, rax
        cmp     qword [rbp-32], rax
        jne     .L10
        mov     rax, qword [rbp-48]
        mov     rdx, qword [rbp-24]
        mov     qword [rax], rdx
        mov     eax, 1
        jmp     .L11
.L10:
        add     qword [rbp-24], 1
.L9:
        mov     rax, qword [rbp-24]
        cmp     rax, qword [rbp-40]
        jb      .L12
        mov     eax, 0
.L11:
        mov     rbx, qword [rbp-8]
        leave
        ret

main:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
        lea     rax, [rbp-8]
        mov     rsi, rax
        mov     edi, 100000
        call    find
        test    al, al
        je      .L14
        mov     rax, qword [rbp-8]
        mov     rsi, rax
        lea     rdi, [found]
        mov     eax, 0
        call    printf
        jmp     .L15
.L14:
        lea     rdi, [not_found]
        call    puts
.L15:
        mov     eax, 0
        leave
        ret

section .data

found: 
        db 0x25, 0x6c, 0x6c, 0x75, 0x0a, 0x00
not_found:
        db 0x6e, 0x6f, 0x74, 0x20, 0x66, 0x6f, 0x75, 0x6e, 0x64, 0x00