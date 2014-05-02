bin_PROGRAMS=mergelog zmergelog

mergelog: mergelog.c
	$(COMPILE) $(LIBS) -DVERSION=\"$(VERSION)\" -Wall mergelog.c -o $@

zmergelog: mergelog.c
	$(COMPILE) $(LIBS) -DVERSION=\"$(VERSION)\" -Wall -DUSE_ZLIB mergelog.c -o $@
