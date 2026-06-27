# riscvtest.s  -  Programa de prueba de Harris & Harris (RISC-V edition)
# Prueba: add, sub, and, or, slt, addi, lw, sw, beq, jal.
# Si todo sale bien, escribe el valor 25 (0x19) en la direccion 100 (0x64).
#
#       Ensamblador             Comentario                Dir   Codigo maquina
main:   addi x2, x0, 5        # x2 = 5                     00    00500113
        addi x3, x0, 12       # x3 = 12                    04    00C00193
        addi x7, x3, -9       # x7 = (12 - 9) = 3          08    FF718393
        or   x4, x7, x2       # x4 = (3 OR 5) = 7          0C    0023E233
        and  x5, x3, x4       # x5 = (12 AND 7) = 4        10    0041F2B3
        add  x5, x5, x4       # x5 = (4 + 7) = 11          14    004282B3
        beq  x5, x7, end      # no se toma                 18    02728863
        slt  x4, x3, x4       # x4 = (12 < 7) = 0          1C    0041A233
        beq  x4, x0, around   # se toma                    20    00020463
        addi x5, x0, 0        # no se ejecuta              24    00000293
around: slt  x4, x7, x2       # x4 = (3 < 5) = 1           28    0023A233
        add  x7, x4, x5       # x7 = (1 + 11) = 12         2C    005203B3
        sub  x7, x7, x2       # x7 = (12 - 5) = 7          30    402383B3
        sw   x7, 84(x3)       # mem[96] = 7                34    0471AA23
        lw   x2, 96(x0)       # x2 = mem[96] = 7           38    06002103
        add  x9, x2, x5       # x9 = (7 + 11) = 18         3C    005104B3
        jal  x3, end          # x3 = 0x44 ; salta a end    40    008001EF
        addi x2, x0, 1        # no se ejecuta              44    00100113
end:    add  x2, x2, x9       # x2 = (7 + 18) = 25         48    00910133
        sw   x2, 0x20(x3)     # mem[0x44+0x20=0x64] = 25   4C    0221A023
done:   beq  x2, x2, done     # loop infinito (fin)        50    00210063
