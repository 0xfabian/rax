export main
import printf

main:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 32
        mov     qword [rbp-8], 0
        mov     qword [rbp-16], 1
        mov     dword [rbp-20], 0
        jmp     .L2
.L3:
        mov     rax, qword [rbp-8]
        mov     rsi, rax
        lea     rdi, [fmt]
        mov     eax, 0
        call    printf
        mov     rdx, qword [rbp-8]
        mov     rax, qword [rbp-16]
        add     rax, rdx
        mov     qword [rbp-32], rax
        mov     rax, qword [rbp-16]
        mov     qword [rbp-8], rax
        mov     rax, qword [rbp-32]
        mov     qword [rbp-16], rax
        add     dword [rbp-20], 1
.L2:
        cmp     dword [rbp-20], 99
        jle     .L3
        mov     eax, 0
        leave
        ret

section .data

fmt: 
        db 0x25, 0x6c, 0x6c, 0x64, 0x0a, 0x00