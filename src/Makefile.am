bin_PROGRAMS=mergelog zmergelog bzmergelog

mergelog: mergelog.c
	$(COMPILE) $(LIBS) -DVERSION=\"$(VERSION)\" -Wall mergelog.c -o $@

zmergelog: mergelog.c
	$(COMPILE) $(LIBS) -DVERSION=\"$(VERSION)\" -Wall -DUSE_ZLIB mergelog.c -o $@

bzmergelog: mergelog.c
	$(COMPILE) -DVERSION=\"$(VERSION)\" -Wall -DUSE_BZIP2 mergelog.c $(LIBS) -o $@
