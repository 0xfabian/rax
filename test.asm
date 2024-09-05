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
    mov rdi, 0
    syscall

section .data
    null:
    db 00 00 00 00
    msg2:
    db 68 65 6c 6c 6f 0a