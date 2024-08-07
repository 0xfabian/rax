section .data
    format db "Address of main: %p", 10, 0  ; Format string with newline

section .text
    extern printf
    global main

main:
    push rbp
    mov rbp, rsp

    lea rdi, [rel format]  ; Load address of format string
    lea rsi, [rel main]    ; Load address of main
    xor eax, eax           ; Clear eax register (no floating-point arguments)
    call printf            ; Call printf

    mov eax, 0
    leave
    ret
