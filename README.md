# Simulador RISC-V (RV32I)

Simulador del ISA base de RISC-V de 32 bits (**RV32I**) para el curso
*Arquitectura de Computadores*. Carga un programa ya compilado
(código de máquina) y lo ejecuta paso por paso, manteniendo el estado
arquitectural completo —contador de programa (PC), 32 registros y memoria—
y permitiendo inspeccionar registros y memoria en cualquier punto.

---

## 1. Informe breve

- **Lenguaje:** C++17 (sin dependencias externas; solo la biblioteca estándar).
- **Build:** `make` (incluye un `CMakeLists.txt` alternativo).
- **Memoria:** plana, *byte-addressable*, espacio de direcciones de 32 bits,
  formato **little-endian** (como RISC-V). Está implementada de forma *sparse*
  por páginas de 4 KiB, así que un espacio de 4 GiB no consume RAM real: solo
  se reservan las páginas tocadas.
- **Carga de programas:** el programa se carga en la dirección `0x00000000`.
  Acepta **binario crudo** (el formato "Raw" de CPUlator) y, además,
  **ejecutables ELF32** de RISC-V (detecta el *magic* `0x7F 'E' 'L' 'F'` y
  usa el `e_entry` y los segmentos `PT_LOAD`).
- **Ejecución:** paso por paso (`step`) o continua (`run`), bajo control del
  usuario, mediante un REPL de línea de comandos.

### Features

| Requisito                             
|------------------------------------------------------|
| ISA base RV32I (Anexo A, 37 instrucciones)             
| Memoria little-endian                                  
| Carga de programas compilados desde archivo a `0x0`    
| Ejecución paso por paso                                
| Inspección de PC, registros y memoria                  
| *(Idea adicional)* `ecall` estilo SPIM/CPUlator        
| *(Idea adicional)* Carga de archivos `.elf`            
| *(Idea adicional)* Desensamblador integrado            

---

## 2. Compilación

```bash
make

mkdir build && cd build && cmake .. && make
```

Requiere un compilador con soporte C++17 (probado con g++ 13).

---

## 3. Uso

```bash
./build/riscv-sim [programa.bin]
```

Si se pasa un archivo, se carga automáticamente en `0x0`. Luego se entra a un
REPL interactivo:

```
$ ./build/riscv-sim tests/riscvtest.bin
=== Simulador RISC-V (RV32I) ===
"tests/riscvtest.bin" cargado a memoria (84 bytes). Entrada = 0x00000000
> step
0x00000000: 00500113   addi  x2, x0, 5
> pc
pc = 0x00000004
> regs x2
x2  (sp  ) = 0x00000005   (5)
> run
Programa detenido en bucle en 0x00000050.  (19 instrucciones)
> mem 0x64 0x67
0x00000060:             19 00 00 00                          |    ....        |
> exit
See you next time...
```

### Comandos del REPL

| Comando | Descripción |
|---------|-------------|
| `load <archivo> [base]` | Carga un programa (binario crudo o ELF). `base` por defecto `0x0`. |
| `step [n]`              | Ejecuta `n` instrucciones (por defecto 1), mostrando cada una desensamblada. |
| `run [max]`             | Ejecuta hasta `exit`/instrucción ilegal/breakpoint/bucle, o hasta `max` instrucciones. |
| `pc`                    | Muestra el PC. |
| `regs [r1 r2 ...]`      | Muestra todos los registros, o los indicados (`x5`, `a0`, `sp`, ...). |
| `mem <ini> [fin]`       | Vuelca la memoria en el rango (hex + ASCII). |
| `dis [addr] [n]`        | Desensambla `n` instrucciones desde `addr` (por defecto PC, 8). |
| `setreg <reg> <valor>`  | Escribe un registro. |
| `setmem <addr> <valor>` | Escribe un *word* en memoria. |
| `break <addr>` / `break clear` | Coloca / elimina breakpoints (los usa `run`). |
| `reset`                 | Reinicia el estado y recarga el último programa. |
| `help`                  | Muestra la ayuda. |
| `exit` \| `quit`        | Sale. |

Los registros se aceptan por número (`x5`), índice (`5`) o nombre ABI
(`a0`, `sp`, `t2`, ...). Los valores aceptan decimal o hex (`0x...`).

