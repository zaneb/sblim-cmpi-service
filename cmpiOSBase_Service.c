/*
 * cmpiOSBase_Service.c
 *
 * (C) Copyright IBM Corp. 2002, 2004
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE 
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE 
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
 *
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
 * Contributors: Heidi Neumann <heidineu@de.ibm.com>
 *
 * Interface Type : Common Manageability Programming Interface ( CMPI )
 *
 * Description: 
 * This is the factory implementation for creating instances of CIM
 * class Linux_Service.
 * 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmpidt.h"
#include "cmpimacs.h"

#include "cmpiOSBase_Common.h"
#include "OSBase_Common.h"
#include "cmpiOSBase_Service.h"


/* ---------------------------------------------------------------------------*/
/* private declarations                                                       */

/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*                            Factory functions                               */
/* ---------------------------------------------------------------------------*/

/* ---------- method to create a CMPIObjectPath of Linux_Service -------------*/

CMPIObjectPath * _makePath_Service( const CMPIBroker * _broker,
		      const CMPIContext * ctx, 
	         const CMPIObjectPath * ref,
	         LXS_Service * lxssvc,
	         CMPIStatus * rc) {
  CMPIObjectPath * op = NULL;
 
  _OSBASE_TRACE(2,("--- _makePath_Service() called"));

  /* the sblim-cmpi-base package offers some tool methods to get common
   * system data 
  */
  if( !get_system_name() ) {   
    CMSetStatusWithChars( _broker, rc, 
			  CMPI_RC_ERR_FAILED, "no host name found" );
    _OSBASE_TRACE(2,("--- _makePath_Service() failed : %s",CMGetCharPtr(rc->msg)));
    goto exit;
  }

  op = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(ref,rc)), 
			_ClassName, rc );
  if( CMIsNullObject(op) ) { 
    CMSetStatusWithChars( _broker, rc, 
			  CMPI_RC_ERR_FAILED, "Create CMPIObjectPath failed." ); 
    _OSBASE_TRACE(2,("--- _makePath_Service() failed : %s",CMGetCharPtr(rc->msg)));
    goto exit; 
  }

  CMAddKey(op, "SystemCreationClassName", CSCreationClassName, CMPI_chars);
  CMAddKey(op, "SystemName", get_system_name(), CMPI_chars);
  CMAddKey(op, "CreationClassName", _ClassName, CMPI_chars);
  CMAddKey(op, "Name", lxssvc->svName, CMPI_chars);

 exit:
  _OSBASE_TRACE(2,("--- _makePath_Service() exited"));
  return op;             
}

/* ----------- method to create a CMPIInstance of Linux_Service --------------*/

CMPIInstance * _makeInst_Service( const CMPIBroker * _broker,
	       const CMPIContext * ctx, 
	       const CMPIObjectPath * ref,
	       const char ** properties,
	       LXS_Service * lxssvc,
	       CMPIStatus * rc) {
  CMPIObjectPath *  op        = NULL;
  CMPIInstance   *  ci        = NULL;
  const char     ** keys      = NULL;
  int               keyCount  = 0;

  _OSBASE_TRACE(2,("--- _makeInst_Service() called"));

  /* the sblim-cmpi-base package offers some tool methods to get common
   * system data
  */
  if( !get_system_name() ) {   
    CMSetStatusWithChars( _broker, rc, 
			  CMPI_RC_ERR_FAILED, "no host name found" );
    _OSBASE_TRACE(2,("--- _makeInst_Service() failed : %s",CMGetCharPtr(rc->msg)));
    goto exit;
  }

  op = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(ref,rc)), 
			_ClassName, rc );
  if( CMIsNullObject(op) ) { 
    CMSetStatusWithChars( _broker, rc, 
			  CMPI_RC_ERR_FAILED, "Create CMPIObjectPath failed." ); 
    _OSBASE_TRACE(2,("--- _makeInst_Service() failed : %s",CMGetCharPtr(rc->msg)));
    goto exit; 
  }

  ci = CMNewInstance( _broker, op, rc);
  if( CMIsNullObject(ci) ) { 
    CMSetStatusWithChars( _broker, rc, 
			  CMPI_RC_ERR_FAILED, "Create CMPIInstance failed." ); 
    _OSBASE_TRACE(2,("--- _makeInst_Service() failed : %s",CMGetCharPtr(rc->msg)));
    goto exit; 
  }

  /* set property filter */
  keys = calloc(5,sizeof(char*));
  keys[0] = strdup("SystemCreationClassName");
  keys[1] = strdup("SystemName");
  keys[2] = strdup("CreationClassName");
  keys[3] = strdup("Name");
  CMSetPropertyFilter(ci,properties,keys);
  for( ;keys[keyCount]!=NULL;keyCount++) { free((char*)keys[keyCount]); }
  free(keys);

  CMSetProperty( ci, "SystemCreationClassName", CSCreationClassName, CMPI_chars );
  CMSetProperty( ci, "SystemName", get_system_name(), CMPI_chars );
  CMSetProperty( ci, "CreationClassName", _ClassName, CMPI_chars );
  CMSetProperty( ci, "Name", lxssvc->svName, CMPI_chars );
  CMSetProperty( ci, "Caption", "Linux (/etc/init.d) Service", CMPI_chars);
  CMSetProperty( ci, "Description", "Instances of this class represents the /etc/init.d managed Services.", CMPI_chars);

  CMSetProperty( ci, "Status", lxssvc->svStatus, CMPI_chars);
  CMSetProperty( ci, "Started", (CMPIValue*)&(lxssvc->svStarted), CMPI_boolean);

 exit:
  _OSBASE_TRACE(2,("--- _makeInst_Service() exited"));
  return ci;
}


/* ---------------------------------------------------------------------------*/

void serviceDup(LXS_Service *old, LXS_Service *new)
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

int serviceChanged(LXS_Service *old, LXS_Service *new)
{
  return strcmp(old->svStatus,new->svStatus) ||
    old->svStarted != new->svStarted;
}


/* ---------------------------------------------------------------------------*/
/*                     end of cmpiOSBase_Service.c                            */
/* ---------------------------------------------------------------------------*/
