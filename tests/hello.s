# Imprime un saludo usando ecall (servicio 4 = print_string) y termina.
    li   a7, 4          # a7 = 4  -> print_string
    la   a0, msg        # a0 = direccion del texto
    ecall
    li   a7, 10         # a7 = 10 -> exit
    ecall
msg: .asciz "Hola, RISC-V!\n"
