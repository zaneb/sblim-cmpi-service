/*
 * svslist.c -- test program
 *
 * (C) Copyright IBM Corp. 2002
 *
 */

#include "cimlxs.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
  LXS_Handle   lhdl;
  LXS_Service *lxssvc;
  int i;

  lhdl = LXS_Init();

  if (lhdl == NULL) {
    fprintf(stderr,"No services found or error occurred.\n");
    exit(-1);
  }
   
  do {
    lxssvc = LXS_Next(lhdl);
    if (lxssvc != NULL) {
      printf("Service %s is %s", lxssvc->svName, 
	     lxssvc->svStarted ? "started" : "stopped");
      i=0;
      while (lxssvc->svProcesses[i]) printf(" pid=%s",
					    lxssvc->svProcesses[i++]);
      printf("\n");
    }
  } while (lxssvc != NULL);

  LXS_Term(lhdl);

  return 0;
}
