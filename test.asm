global main

main:
    mov rax, 1  ; write
    mov rdi, 1  ; fd = stdout
    lea rsi, [rip + 25] ; address of msg
    mov rdx, 5  ; size
    syscall
    mov rax, 60 ; exit
    mov rdi, 0
    syscall
    msg: db 70 75 6C 61 0a