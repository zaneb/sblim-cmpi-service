/*
 * Linux_Service.c
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
 * Description: This is the instance provider for services.
 */

#define INSTANCE_PROVIDER
#define ASSOCIATOR_PROVIDER
#define METHOD_PROVIDER
#ifndef NOEVENTS
#define EVENT_PROVIDER
#endif

#include <cimibase.h>
#include <cimlxs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "provider.h"
#include "npi_import.h"

PROVIDER_NAME(Linux_Service)

static CIMInstance makeService(NPIHandle *, LXS_Service *, const char *); 
static CIMInstance makeHostedService(NPIHandle *, LXS_Service *, 
				     CIMObjectPath, const char *); 
static CIMInstance  makeServiceProcess(NPIHandle *, LXS_Service *, 
				       CIMObjectPath, const char *);
static Vector fakeProcessList(NPIHandle *nh,LXS_Service *lxssvc,
			      const char *namespace);
     
static Vector assocHelper(NPIHandle *, CIMObjectPath, 
			  CIMObjectPath, int);

#ifndef NOEVENTS
static pthread_t tid;
static int threadActive=0;
static int threadCancelPending=0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int checkProviderThread();
static void startProviderThread(NPIHandle *);
static void stopProviderThread();
static void * statusLurker(void *);
#endif

static LXS_Service * createReferenceList();
static void freeReferenceList(LXS_Service*);
static void serviceDup(LXS_Service*,LXS_Service*);
static int serviceChanged(LXS_Service*,LXS_Service*);

#define ATYPE_NONE    0x00000000
#define ATYPE_ASSOC   0x00000001
#define ATYPE_NAME    0x00000002

#define RTYPE_NONE    0x00000000
#define RTYPE_SERVICE 0x00000001 
#define RTYPE_PROCESS 0x00000002
#define RTYPE_UCS     0x00000004
#define RTYPE_ALL     0x00000007

static void initialize (NPIHandle *nh, CIMOMHandle ch)
{
  fprintf(stderr,"Linux_Service: initializing\n");
#ifndef NOEVENTS
  pthread_mutex_init(&mutex,NULL);
#endif
}

static void cleanup (NPIHandle *nh) 
{
#ifndef NOEVENTS
  pthread_mutex_destroy(&mutex);
#endif
}

static Vector enumInstanceNames(NPIHandle *nh,CIMObjectPath cop,
				int bool_deep,CIMClass cc)
{
  Vector        vec;
  CIMInstance   ci;
  CIMObjectPath co;
  LXS_Handle    lhdl;
  LXS_Service  *lxssvc;
  char         *namespace=NULL, *clsname=NULL;

  fprintf(stderr,"Linux_Service: enumerate instance names\n");
  vec = VectorNew(nh);
  namespace=CIMObjectPathGetNameSpace(nh,cop);
  clsname=CIMObjectPathGetClassName(nh,cop);

  if (errorCheck(nh)) {
      return vec;
  }
  
  if (strcasecmp(clsname,"linux_service") &&
      strcasecmp(clsname,"cim_service") &&
      strcasecmp(clsname,"cim_logicalelement") &&
      strcasecmp(clsname,"cim_managedsystemelement") &&
      strcasecmp(clsname,"cim_managedelement") )
    return vec;
  
  /* init service listing */
  lhdl=LXS_Init();
  if (lhdl==NULL) {
    raiseError(nh,"Couldn't list services.");
    return vec;
  }

  /* loop through services */
  while ((lxssvc=LXS_Next(lhdl))) {
    ci=makeService(nh,lxssvc,namespace);
    co=CIMObjectPathFromCIMInstance(nh,ci);
    if (errorCheck(nh)) {
      /* Error in object path -> return */
      return VectorNew(nh);
    }
    VectorAddTo(nh,vec,co);
  } 
  
  /* terminate service listing */
  LXS_Term(lhdl);
   if (namespace) free(namespace);
   if (clsname) free(clsname);
  return vec;
}

