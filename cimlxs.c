/*
 * cimlxs.c
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

#include "cimlxs.h"
#include "OSBase_Common.h"
#include <stdlib.h>
#include <string.h>

static LXS_Service *strToSvc(const char *);
static void freeSvc(LXS_Service *);

typedef struct _LXS_List {
  struct _LXS_List *lxsCurrent;
  struct _LXS_List *lxsNext;
  LXS_Service      *lxsSvc;
} LXS_List; 


static char * mklistcmd(const char * arg)
{
  size_t   cmdlen;
  char     *scriptname="/listservices";
  char     *scriptdir, *cmdstring;
  
  /*
  scriptdir=getenv("NPISCRIPTDIR");
  if (scriptdir==NULL)
    scriptdir=".";
  */
  scriptdir="/usr/local/bin";

  cmdlen=strlen(scriptdir) + strlen(scriptname) + strlen(arg) + 2;
  cmdstring=malloc(cmdlen);
  strcpy(cmdstring,scriptdir);
  strcat(cmdstring,scriptname);
  strcat(cmdstring," ");
  strcat(cmdstring,arg);
  return cmdstring;
}

LXS_Handle LXS_Init()
{
  return LXS_InitFiltered("");
}

LXS_Handle LXS_InitFiltered(const char * filter)
{
  LXS_List *head=NULL, *current;
  char     **services=NULL;
  char     *cmd;
  int      i, rc;
  
  cmd=mklistcmd(filter);
  rc=runcommand(cmd,NULL,&services,NULL);
  free(cmd);
  if (rc)
    return NULL;
  i=0;
  while (services!= NULL && services[i] != NULL) {
    if (head == NULL) {
      /* first service -- set up list head */
      head = malloc(sizeof(LXS_List));
      current = head;
    } else {
      /* regular case -- advance pointer */
      current->lxsNext = malloc(sizeof(LXS_List));
      current = current->lxsNext;
    }
    current->lxsSvc = strToSvc(services[i++]);
    current->lxsNext = NULL;      
  }
  if (head != NULL)
    head->lxsCurrent = head;
  freeresultbuf(services);
  return (LXS_Handle) head;
}

void LXS_Term(LXS_Handle hdl)
{
  LXS_List *head = (LXS_List*) hdl;
  LXS_List *current = head;
  while (current != NULL) {
    freeSvc(current->lxsSvc);
    head = current->lxsNext;
    free(current);
    current = head;
  }
}

LXS_Service *LXS_Next(LXS_Handle hdl)
{
  LXS_List *current;
  if (hdl != NULL) {
    current = ((LXS_List*)hdl)->lxsCurrent;
    if (current==NULL)
      return NULL;
    ((LXS_List*)hdl)->lxsCurrent = current->lxsNext;
    return current->lxsSvc;
  }
  else
    return NULL;
}

int LXS_ServiceOp(const char * service, const char * method)
{
  char * op, * cmdstring;
  char ** out = NULL, ** err = NULL;
  int rc;

  if (!strcasecmp(method,"startservice"))
    op="start";
  else if (!strcasecmp(method,"stopservice"))
    op="stop";
  else 
    return -1; 
  
  cmdstring = malloc(strlen("/sbin/service") + strlen(service) + strlen(op) +
		     3);
  sprintf(cmdstring,"%s %s %s","/sbin/service",service,op);
  rc = runcommand(cmdstring,NULL,&out,&err);
  freeresultbuf(out);
  freeresultbuf(err);
  free(cmdstring);
  return rc;
}

static LXS_Service *strToSvc(const char * buf) 
{
#define S2SERR() {free(svc); return NULL;}
  LXS_Service *svc;
  char *token, *tokenbuf, *helper, *nl;
  int procnum;
  if (buf==NULL ||strlen(buf)==0)
    return NULL;
  svc=calloc(1,sizeof(LXS_Service));
  /* layout of input lines:
   * servicename(str) status(1|0|-1) processlist
   */
  tokenbuf=strdup(buf);
  token=strtok_r(tokenbuf," ",&helper);
  if (token==NULL)
    S2SERR();
  svc->svName=strdup(token);
  token=strtok_r(NULL," ",&helper);
  if (token==NULL)
    S2SERR();
  switch(atoi(token)) {
  case 1:
    svc->svStarted=1;
    svc->svStatus=strdup("OK");
    break;
  case 0:
    svc->svStarted=0;
    svc->svStatus=strdup("Stopped");
    break;
  default:
    svc->svStarted=0;
    svc->svStatus=strdup("Unknown");
    break;
  }    
  svc->svSystemname=strdup(get_system_name());
  svc->svSystemCCname=strdup("Linux_ComputerSystem");
  svc->svCCname=strdup("Linux_Service");
  svc->svProcesses = calloc(1,sizeof(char*));
  procnum=0;
  while ((token=(strtok_r(NULL," ",&helper)))) {
    procnum+=1;
    svc->svProcesses=realloc(svc->svProcesses,(procnum+1)*sizeof(char*));
    /* strip newlines if necessary */
    if ((nl=strchr(token,'\n'))) *nl=0;
    svc->svProcesses[procnum-1] = strdup(token);
    svc->svProcesses[procnum] = NULL;
  }
  return svc;
}

static void freeSvc(LXS_Service * svc)
{
#define CONDFREE(p) if((p)!=NULL) free((p))
  int i;
  if (svc==NULL) return;
  CONDFREE(svc->svSystemCCname);
  CONDFREE(svc->svSystemname);
  CONDFREE(svc->svCCname);
  CONDFREE(svc->svName);
  CONDFREE(svc->svCaption);
  CONDFREE(svc->svDescription);
  CONDFREE(svc->svInstalldate);
  CONDFREE(svc->svStatus);
  CONDFREE(svc->svStartmode);
  if (svc->svProcesses) {
    for (i=0;svc->svProcesses[i];i++) {
      free(svc->svProcesses[i]);
    }
    free(svc->svProcesses);
  }
  free(svc);
}
