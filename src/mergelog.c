/*
  mergelog - a fast tool to merge and sort by date http log files

  Copyright (C) 2000-2001 Bertrand Demiddelaer <bert@zehc.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


/*
  Contribution:
  Daniel Sully <daniel-zlib@electricrain.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <zlib.h>
#include <unistd.h>


#define BUFFER_SIZE 32768
#define DATE_SIZE 14
#define SCAN_OFFSET 9
#define SCAN_SIZE 512


#ifdef USE_ZLIB
#define myFH gzFile
#define myopen gzopen
#define mygets(a,b,c,d) fast_gzgets(a,b,c,d)
#define myrewind gzrewind
#define myclose gzclose
#else
#define myFH FILE
#define myopen fopen
#define mygets(a,b,c,d) fgets(a,b,c)
#define myrewind rewind
#define myclose fclose
#endif


#ifdef USE_ZLIB
#define GZBUFFER_SIZE 65536
#define MAX_FILES 512


char    *f_buf[MAX_FILES];
char    *f_cp[MAX_FILES];
char    *f_end[MAX_FILES];


/*
  replacement for gzgets which reduces calls to zlib decompression
 */
char *fast_gzgets (char *buf, int size, myFH *fp, int i) {
  
  char *out_cp=buf;

  for(;;) {
    if (f_cp[i]>f_end[i]) {
      f_end[i]=gzread(fp, f_buf[i], GZBUFFER_SIZE)+f_buf[i]-1;
      if (f_end[i]<f_buf[i]) {
	return NULL;
      }
      f_cp[i]=f_buf[i];
    }
    if (--size) {
      *out_cp++ = *f_cp[i];
      if (*f_cp[i]++ == '\n') {
        *out_cp='\0';
        return buf;
      }
    } else {
      *out_cp='\0';
      return buf;
    }
  }
}
#endif