static Vector enumInstances(NPIHandle *nh,CIMObjectPath cop,
			    int bool_deep,CIMClass cc,
			    int bool_localOnly) 
{
  Vector        vec;
  CIMInstance   ci;
  LXS_Handle    lhdl;
  LXS_Service  *lxssvc;
  char         *namespace=NULL, *clsname=NULL;

  fprintf(stderr,"Linux_Service: enumerate instance names\n");
  vec = VectorNew(nh);
  namespace=CIMObjectPathGetNameSpace(nh,cop);
  clsname=CIMObjectPathGetClassName(nh,cop);

  if (errorCheck(nh)) {
      return vec;
  }

  if (strcasecmp(clsname,"linux_service") &&
      strcasecmp(clsname,"cim_service") &&
      strcasecmp(clsname,"cim_logicalelement") &&
      strcasecmp(clsname,"cim_managedsystemelement") &&
      strcasecmp(clsname,"cim_managedelement") )
    return vec;
  
  /* init service listing */
  lhdl=LXS_Init();
  if (lhdl==NULL) {
    raiseError(nh,"Couldn't list services.");
    return vec;
  }

  /* loop through services */
  while ((lxssvc=LXS_Next(lhdl))) {
    ci=makeService(nh,lxssvc,namespace);
    if (errorCheck(nh)) {
      /* Error in object path -> return */
      return VectorNew(nh);
    }
    VectorAddTo(nh,vec,ci);
  } 
  
  /* terminate service listing */
  LXS_Term(lhdl);
  if (namespace) free(namespace);
  if (clsname) free(clsname);
  return vec;
}

static CIMInstance getInstance(NPIHandle *nh,CIMObjectPath cop,CIMClass cc,
			       int bool_localOnly) 
{
  CIMInstance  ci;
  LXS_Handle   lhdl;
  LXS_Service *lxssvc;
  char        *spec; 
  char        *namespace;

  spec = CIMObjectPathGetStringKeyValue(nh,cop,"name"); 
  namespace = CIMObjectPathGetNameSpace(nh,cop);
  
  if (errorCheck(nh)) {
    fprintf(stderr,"error accessing object path\n");
    return ci;
  }
  
  lhdl=LXS_InitFiltered(spec);
  if (lhdl==NULL) {
    raiseError(nh,"Couldn't list services.");
    /* would neet a call to invalidate objects , ci in this case*/
    return ci;
  }

  /* only looking for one - the first - match */
  lxssvc=LXS_Next(lhdl);
  if (lxssvc != NULL)
    ci=makeService(nh,lxssvc,namespace);
  else
    raiseError(nh,"specified service instance not found");

  LXS_Term(lhdl);
  
  if (spec) free(spec);
  if (namespace) free(namespace);
  return ci;
}

static CIMObjectPath createInstance(NPIHandle *nh,CIMObjectPath cop,
				    CIMInstance ci) 
{
  raiseError(nh,"create not supported");
  return CIMObjectPathFromCIMInstance (nh,ci);
}

static void setInstance(NPIHandle *nh,CIMObjectPath cop,CIMInstance ci) 
{
  raiseError(nh,"set not supported");
}

static void deleteInstance(NPIHandle *nh,CIMObjectPath cop) 
{
  raiseError(nh,"delete not supported");
}

static Vector execQuery(NPIHandle *nh,CIMObjectPath cop,
			const char * str,int i,CIMClass cc)
{
  return VectorNew(nh);
}

static Vector associators ( NPIHandle *nh, CIMObjectPath assoc, 
			    CIMObjectPath path,
			    const char *resultClass, 
			    const char *role, const char *resultRole,
			    int inclQ, int inclCO, 
			    const char *propertyList[], int plLen)
{
  return assocHelper(nh,assoc,path,ATYPE_ASSOC);
}

