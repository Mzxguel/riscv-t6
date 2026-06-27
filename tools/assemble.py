import sys, struct, re

REGS = {f"x{i}": i for i in range(32)}
ABI = {
    "zero":0,"ra":1,"sp":2,"gp":3,"tp":4,"t0":5,"t1":6,"t2":7,
    "s0":8,"fp":8,"s1":9,"a0":10,"a1":11,"a2":12,"a3":13,"a4":14,"a5":15,
    "a6":16,"a7":17,"s2":18,"s3":19,"s4":20,"s5":21,"s6":22,"s7":23,
    "s8":24,"s9":25,"s10":26,"s11":27,"t3":28,"t4":29,"t5":30,"t6":31,
}
REGS.update(ABI)

def reg(t):
    t = t.strip()
    if t not in REGS:
        raise ValueError(f"registro invalido: {t}")
    return REGS[t]

def imm(t, labels=None, pc=None, rel=False):
    t = t.strip()
    if labels is not None and t in labels:
        return (labels[t] - pc) if rel else labels[t]
    return int(t, 0)

def R(f7,rs2,rs1,f3,rd,op): return (f7<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op
def I(immv,rs1,f3,rd,op):   return ((immv&0xFFF)<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op
def S(immv,rs2,rs1,f3,op):
    immv&=0xFFF
    return ((immv>>5)<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|((immv&0x1F)<<7)|op
def B(immv,rs2,rs1,f3,op):
    immv&=0x1FFF
    b12=(immv>>12)&1; b11=(immv>>11)&1; b10_5=(immv>>5)&0x3F; b4_1=(immv>>1)&0xF
    return (b12<<31)|(b10_5<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(b4_1<<8)|(b11<<7)|op
def U(immv,rd,op): return (immv&0xFFFFF000)|(rd<<7)|op
def J(immv,rd,op):
    immv&=0x1FFFFF
    b20=(immv>>20)&1; b10_1=(immv>>1)&0x3FF; b11=(immv>>11)&1; b19_12=(immv>>12)&0xFF
    return (b20<<31)|(b10_1<<21)|(b11<<20)|(b19_12<<12)|(rd<<7)|op

ALU3   = {"add":0,"sub":0,"sll":1,"slt":2,"sltu":3,"xor":4,"srl":5,"sra":5,"or":6,"and":7}
IMM3   = {"addi":0,"slti":2,"sltiu":3,"xori":4,"ori":6,"andi":7}
SH3    = {"slli":1,"srli":5,"srai":5}
LOAD3  = {"lb":0,"lh":1,"lw":2,"lbu":4,"lhu":5}
STORE3 = {"sb":0,"sh":1,"sw":2}
BR3    = {"beq":0,"bne":1,"blt":4,"bge":5,"bltu":6,"bgeu":7}

def split_mem(arg):
    """ 'imm(rs1)' -> (imm, rs1) """
    m = re.match(r"\s*(-?[\w]+)\s*\(\s*(\w+)\s*\)\s*", arg)
    if not m:
        raise ValueError(f"esperaba imm(rs1): {arg}")
    return m.group(1), m.group(2)

def expand_pseudo(mn, ops):
    """Devuelve lista de reales. Soporta pseudo basicas."""
    if mn == "nop":   return [("addi", ["x0","x0","0"])]
    if mn == "mv":    return [("addi", [ops[0], ops[1], "0"])]
    if mn == "j":     return [("jal",  ["x0", ops[0]])]
    if mn == "ret":   return [("jalr", ["x0", "ra", "0"])]
    if mn == "li":    return [("__li", ops)]   # resuelto en 2da pasada
    if mn == "la":    return [("__la", ops)]
    if mn == "call":  return [("jal",  ["ra", ops[0]])]
    return [(mn, ops)]

def assemble(text):
    items = []
    labels = {}
    pc = 0
    for raw in text.splitlines():
        line = raw.split("#",1)[0].split(";",1)[0].strip()
        if not line:
            continue
        while ":" in line.split()[0] if line.split() else False:
            lab, _, rest = line.partition(":")
            labels[lab.strip()] = pc
            line = rest.strip()
            if not line:
                break
        if not line:
            continue
        parts = line.split(None, 1)
        mn = parts[0].lower()
        ops_str = parts[1] if len(parts) > 1 else ""
        if mn == ".word":
            for v in ops_str.split(","):
                items.append(("dword", v.strip())); pc += 4
            continue
        if mn == ".byte":
            for v in ops_str.split(","):
                items.append(("dbyte", v.strip())); pc += 1
            continue
        if mn in (".asciz",".string",".ascii"):
            s = bytes(ops_str.strip().strip('"').encode().decode("unicode_escape"),"latin1")
            if mn != ".ascii": s += b"\x00"
            items.append(("dbytes", s)); pc += len(s)
            continue
        if mn == ".space":
            n = int(ops_str,0); items.append(("dbytes", b"\x00"*n)); pc += n
            continue
        if mn == ".align":
            a = 1 << int(ops_str,0)
            pad = (-pc) % a
            if pad: items.append(("dbytes", b"\x00"*pad)); pc += pad
            continue
        ops = [o.strip() for o in ops_str.split(",")] if ops_str else []
        for rmn, rops in expand_pseudo(mn, ops):
            items.append(("instr", rmn, rops, pc))
            if rmn in ("__li","__la"):
                val = 0
                pc += 8
            else:
                pc += 4
    out = bytearray()
    addr = 0
    def emit(word):
        nonlocal addr
        out.extend(struct.pack("<I", word & 0xFFFFFFFF)); addr += 4

    for it in items:
        if it[0] == "dword":
            out.extend(struct.pack("<I", imm(it[1], labels) & 0xFFFFFFFF)); addr += 4; continue
        if it[0] == "dbyte":
            out.append(imm(it[1], labels) & 0xFF); addr += 1; continue
        if it[0] == "dbytes":
            out.extend(it[1]); addr += len(it[1]); continue

        _, mn, ops, ipc = it
        if mn in IMM3:
            out_word = I(imm(ops[2],labels), reg(ops[1]), IMM3[mn], reg(ops[0]), 0x13); emit(out_word)
        elif mn in SH3:
            f7 = 0x20 if mn=="srai" else 0x00
            sh = imm(ops[2],labels) & 0x1F
            emit(I((f7<<5)|sh, reg(ops[1]), SH3[mn], reg(ops[0]), 0x13))
        elif mn in ALU3:
            f7 = 0x20 if mn in ("sub","sra") else 0x00
            emit(R(f7, reg(ops[2]), reg(ops[1]), ALU3[mn], reg(ops[0]), 0x33))
        elif mn in LOAD3:
            im, rs1 = split_mem(ops[1])
            emit(I(imm(im,labels), reg(rs1), LOAD3[mn], reg(ops[0]), 0x03))
        elif mn in STORE3:
            im, rs1 = split_mem(ops[1])
            emit(S(imm(im,labels), reg(ops[0]), reg(rs1), STORE3[mn], 0x23))
        elif mn in BR3:
            off = imm(ops[2], labels, pc=ipc, rel=True)
            emit(B(off, reg(ops[1]), reg(ops[0]), BR3[mn], 0x63))
        elif mn == "lui":
            emit(U(imm(ops[1],labels)<<12 if imm(ops[1],labels) < 0x100000 else imm(ops[1],labels), reg(ops[0]), 0x37))
        elif mn == "auipc":
            v = imm(ops[1],labels)
            emit(U((v<<12) if v < 0x100000 else v, reg(ops[0]), 0x17))
        elif mn == "jal":
            if len(ops)==1: rd, tgt = "ra", ops[0]
            else:           rd, tgt = ops[0], ops[1]
            off = imm(tgt, labels, pc=ipc, rel=True)
            emit(J(off, reg(rd), 0x6F))
        elif mn == "jalr":
            if len(ops)==2:
                rd, rs1, off = ops[0], ops[1], "0"
            elif "(" in ops[1]:
                im, rs1 = split_mem(ops[1]); rd, off = ops[0], im
            else:
                rd, rs1, off = ops[0], ops[1], ops[2]
            emit(I(imm(off,labels), reg(rs1), 0, reg(rd), 0x67))
        elif mn == "ecall":
            emit(0x00000073)
        elif mn == "ebreak":
            emit(0x00100073)
        elif mn == "__li":
            rd = reg(ops[0]); val = imm(ops[1], labels) & 0xFFFFFFFF
            lo = val & 0xFFF
            hi = (val - (lo if lo < 0x800 else lo - 0x1000)) & 0xFFFFFFFF
            if (val & 0xFFFFF800) in (0, 0xFFFFF800):
                emit(I(val, 0, 0, rd, 0x13)); emit(0x00000013)
            else:
                emit(U(hi, rd, 0x37))
                simm = lo if lo < 0x800 else lo - 0x1000
                emit(I(simm & 0xFFF, rd, 0, rd, 0x13))
        elif mn == "__la":
            rd = reg(ops[0]); val = imm(ops[1], labels) & 0xFFFFFFFF
            if val < 0x800:
                emit(I(val, 0, 0, rd, 0x13)); emit(0x00000013)
            else:
                lo = val & 0xFFF
                hi = (val - (lo if lo < 0x800 else lo - 0x1000)) & 0xFFFFFFFF
                emit(U(hi, rd, 0x37))
                simm = lo if lo < 0x800 else lo - 0x1000
                emit(I(simm & 0xFFF, rd, 0, rd, 0x13))
        else:
            raise ValueError(f"instruccion no soportada: {mn}")
    return bytes(out)

def main():
    if len(sys.argv) != 3:
        print("uso: python3 assemble.py entrada.s salida.bin"); sys.exit(1)
    with open(sys.argv[1]) as f:
        text = f.read()
    data = assemble(text)
    with open(sys.argv[2], "wb") as f:
        f.write(data)
    print(f"Ensamblado {sys.argv[1]} -> {sys.argv[2]} ({len(data)} bytes)")

if __name__ == "__main__":
    main()
