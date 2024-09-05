global main
extern puts

main:
    sub rsp, 8
    lea rdi, [msg]
    call puts
    xor eax, eax
    add rsp, 8
    ret

section .data
    msg: db 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x00