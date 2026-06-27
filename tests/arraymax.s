# Escribe el arreglo {3,7,2,9,5} en memoria (0x100) y halla el maximo (9).
    li   t0, 0x100      # base del arreglo
    li   t1, 3
    sw   t1, 0(t0)
    li   t1, 7
    sw   t1, 4(t0)
    li   t1, 2
    sw   t1, 8(t0)
    li   t1, 9
    sw   t1, 12(t0)
    li   t1, 5
    sw   t1, 16(t0)
    li   t2, 5          # n = 5
    lw   t4, 0(t0)      # max = arr[0]
    li   t3, 1          # i = 1
loop:
    bge  t3, t2, done   # i >= n -> fin
    slli t5, t3, 2      # offset = i*4
    add  t5, t0, t5     # addr = base + offset
    lw   t6, 0(t5)      # x = arr[i]
    bge  t4, t6, skip   # si max >= x no actualiza
    mv   t4, t6         # max = x
skip:
    addi t3, t3, 1      # i++
    jal  x0, loop
done:
    mv   a0, t4         # a0 = maximo
    li   a7, 1          # print_int
    ecall
    li   a0, 10
    li   a7, 11         # print_char '\n'
    ecall
    li   a7, 10         # exit
    ecall
