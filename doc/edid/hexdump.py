f = open("EDID_vga_aud.bin", "rb")
data = f.read()
f.close()
n=0
for b in data:
    print ('{:02X}'.format(b), end=" ")
    n=n+1
    if n % 16 == 0:
        print()
        