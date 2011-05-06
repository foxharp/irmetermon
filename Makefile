

all:  avr host


avr:
	make -C avr all

host:
	make -C host all

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
