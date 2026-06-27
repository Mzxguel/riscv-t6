# Calcula la suma 1+2+...+10 = 55 con un bucle y la imprime con ecall.
    li   t0, 0          # t0 = acumulador = 0
    li   t1, 1          # t1 = contador i = 1
    li   t2, 11         # t2 = limite (i < 11)
loop:
    bge  t1, t2, done   # si i >= 11 -> salir del bucle
    add  t0, t0, t1     # acum += i
    addi t1, t1, 1      # i++
    jal  x0, loop       # repetir
done:
    mv   a0, t0         # a0 = resultado
    li   a7, 1          # a7 = 1 -> print_int
    ecall
    li   a0, 10         # a0 = '\n'
    li   a7, 11         # a7 = 11 -> print_char
    ecall
    li   a7, 10         # exit
    ecall
