#ifndef _CIMLXS_H_
#define _CIMLXS_H_

/*
 * cimlxs.h
 *
 * (C) Copyright IBM Corp. 2002
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE 
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE 
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
 *
 * Author: Viktor Mihajlovski <mihajlov@de.ibm.com>
 * Contributors:
 *
 * Description: Linux service representation
 */


typedef struct {
  char * svSystemCCname;
  char * svSystemname;
  char * svCCname;
  char * svName;
  char * svCaption;
  char * svDescription;
  char * svInstalldate;
  char * svStatus;
  char * svStartmode;
  int    svStarted;
  char ** svProcesses;
} LXS_Service;

typedef void * LXS_Handle;

LXS_Handle LXS_Init();
LXS_Handle LXS_InitFiltered(const char *);
void LXS_Term(LXS_Handle);

LXS_Service* LXS_Next(LXS_Handle);

int LXS_ServiceOp(const char *, const char *);

#endif