static Vector associatorNames ( NPIHandle *nh, CIMObjectPath assoc, 
				CIMObjectPath path,
				const char *resultClass, 
				const char *role, const char *resultRole)
{
  return assocHelper(nh,assoc,path,ATYPE_ASSOC|ATYPE_NAME);
}

static Vector references ( NPIHandle *nh, CIMObjectPath assoc, 
			   CIMObjectPath path, const char *role, 
			   int inclQ, int inclCO, 
			   const char *propertyList [], int plLen)
{
  return assocHelper(nh,assoc,path,0);
}

static Vector referenceNames ( NPIHandle *nh, CIMObjectPath assoc, 
			       CIMObjectPath path, 
			       const char * role)
{
  return assocHelper(nh,assoc,path,ATYPE_NAME);
}

static CIMValue invokeMethod ( NPIHandle * nh, CIMObjectPath cop, 
			       const char *method, Vector in, Vector out)
{
#undef  FREESTUFF  
#define FREESTUFF() free(clsname), \
                    free(servicename)
  CIMValue val;
  char *clsname=NULL, *servicename=NULL;
  int rc;

  clsname=CIMObjectPathGetClassName(nh,cop);
  servicename=CIMObjectPathGetStringKeyValue(nh,cop,"name");
  if (errorCheck(nh)) {
    FREESTUFF();
    return val;
  }
  if (strcasecmp(clsname,"linux_service") || servicename==NULL) {
    raiseError(nh,"invalid class/service name");
    return val;
  }
  
  rc = LXS_ServiceOp(servicename, method);
  val = CIMValueNewInteger(nh,rc);
  return val;
}

#ifndef NOEVENTS
static void authorizeFilter(NPIHandle* nh,SelectExp se, const char* type,
			    CIMObjectPath co, const char* owner)
{
  fprintf(stderr,"Linux_Service: authorize filter\n");
  /* we don't object */
}

static int mustPoll(NPIHandle* nh,SelectExp se, const char* type,
		    CIMObjectPath co)
{
  return 0; /* no polling */
}

static void activateFilter(NPIHandle* nh,SelectExp se, const char* type,
			   CIMObjectPath co, int first)
{
  fprintf(stderr,"Linux_Service: activate filter for %s\n", type);
  if (checkProviderThread())
    return; /* we are already running */
  if (strcasecmp(type,"cim_instmodification")==0)
    startProviderThread(nh);
}

static void deActivateFilter(NPIHandle* nh,SelectExp se, const char* type,
			     CIMObjectPath co, int last)
{
  fprintf(stderr,"Linux_Service: deactivate filter\n");
  if (last && checkProviderThread())
    stopProviderThread();
}
#endif

/* private stuff */

