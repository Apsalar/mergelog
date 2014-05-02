mergelog
========

Apsalar's fork of Bertrand Demiddelaer's mergelog utility. We compress our
production web server logs using bzip2 for significantcompression gains over
gzip, and needed the ability to process log data over multiple hosts. Our
version supports merging bzipped log files using a new utility bzmergelog
similar to the gzip-merging zmergelog.

For more details on mergelog, see: http://mergelog.sourceforge.net/

This version is a straight retrofit of bzip2 instead of zlib, using the
existing framework. Unfortunately, this also means bzip2 decompression is
single-threaded and in most cases bzmergelog will be CPU bound, not disk I/O
bound as it ought to be.
