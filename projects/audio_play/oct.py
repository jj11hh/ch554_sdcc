A4 = 440
A1 = 55
A0 = 27.5
C0 = A0 * (2 ** (-9/12))

#define FREQ_2_ACC(f) ((uint16_t)(65535/((FREQ_SYS/256/SAMPLE_RATE_DIV)/(f))))

FREQ_SYS = 24_000_000
SRD = 2



def freq_2_acc(f):
    return round(65535 / ((FREQ_SYS/256/SRD) / f))

def tbl(base_freq):
    for i in range(8*12):
        yield base_freq * 2 ** (i/12)

for i, a in enumerate(map(freq_2_acc, tbl(C0))):
    print("{:<4d}".format(a), end=", ")
    if i % 12 == 11:
        print("")

for i in range(8*12):
    sym = ["C", "CS", "D", "DS", "E", "F", "FS", "G", "GS", "A", "AS", "B"][i % 12]
    num = i // 12
    print(f"#define {sym}{num}{' ' if len(sym) == 1 else ''}     0x{i :02X}")