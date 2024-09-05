global main
extern printf

main:
    mov rax, 1  ; write
    mov rdi, 1  ; fd = stdout
    lea rsi, [msg2] ; address of msg
    mov rdx, 6  ; size
    call printf
    syscall
lbl:
    jmp lbl
    mov rax, 60 ; exit
    mov rdi, 2
    syscall

section .data
    null:
    db 0x00, 0x00, 0x00, 0x00
    msg2:
    db 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x0a, 0, 0, 0

section .bss
    resb 4