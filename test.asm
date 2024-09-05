global main

main:
    mov rax, 1  ; write
    mov rdi, 1  ; fd = stdout
    lea rsi, [rip + 25] ; address of msg
    mov rdx, 6  ; size
    syscall
    mov rax, 60 ; exit
    mov rdi, 0
    syscall
    db 68 65 6c 6c 6f 0a