static Vector assocHelper(NPIHandle *nh, CIMObjectPath assoc, 
			  CIMObjectPath path, int assocType)
{
#undef  FREESTUFF  
#define FREESTUFF() free(clsname), \
                    free(namespace), \
                    free(assocname), \
                    free(servicename)

  char   *clsname=NULL, *namespace=NULL;
  char   *servicename=NULL, *assocname=NULL;
  Vector result, enumresult;
  LXS_Handle lhdl;
  LXS_Service *lxssvc;
  CIMInstance ci;
  CIMObjectPath co;
  int resulttype, assocfilter, i;
  
  clsname=CIMObjectPathGetClassName(nh,path);
  if (!ObjectIsNull(assoc))
    assocname=CIMObjectPathGetClassName(nh,assoc);

  result=VectorNew(nh);
  if(errorCheck(nh)) {
    FREESTUFF();
    return result;
  }

  if (clsname == NULL) { 
    raiseError(nh,"Couldn't get class name");
    return result;
  }

  if (!strcasecmp(clsname,"linux_service") ||
      !strcasecmp(clsname,"cim_service")) {
    resulttype=RTYPE_UCS|RTYPE_PROCESS;
    servicename=CIMObjectPathGetStringKeyValue(nh,path,"name");
  }
  else if (!strcasecmp(clsname,"linux_unitarycomputersystem")
	   ||!strcasecmp(clsname,"cim_unitarycomputersystem")
	   ||!strcasecmp(clsname,"cim_computersystem")
	   ||!strcasecmp(clsname,"cim_system"))
    resulttype = RTYPE_SERVICE;
  else
    resulttype = RTYPE_NONE;

  if (assocname == NULL)
    assocfilter = RTYPE_ALL;
  else if (!strcasecmp(assocname,"linux_serviceprocess") ||
	   !strcasecmp(assocname,"cim_serviceprocess"))
    assocfilter = RTYPE_PROCESS | RTYPE_SERVICE;
  else if (!strcasecmp(assocname,"linux_hostedservice") ||
	   !strcasecmp(assocname,"cim_hostedservice"))
    assocfilter = RTYPE_SERVICE | RTYPE_UCS;
  resulttype &= assocfilter;

  namespace=CIMObjectPathGetNameSpace(nh,path);
  /* check for "not set" */ 
  if (namespace==NULL || strchr(namespace,'$')) namespace=strdup("root/cimv2");
  
  /* process the services */
  if (resulttype&RTYPE_SERVICE) {
    lhdl=LXS_Init();
    if (lhdl==NULL) {
      raiseError(nh,"Couldn't list associated services.");
      FREESTUFF();
      return result;
    }
    while ((lxssvc=LXS_Next(lhdl))) {
      if (assocType&ATYPE_ASSOC) {
	ci=makeService(nh,lxssvc,namespace);
      } else {
	ci=makeHostedService(nh,lxssvc,path,namespace);
      }
      if (assocType&ATYPE_NAME) {
	co=CIMObjectPathFromCIMInstance(nh,ci);
	VectorAddTo(nh,result,co);
      } else {
	VectorAddTo(nh,result,ci);
      }
    }
    LXS_Term(lhdl);
  }
  /* process the unitary computer system */
  if (resulttype&RTYPE_UCS) {
    /* get the one (and ONLY) instance of unitary computer system */
    co=CIMObjectPathNew(nh,"linux_unitarycomputersystem");
    CIMObjectPathSetNameSpace(nh,co,namespace);
    enumresult = CIMOMEnumInstances(nh,co, 1,0);
    VectorGet(nh,enumresult,0,ci);          /* UCS instance */
    co=CIMObjectPathFromCIMInstance(nh,ci); /* path of UCS instance */
    if (errorCheck(nh)) {
      fprintf(stderr,"couldn't get UCS\n");
      FREESTUFF();
      return result;
    }
    if (!(assocType&ATYPE_ASSOC)) {
      /* find the required service */
      lhdl=LXS_InitFiltered(servicename);
      if (lhdl==NULL) {
	raiseError(nh,"Couldn't find associated service");
	FREESTUFF();
	return result;
      }
      lxssvc=LXS_Next(lhdl);
      if (lxssvc==NULL) {
	raiseError(nh,"Couldn't find associated service");
	FREESTUFF();
	return result;
      }
      ci=makeHostedService(nh,lxssvc,co,namespace); /* the reference */
      LXS_Term(lhdl);
    }
    if (assocType&ATYPE_NAME) {
      co=CIMObjectPathFromCIMInstance(nh,ci);
      VectorAddTo(nh,result,co);
    } else {
      VectorAddTo(nh,result,ci);
    }
  }
    /* process the processes */
  if (resulttype&RTYPE_PROCESS) {
    /* obtain the service, we will need it */
    lhdl=LXS_InitFiltered(servicename);
    if (lhdl==NULL) {
      raiseError(nh,"Couldn't find associated service");
      FREESTUFF();
      return result;
    }
    lxssvc=LXS_Next(lhdl);
    if (lxssvc==NULL) {
      raiseError(nh,"Couldn't find associated service");
      FREESTUFF();
      return result;
    }
    /* get the list of associated processe */
    enumresult=fakeProcessList(nh,lxssvc,namespace);
    for (i=0; i < VectorSize(nh,enumresult); i++) {
      VectorGet(nh,enumresult,i,co); /* get process path */
      if (assocType&ATYPE_ASSOC) {
	/* the process (name) */
	if (assocType&ATYPE_NAME)
	  VectorAddTo(nh,result,co);
	else {
	  ci=CIMOMGetInstance(nh,co,0);
	  if (errorCheck(nh)) {
	    fprintf(stderr,"couldn't get process instance\n");
	    FREESTUFF();
	    return result;
	  }
	  VectorAddTo(nh,result,ci);
	}
      } else {
	/* the reference object (name) */
	ci=makeServiceProcess(nh,lxssvc,co,namespace);
	if (assocType&ATYPE_NAME) {
	  co=CIMObjectPathFromCIMInstance(nh,ci);
	  if (errorCheck(nh)) {
	    FREESTUFF();
	    return result;
	  }
	  VectorAddTo(nh,result,co);
	} else
	  VectorAddTo(nh,result,ci);
      }
    }
    LXS_Term(lhdl);
  }
  
  FREESTUFF();
  return result;
}

