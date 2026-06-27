import struct, os

words = [
    0x00500113,  
    0x00C00193,  
    0xFF718393,  
    0x0023E233,  
    0x0041F2B3,  
    0x004282B3,  
    0x02728863,  
    0x0041A233,  
    0x00020463,
    0x00000293,  
    0x0023A233,  
    0x005203B3,  
    0x402383B3,  
    0x0471AA23,  
    0x06002103,  
    0x005104B3,  
    0x008001EF,  
    0x00100113,  
    0x00910133,  
    0x0221A023,
    0x00210063,  
]

out = os.path.join(os.path.dirname(__file__), "..", "tests", "riscvtest.bin")
out = os.path.normpath(out)
with open(out, "wb") as f:
    for w in words:
        f.write(struct.pack("<I", w))   # little-endian
print(f"Escrito {out} ({len(words)*4} bytes)")
