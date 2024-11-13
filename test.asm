global main
extern puts
extern printf
extern scanf

is_prime:
        push    rbp
        mov     rbp, rsp
        mov     dword [rbp-20], edi
        cmp     dword [rbp-20], 1
        jne     L2
        mov     eax, 0
        jmp     L3
L2:
        mov     dword [rbp-4], 2
        jmp     L4
L6:
        mov     eax, dword [rbp-20]
        cdq
        idiv    dword [rbp-4]
        mov     eax, edx
        test    eax, eax
        jne     L5
        mov     eax, 0
        jmp     L3
L5:
        add     dword [rbp-4], 1
L4:
        mov     eax, dword [rbp-4]
        cmp     eax, dword [rbp-20]
        jl      L6
        mov     eax, 1
L3:
        pop     rbp
        ret
LC0:
        db "> ", 0
LC1:
        db "%d", 0
LC2:
        db "true", 0
LC3:
        db "false", 0
main:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
L15:
        mov     edi, LC0
        mov     eax, 0
        call    printf
        lea     rax, [rbp-4]
        mov     rsi, rax
        mov     edi, LC1
        mov     eax, 0
        call    scanf
        test    eax, eax
        je      L8
        mov     eax, dword [rbp-4]
        test    eax, eax
        jne     L9
L8:
        mov     eax, 1
        jmp     L10
L9:
        mov     eax, 0
L10:
        test    al, al
        jne     L18
        mov     eax, dword [rbp-4]
        mov     edi, eax
        call    is_prime
        test    al, al
        je      L13
        mov     edi, LC2
        call    puts
        jmp     L15
L13:
        mov     edi, LC3
        call    puts
        jmp     L15
L18:
        nop
        mov     eax, 0
        leave
        ret