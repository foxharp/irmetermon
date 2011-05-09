

CFLAGS=-Wall -O

all: read_ir ir_meterlog plotfix

clean:
	rm -f read_ir ir_meterlog plotfix
size:
	size read_ir ir_meterlog plotfix

format:
	indent -kr -ts4 -i4 read_ir.c ir_meterlog.c read_ir_breakdetect.c plotfix.c
	rm -f *.[ch]~