---

## 4. Llamadas al sistema

Convención estilo SPIM/CPUlator: **`a7`** lleva el número de servicio,
**`a0`/`a1`** los argumentos.

| `a7` | Servicio | Argumentos |
|:----:|----------|------------|
| 1  | `print_int`    | `a0` = entero con signo |
| 4  | `print_string` | `a0` = dirección de cadena terminada en `\0` |
| 5  | `read_int`     | resultado en `a0` |
| 8  | `read_string`  | `a0` = buffer, `a1` = longitud máxima |
| 10 | `exit`         | — |
| 11 | `print_char`   | `a0` = carácter |
| 12 | `read_char`    | resultado en `a0` |
| 17 | `exit2`        | `a0` = código de salida |
| 34 | `print_hex`    | `a0` = valor |
| 35 | `print_bin`    | `a0` = valor |
| 36 | `print_uint`   | `a0` = entero sin signo |

---

## 5. Instrucciones soportadas (Anexo A)

Todas las 37 instrucciones del RV32I pedidas:

- **Loads:** `lb lh lw lbu lhu`
- **Op-inmediato:** `addi slli slti sltiu xori srli srai ori andi`
- **U-type:** `lui auipc`
- **Stores:** `sb sh sw`
- **Op (R-type):** `add sub sll slt sltu xor srl sra or and`
- **Branches:** `beq bne blt bge bltu bgeu`
- **Saltos:** `jal jalr`
- **Sistema:** `ecall` (`ebreak` se trata como no-op).

Las instrucciones no reconocidas se reportan explícitamente como error
(*"instrucción ilegal"*) sin abortar el simulador.

---

## 6. Estructura del proyecto

```
riscv-sim/
├── src/
│   ├── memory.hpp        Memoria sparse little-endian (read/write byte/half/word)
│   ├── cpu.hpp/.cpp      Estado (PC, regs) + fetch-decode-execute de todo RV32I
│   ├── syscalls.hpp/.cpp Manejo de ecall (servicios SPIM/CPUlator)
│   ├── disassembler.*    Desensamblador (instrucción -> texto ensamblador)
│   ├── loader.hpp/.cpp   Carga de binario crudo y de ELF32
│   └── main.cpp          REPL (interfaz de línea de comandos)
├── tests/
│   ├── riscvtest.s/.bin  Prueba de Harris&Harris (escribe 25 en 0x64)
│   ├── hello.s/.bin      "Hola, RISC-V!" vía ecall print_string
│   ├── sum.s/.bin        Suma 1..10 = 55 (bucle + print_int)
│   ├── arraymax.s/.bin   Máximo de un arreglo en memoria = 9
│   └── hello.elf         Mismo hello, empaquetado como ELF32 (prueba del loader)
├── tools/
│   ├── make_riscvtest.py Genera riscvtest.bin
│   ├── assemble.py       Mini-ensamblador RV32I (utilidad, NO parte del simulador)
│   └── wrap_elf.py       Envuelve un .bin en un ELF32 mínimo (para probar el loader)
├── Makefile
├── CMakeLists.txt
└── README.md
```

---

## 7. Programas de prueba (4 ejemplos para la demo)

```bash
# 1) riscvtest: prueba add/sub/and/or/slt/addi/lw/sw/beq/jal.
#    Si todo va bien, escribe 25 (0x19) en la direccion 100 (0x64).
make run                       # equivale a ./build/riscv-sim tests/riscvtest.bin
> run
> mem 0x64 0x67                # -> 19 00 00 00  (= 25, little-endian)

# 2) hello: imprime texto con ecall.
./build/riscv-sim tests/hello.bin
> run                          # -> Hola, RISC-V!

# 3) sum: bucle que suma 1..10 e imprime 55.
./build/riscv-sim tests/sum.bin
> run                          # -> 55

# 4) arraymax: arreglo en memoria, halla el maximo (9).
./build/riscv-sim tests/arraymax.bin
> run                          # -> 9
> mem 0x100 0x113              # muestra el arreglo en little-endian
```

Para probar la carga de ELF: `./build/riscv-sim tests/hello.elf`.