static CIMInstance makeService(NPIHandle * nh, LXS_Service * lxssvc, 
			       const char * namespace) 
{
  CIMObjectPath co;
  CIMClass cc;
  CIMInstance ci;
  co=CIMObjectPathNew(nh,"linux_service");
  CIMObjectPathSetNameSpace(nh,co,namespace);
  cc=CIMOMGetClass(nh,co,0);
  if (errorCheck(nh)) 
    return ci;
  ci=CIMClassNewInstance(nh,cc);
  if (errorCheck(nh)) 
    return ci;
  CIMInstanceSetStringProperty(nh,ci,"Name",lxssvc->svName);
  CIMInstanceSetStringProperty(nh,ci,"CreationClassName",lxssvc->svCCname);
  CIMInstanceSetStringProperty(nh,ci,"SystemName",lxssvc->svSystemname);
  CIMInstanceSetStringProperty(nh,ci,"SystemCreationClassName",
			       lxssvc->svSystemCCname);
  CIMInstanceSetBooleanProperty(nh,ci,"Started",lxssvc->svStarted);
  CIMInstanceSetStringProperty(nh,ci,"Status",lxssvc->svStatus);
  return ci;
}

static CIMInstance makeHostedService(NPIHandle * nh,LXS_Service * lxssvc,
				     CIMObjectPath cop,
				     const char * namespace) 
{
  CIMObjectPath co;
  CIMClass cc;
  CIMInstance ci, cis;
  co=CIMObjectPathNew(nh,"linux_hostedservice");
  CIMObjectPathSetNameSpace(nh,co,namespace);
  cc=CIMOMGetClass(nh,co,0);
  if (errorCheck(nh)) 
    return ci;
  ci=CIMClassNewInstance(nh,cc);
  if (errorCheck(nh)) 
    return ci;
  cis=makeService(nh,lxssvc,namespace);
  if (errorCheck(nh)) 
    return ci;
  CIMInstanceSetRefProperty(nh,ci,"antecedent",cop);
  CIMInstanceSetRefProperty(nh,ci,"dependent",
			    CIMObjectPathFromCIMInstance(nh,cis));
  return ci;
}

static CIMInstance makeServiceProcess(NPIHandle *nh, LXS_Service *lxssvc, 
				      CIMObjectPath cop,
				      const char *namespace)
{
  CIMObjectPath co;
  CIMClass cc;
  CIMInstance ci, cis;
  
  co=CIMObjectPathNew(nh,"linux_serviceprocess");
  CIMObjectPathSetNameSpace(nh,co,namespace);
  cc=CIMOMGetClass(nh,co,0);
  if (errorCheck(nh)) 
    return ci;
  ci=CIMClassNewInstance(nh,cc);
  if (errorCheck(nh)) 
    return ci;
   cis=makeService(nh,lxssvc,namespace);
  if (errorCheck(nh)) 
    return ci;
  
  CIMInstanceSetRefProperty(nh,ci,"process",cop);
  CIMInstanceSetRefProperty(nh,ci,"service",
			    CIMObjectPathFromCIMInstance(nh,cis));
  return ci;
}