int main (int argc, char *argv[]) {

  int i,j,nb_files,nb_files_orig;
  myFH *log_file[argc-1];
  char *log_buffer[argc-1];
  char *log_scan[argc-1];
  char *log_month[argc-1];
  char ref_date_buf[DATE_SIZE+1];
  char *tmp_date_buf[argc-1];
  char *log_date;
  int year,day,hour,minut,second;
  char month[3];
  struct tm *date;
  time_t start=0;
  time_t start_new;
  char *trans_digits[60];
  char *trans_year[200];
  char months[24]="anebarprayunulugepctovec";

  /*
    print usage if necessary
   */
  if (argc == 1) {
    fprintf(stderr,"usage: %s logfile1 logfile2 ...\nmergelog %s Copyright (C) 2000-2001 Bertrand Demiddelaer\n",argv[0],VERSION);
    exit(1);
  }

#ifdef USE_ZLIB
  /*
    check if there are enough gunzip buffers
   */
  if(argc>MAX_FILES) {
    fputs("too many gzipped log files, aborting\n",stderr);
    exit(1);
  }
#endif

  /*
    open log files
  */
  for (i=1;i<argc;i++) {
    log_file[i-1]=myopen(argv[i],"r");
    if (log_file[i-1] == NULL) {
      fprintf(stderr,"can't open %s, aborting\n",argv[i]);
      exit(1);
    }
  }

  /*
    feed arrays which will be used to translate dates
   */
  for(i=0;i<60;i++) {
    trans_digits[i]=malloc(3);
    if (trans_digits[i] == NULL) {
      perror("malloc");
      exit(1);
    }
    sprintf(trans_digits[i],"%.2d",i);
  }
  for (i=70;i<200;i++) {
    trans_year[i]=malloc(5);
    if (trans_year[i] == NULL) {
      perror("malloc");
      exit(1);
    }
    sprintf(trans_year[i],"%.4d",1900+i);
  }

  /*
    malloc for the 'tm' structure
   */
  date=malloc(sizeof(struct tm));
  if (date == NULL) {
    perror("malloc");
    exit(1);
  }

  /*
    init things for each log file and get the older date to start with
   */
  nb_files=argc-1;
  for (i=0;i<argc-1;i++) {

#ifdef USE_ZLIB
    /*
      init the gzip buffer
     */
    f_buf[i]=malloc(GZBUFFER_SIZE);
    if (f_buf[i] == NULL) {
      perror("malloc");
      exit(1);
    }
    f_cp[i]=f_buf[i]+GZBUFFER_SIZE;
#endif

    /*
      init log_month buffers
      the first 2 digits will be used for the number of the month
      the last 2 digits will be used for the two letters of the month
     */
    log_month[i]=calloc(4,1);
    if (log_month[i] == NULL) {
      perror("calloc");
      exit(1);
    }

    /*
      get the first line of the log file and init log_scan
     */
    log_buffer[i]=malloc(BUFFER_SIZE);
    log_scan[i]=log_buffer[i]+SCAN_OFFSET;
    if (log_buffer[i] == NULL) {
      perror("malloc");
      exit(1);
    }

    /*
      init the tmp_date_buf
     */
    tmp_date_buf[i]=malloc(DATE_SIZE+1);
    if (tmp_date_buf[i] == NULL) {
      perror("malloc");
      exit(1);
    }
    memset(tmp_date_buf[i]+DATE_SIZE,'0',1);

    /*
      is it an empty file ?
     */
    if (mygets(log_buffer[i],BUFFER_SIZE,log_file[i],i) != NULL ) {

      /*
	get the date pointers
      */
      log_date=memchr(log_scan[i],'[',SCAN_SIZE);
      if (log_date == NULL) {
	fprintf(stderr,"abort due to a problem with %s:\n%s\n",argv[i+1],log_buffer[i]);
	exit(1);
      }
      
      /*
	put the date in the tmp_date_buf
       */
       for (j=0;((j == 12)&&(memcmp(months+2*j,log_date+5,2) != 0));j++);
       if (j == 12) {
	 fprintf(stderr,"abort due to a problem with %s:\n%s\n",argv[i+1],log_buffer[i]);
	 exit(1);
       }
       memcpy(log_month[i],trans_digits[j],2);
       memcpy(log_month[i]+2,months+2*j,2);
       memcpy(tmp_date_buf[i],log_date+8,4);
       memcpy(tmp_date_buf[i]+4,trans_digits[j],2);
       memcpy(tmp_date_buf[i]+6,log_date+1,2);
       memcpy(tmp_date_buf[i]+8,log_date+13,2);
       memcpy(tmp_date_buf[i]+10,log_date+16,2);
       memcpy(tmp_date_buf[i]+12,log_date+19,2);

      /*
	extract the date of this first line
      */
       if (sscanf(log_date+1,"%d/%3c/%d:%d:%d:%d",&day,month,&year,&hour,&minut,&second) < 6) {
	 fprintf(stderr,"abort due to a problem with %s:\n%s\n",argv[i+1],log_buffer[i]);
	 exit(1);
       }
      
      /*
	put this date in a 'tm' structure
      */
      date->tm_sec=second;
      date->tm_min=minut;
      date->tm_hour=hour;
      date->tm_mday=day;
      date->tm_year=year-1900;
      date->tm_isdst=-1;
      for (j=0;((j<12)&&(memcmp(months+2*j,month+1,2) != 0));j++);
      if (j == 12) {
	fprintf(stderr,"abort due to a problem with %s:\n%s\n",argv[i+1],log_buffer[i]);
	exit(1);
      }
      date->tm_mon=j;
      memcpy(log_month[i],trans_digits[j],2);
      memcpy(log_month[i]+2,months+2*j,2);
      memcpy(tmp_date_buf[i]+4,trans_digits[j],2);

      /*
	convert it in the 'seconds since 00:00:00, Jan 1, 1970' format
      */
      start_new=mktime(date);
      
      /*
	keep the older date
      */
      if ((start_new < start)||(start == 0)) {
	start=start_new;
      }
    } else {

      /*
	this is an empty file
       */
      nb_files--;
      *(tmp_date_buf[i])='9';
    }
  }

  /*
    exit if we have only empty files
   */
  if (nb_files == 0) {
    exit(0);
  }

  /*
    init 'start', 'date' and 'ref_date_buf'
  */
  free(date);
  start--;
  date=localtime(&start);
  memcpy(ref_date_buf,trans_year[date->tm_year],4);
  memcpy(ref_date_buf+4,trans_digits[date->tm_mon],2);
  memcpy(ref_date_buf+6,trans_digits[date->tm_mday],2);
  memcpy(ref_date_buf+8,trans_digits[date->tm_hour],2);
  memcpy(ref_date_buf+10,trans_digits[date->tm_min],2);
  memcpy(ref_date_buf+12,trans_digits[date->tm_sec],2);
  memset(ref_date_buf+DATE_SIZE,'0',1);

  /*
    start to compute since this date
  */
  nb_files_orig=argc-1;
  for(;;) {

    /*
      update 'start' 'date' and 'ref_date_buf'
    */
    start++;
    if (date->tm_sec < 59) {
      date->tm_sec++;
      memcpy(ref_date_buf+12,trans_digits[date->tm_sec],2);
    } else {
      date->tm_sec=0;
      memset(ref_date_buf+12,'0',2);
      if (date->tm_min < 59) {
	date->tm_min++;
	memcpy(ref_date_buf+10,trans_digits[date->tm_min],2);
      } else {
	date->tm_min=0;
	memset(ref_date_buf+10,'0',2);
	if (date->tm_hour < 23) {
	  date->tm_hour++;
	  memcpy(ref_date_buf+8,trans_digits[date->tm_hour],2);
	} else {
	  memset(ref_date_buf+8,'0',2);
	  date=localtime(&start);
	  memcpy(ref_date_buf,trans_year[date->tm_year],4);
	  memcpy(ref_date_buf+4,trans_digits[date->tm_mon],2);
	  memcpy(ref_date_buf+6,trans_digits[date->tm_mday],2);
	}
      }
    }

    /*
      scan this date for each log file
     */
    for(i=0;i<nb_files_orig;i++) {

      /*
	write the log lines until the reference date is older than the log line
      */
      for(;;) {

	/*
	  if the reference date is older than the log line then go to next file
          we use here a faster implementation than something like:
          if (memcmp(ref_date_buf,tmp_date_buf[i],DATE_SIZE)<0) break;
	 */
	for(j=0;(j<DATE_SIZE)&&(*(ref_date_buf+j)==*(tmp_date_buf[i]+j));j++);
	if (*(ref_date_buf+j)<*(tmp_date_buf[i]+j)) break;

	/*
	  write the log line
	  faster than a puts and we are sure to find a '\0' in log_buffer[i]
	 */
	write(1,log_buffer[i],(size_t)((char *)memchr(log_buffer[i],0,BUFFER_SIZE)-log_buffer[i]));

	/*
	  is it an end of file ?
	 */
	if (mygets(log_buffer[i],BUFFER_SIZE,log_file[i],i) == NULL) {

	  /*
	    close all log files and exit if all end of files are reached
	   */
	  if (--nb_files == 0) {
	    for (j=0;j<argc-1;j++) {
	      myclose(log_file[j]);
	    }
	    exit(0);
	  }

	  /*
	    we don't want anymore output from this file
	    we put a '9' at the beginning  of the year, to fail the date test
	    it's dirty, but it's fast, and doesn't need an extra test
	   */
	  *(tmp_date_buf[i])='9';
          break;
	} else {

	  /*
	    prepare the new pointer for the date test
	   */
	  log_date=memchr(log_scan[i],'[',SCAN_SIZE);
	  if (log_date != NULL) {

	    /*
	      convert the log line month if necessary
	      copy the new date in the buffer
	    */
	    if ((*(log_month[i]+2)==*(log_date+5))&&(*(log_month[i]+3)==*(log_date+6))) {	
	      memcpy(tmp_date_buf[i]+4,log_month[i],2);

	      memcpy(tmp_date_buf[i],log_date+8,4);
	      memcpy(tmp_date_buf[i]+6,log_date+1,2);
	      memcpy(tmp_date_buf[i]+8,log_date+13,2);
	      memcpy(tmp_date_buf[i]+10,log_date+16,2);
	      memcpy(tmp_date_buf[i]+12,log_date+19,2);

	    } else {
	      for (j=0;((j<12)&&(memcmp(months+2*j,log_date+5,2) != 0));j++);
	      if (j == 12) {
		fprintf(stderr,"problem with %s:\n%s\ncontinuing...\n",argv[i+1],log_buffer[i]);
	      } else {
		memcpy(log_month[i],trans_digits[j],2);
		memcpy(log_month[i]+2,months+2*j,2);
		memcpy(tmp_date_buf[i]+4,trans_digits[j],2);
		
		memcpy(tmp_date_buf[i],log_date+8,4);
		memcpy(tmp_date_buf[i]+6,log_date+1,2);
		memcpy(tmp_date_buf[i]+8,log_date+13,2);
		memcpy(tmp_date_buf[i]+10,log_date+16,2);
		memcpy(tmp_date_buf[i]+12,log_date+19,2);
	      }
	    }
	  } else {
	    fprintf(stderr,"problem with %s:\n%s\ncontinuing...\n",argv[i+1],log_buffer[i]);
	  }
	}
      }
    }
  }

  /*
    never reached
   */
  exit(1);
}
