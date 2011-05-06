

CFLAGS=-Wall -O

all: read_ir ir_meterlog


clean:
	rm -f read_ir ir_meterlog
size:
	size read_ir ir_meterlog

format:
	indent -kr -ts4 -i4 read_ir.c ir_meterlog.c read_ir_breakdetect.c
	rm -f *.[ch]~