static Vector fakeProcessList(NPIHandle *nh,LXS_Service *lxssvc,
			      const char *namespace)
{
  /* construct a fake process from the services pid list */
  CIMObjectPath co;
  CIMClass cc;
  CIMInstance ci;
  Vector result;
  int i;

  result=VectorNew(nh);
  co=CIMObjectPathNew(nh,"linux_process");
  CIMObjectPathSetNameSpace(nh,co,namespace);
  cc=CIMOMGetClass(nh,co,0);
  if (errorCheck(nh)) 
    return result;
  for (i=0;lxssvc->svProcesses[i]!=NULL;i++) {
    ci=CIMClassNewInstance(nh,cc);
    if (errorCheck(nh)) 
      return result;
    CIMInstanceSetStringProperty(nh,ci,"Handle",lxssvc->svProcesses[i]);
    CIMInstanceSetStringProperty(nh,ci,"CreationClassName","Linux_Process");
    CIMInstanceSetStringProperty(nh,ci,"CSName",lxssvc->svSystemname);
    CIMInstanceSetStringProperty(nh,ci,"CSCreationClassName",
				 lxssvc->svSystemCCname);
    CIMInstanceSetStringProperty(nh,ci,"OSName",get_os_name());
    CIMInstanceSetStringProperty(nh,ci,"OSCreationClassName",
				 "Linux_OperatingSystem");
    co=CIMObjectPathFromCIMInstance(nh,ci);
    CIMObjectPathSetNameSpace(nh,co,namespace); /* why ? */
    if (errorCheck(nh)) 
      return result;
    VectorAddTo(nh,result,co);
  }
  return result;
}

#ifndef NOEVENTS
static int checkProviderThread()
{
  int status;
  if (pthread_mutex_lock(&mutex)) {
    fprintf(stderr,"Linux_Service: couldn't obtain mutex lock");
    return -1;
  }
  status = threadActive;
  pthread_mutex_unlock(&mutex);
  return status;
}

static void startProviderThread(NPIHandle* nh)
{
  NPIHandle* tnh;
  if (pthread_mutex_lock(&mutex)) {
    fprintf(stderr,"Linux_Service: couldn't obtain mutex lock");
    return;
  }
  if (threadActive==0) {
    fprintf(stderr,"Linux_Service: starting status thread\n");
    tnh=CIMOMPrepareAttach(nh);
    if (errorCheck(tnh))
      CIMOMCancelAttach(tnh);
    else if (pthread_create(&tid,NULL,&statusLurker,tnh)==0)
      threadActive=1;
  }
  pthread_mutex_unlock(&mutex);
}

static void stopProviderThread()
{
  if (pthread_mutex_lock(&mutex)) {
    fprintf(stderr,"Linux_Service: couldn't obtain mutex lock");
    return;
  }
  if (threadActive) {
    fprintf(stderr,"Linux_Service: stopping status thread\n");
    threadActive=0;
    threadCancelPending+=1;
  }
  pthread_mutex_unlock(&mutex);
}

