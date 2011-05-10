

all:  avr host


avr:
	make -C avr all

host:
	make -C host all

DEST=/usr/local/power
install:
	mv $(DEST)/read_ir $(DEST)/read_ir.old
	mv $(DEST)/ir_meterlog $(DEST)/ir_meterlog.old
	cd host; cp -a read_ir ir_meterlog plotfix acquire $(DEST)
	cd plot; cp -a power power.plot index.html.template $(DEST)

clean:
	make -C avr clean
	make -C host clean

RVER=$(shell git log --oneline | wc -l)
RHASH=$(shell echo -n $$(git rev-parse --short HEAD);  \
	    git diff-index --quiet HEAD || echo "-dirty" )

archive:
	git archive --format tar \
	    --prefix irmetermon/ \
		master | gzip -c > irmetermon-$(RHASH)-$(RVER).tgz

.PHONY:  avr host archive
