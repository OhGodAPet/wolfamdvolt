CC = gcc
WINCC = x86_64-w64-mingw32-gcc
CFLAGS = -O3 -s -static
LIBS = libpci.a -lm
WINLIBS = -lm

all: wolfamdvolt wolfamdbg

wolfamdvolt: main.c amdi2c.c amdi2cdbg.c amdgpu.c amdgpu.h amdi2c.h amdi2cdbg.h vbios-tables.h amdsmbus.c amdsmbus.h amdpmbus.c amdpmbus.h ir356xx.c ir356xx.h ir35217.c ir35217.h ncp81022.c ncp81022.h up9505.c up9505.h up1801.c up1801.h rt8894a.c rt8894a.h vrm.c vrm.h wolfamdvolt.h wolfamdvolt-args.c
	$(CC) $(CFLAGS) main.c amdi2c.c amdi2cdbg.c amdgpu.c wolfamdvolt-args.c ir356xx.c ir35217.c amdsmbus.c amdpmbus.c ncp81022.c up9505.c up1801.c rt8894a.c vrm.c $(LIBS) -o wolfamdvolt

wolfamdbg: wolfamdbg.c wolfamdbg.h wolfamdbg-args.c amdi2c.c amdi2cdbg.c amdgpu.c amdgpu.h amdi2c.h amdi2cdbg.h wolfamdbg.h amdsmbus.c amdsmbus.h amdpmbus.c amdpmbus.h
	$(CC) $(CFLAGS) wolfamdbg.c wolfamdbg-args.c amdi2c.c amdi2cdbg.c amdgpu.c amdsmbus.c amdpmbus.c $(LIBS) -o wolfamdbg

ad527xdbg: ad527xdbg.c ad527xdbg.h ad527xdbg-args.c amdi2c.c amdi2cdbg.c amdgpu.c amdgpu.h amdi2c.h amdi2cdbg.h ad527xdbg.h
	$(CC) $(CFLAGS) ad527xdbg.c ad527xdbg-args.c amdi2c.c amdi2cdbg.c amdgpu.c $(LIBS) -o ad527xdbg

wolfir35217dbg: wolfir35217dbg.c wolfamdbg.h wolfamdbg-args.c amdi2c.c amdi2cdbg.c amdgpu.c amdgpu.h amdi2c.h amdi2cdbg.h wolfamdbg.h amdsmbus.c amdsmbus.h amdpmbus.c amdpmbus.h vrm.c vrm.h ir356xx.c ir356xx.h ir35217.c ir35217.h ncp81022.c up9505.c up9505.h up1801.c up1801.h rt8894a.c rt8894a.h
	$(CC) $(CFLAGS) wolfir35217dbg.c wolfamdbg-args.c amdi2c.c amdi2cdbg.c amdgpu.c amdsmbus.c amdpmbus.c vrm.c ir356xx.c ir35217.c ncp81022.c up9505.c up1801.c rt8894a.c $(LIBS) -o wolfir35217dbg

windows-all: win-wolfamdvolt win-wolfamdbg

win-wolfamdvolt: main.c amdi2c.c amdi2cdbg.c amdgpu.c amdgpu.h amdi2c.h amdi2cdbg.h vbios-tables.h wolfamdvolt-args.c
	$(WINCC) $(CFLAGS) main.c amdi2c.c amdi2cdbg.c amdgpu.c wolfamdvolt-args.c IR356XX.c amdsmbus.c amdpmbus.c ncp81022.c up9505.c up1801.c rt8894a.c vrm.c $(WINLIBS) -o wolfamdvolt

win-wolfamdbg: wolfamdbg.c wolfamdbg.h wolfamdbg-args.c amdi2c.c amdi2cdbg.c amdgpu.c amdgpu.h amdi2c.h amdi2cdbg.h wolfamdbg.h amdsmbus.c amdsmbus.h amdpmbus.c amdpmbus.h
	$(WINCC) $(CFLAGS) wolfamdbg.c wolfamdbg-args.c amdi2c.c amdi2cdbg.c amdgpu.c amdsmbus.c amdpmbus.c $(WINLIBS) -o wolfamdbg
