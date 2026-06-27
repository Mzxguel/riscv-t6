import sys, struct

inp, outp = sys.argv[1], sys.argv[2]
vaddr = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0x0
with open(inp, "rb") as f:
    code = f.read()

EHSIZE, PHENTSIZE = 52, 32
phoff = EHSIZE
code_off = EHSIZE + PHENTSIZE
entry = vaddr

e_ident = b"\x7fELF" + bytes([1,1,1,0]) + b"\x00"*8
ehdr = e_ident
ehdr += struct.pack("<HHIIIIIHHHHHH",
    2,        
    0xF3,     
    1,       
    entry,    
    phoff, 
    0,    
    0,    
    EHSIZE,  
    PHENTSIZE,
    1,      
    0, 0, 0)

phdr = struct.pack("<IIIIIIII",
    1,         
    code_off,    
    vaddr,     
    vaddr,      
    len(code), 
    len(code),  
    5,
    4) 

with open(outp, "wb") as f:
    f.write(ehdr + phdr + code)
print(f"ELF escrito: {outp} (entry=0x{entry:08X}, {len(code)} bytes de codigo)")