static void * statusLurker(void * args)
{
  int cancelstatus=0;
  NPIHandle* nh=args;

  LXS_Handle    lhdl;
  LXS_Service  *lxssvc, *svcbuf=NULL;
  CIMInstance ciprev,cisrc,cimod;
  CIMObjectPath cop;
  CIMClass cc;
  int svcindex;

  CIMOMAttachThread(nh);
  fprintf(stderr,"Linux_Service: status thread starting\n");
  while(!cancelstatus) { 
    fprintf(stderr,"Linux_Service: status thread blip\n");
    /* Get Initial Service Listing */
    if (svcbuf==NULL)
      svcbuf=createReferenceList();
    sleep(20);
    cop=CIMObjectPathNew(nh,"cim_instmodification");
    CIMObjectPathSetNameSpace(nh,cop,"root/cimv2");
    cc=CIMOMGetClass(nh,cop,0);
    if (errorCheck(nh)) {
      fprintf(stderr,"Linux_Service: couldn't get indication class\n");
      stopProviderThread();
    } else {
      cimod=CIMClassNewInstance(nh,cc);
      
      lhdl=LXS_Init();
      svcindex=0;
      if (lhdl) {
	/* lookout for modified ones */
	while((lxssvc=LXS_Next(lhdl))) {
	  if (svcbuf == NULL || 
	      svcbuf[svcindex].svName==NULL ||
	      strcmp(svcbuf[svcindex].svName,lxssvc->svName)) {
	    fprintf(stderr,"Linux_Service: previous list out of sync\n");
	    freeReferenceList(svcbuf);
	    svcbuf=NULL;
	    break; /* rebuild initial services list */
	  }
	  if (serviceChanged(&svcbuf[svcindex],lxssvc)) {
	    ciprev=makeService(nh,&svcbuf[svcindex],"root/cimv2");
	    cisrc=makeService(nh,lxssvc,"root/cimv2");
	    serviceDup(&svcbuf[svcindex],lxssvc);
	    CIMOMDeliverInstanceEvent(nh,"root/cimv2",cimod,cisrc,ciprev);
	    if (errorCheck(nh)) {
	      fprintf(stderr,"Linux_Service: indication not delivered\n");
	      /* we do not exit here - could be a transient problem */
	      errorReset(nh);
	    }
	  }
	  svcindex+=1;
	}
	LXS_Term(lhdl);
      } 
    }
    if (pthread_mutex_lock(&mutex)) {
      fprintf(stderr,"Linux_Service: couldn't obtain mutex lock");    
      break;
    }
    if (threadCancelPending) {
      cancelstatus=1;
      threadCancelPending-=1;
    }
    pthread_mutex_unlock(&mutex);
  }
  fprintf(stderr,"Linux_Service: status thread terminating\n");
  CIMOMDetachThread(nh);
  return NULL;
}
#endif

static LXS_Service * createReferenceList()
{
  LXS_Handle lhdl;
  LXS_Service *lxssvc, *reflist=NULL;
  int index=0;

  lhdl=LXS_Init();
  if (!lhdl)
    return NULL;
  while((lxssvc=LXS_Next(lhdl))) {
    reflist=realloc(reflist,sizeof(LXS_Service)*(index+1));
    memset(&reflist[index],0,sizeof(LXS_Service));
    serviceDup(&reflist[index],lxssvc);
    reflist[index].svStarted=2; /* little trick to force initial event */
    index+=1;
  }
  LXS_Term(lhdl);
  return reflist;
}

static void freeReferenceList(LXS_Service *list)
{
  LXS_Service *reflist=list;

  if (list==NULL)
    return;
  
  while(list->svName) {
    free(list->svName);
    free(list->svCCname);
    free(list->svSystemname);
    free(list->svSystemCCname);
    free(list->svStatus);
    list+=1;
  }
  free(reflist);
}

static void serviceDup(LXS_Service *old, LXS_Service *new)
{
  if (old==NULL || new==NULL)
    return;
  
  if (old->svName) {
    free(old->svName);
    free(old->svCCname);
    free(old->svSystemname);
    free(old->svSystemCCname);
    free(old->svStatus);
  }
  old->svName=strdup(new->svName);
  old->svCCname=strdup(new->svCCname);
  old->svSystemname=strdup(new->svSystemname);
  old->svSystemCCname=strdup(new->svSystemCCname);
  old->svStatus=strdup(new->svStatus);
  old->svStarted=new->svStarted;
}

static int serviceChanged(LXS_Service *old, LXS_Service *new)
{
  return strcmp(old->svStatus,new->svStatus) ||
    old->svStarted != new->svStarted;
}
