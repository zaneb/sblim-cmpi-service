/*
 * cmpiOSBase_ServiceProcess.c
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
 * association Linux_ServiceProcess.
 * 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmpidt.h"
#include "cmpimacs.h"

#include "cmpiOSBase_Common.h"
#include "OSBase_Common.h"

#include "cmpiOSBase_ServiceProcess.h"


/* ---------------------------------------------------------------------------*/
/* private declarations                                                       */

/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*                            Factory functions                               */
/* ---------------------------------------------------------------------------*/

/* ---------- method to create a CMPIObjectPath of Linux_ServiceProcess ------*/

CMPIObjectPath * _makePath_ServiceProcess( CMPIBroker * _broker, 
					   CMPIObjectPath * service,
					   CMPIObjectPath * process,
					   CMPIStatus * rc) {
  CMPIObjectPath * op = NULL;
  CMPIInstance   * ci = NULL;

  _OSBASE_TRACE(4,("--- _makePath_ServiceProcess() called"));

  ci = _makeInst_ServiceProcess(_broker, service, process, rc);
  if( ci == NULL ) { return NULL; }

  op = CMGetObjectPath(ci,rc); 
  CMSetNameSpace(op,CMGetCharPtr(CMGetNameSpace(service,rc)));

  _OSBASE_TRACE(4,("--- _makePath_ServiceProcess() exited"));
  return op; 
}

/* ----------- method to create a CMPIInstance of Linux_ServiceProcess -------*/

CMPIInstance * _makeInst_ServiceProcess( CMPIBroker * _broker,
					 CMPIObjectPath * service,
					 CMPIObjectPath * process,
					 CMPIStatus * rc) {
  CMPIObjectPath * op = NULL;
  CMPIInstance   * ci = NULL;

  _OSBASE_TRACE(4,("--- _makeInst_ServiceProcess() called"));

  /* create CMPIObjectPath of this association <_ClassName> */
  op = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(service,rc)),
			_ClassName, rc );
  if( CMIsNullObject(op) ) { 
    CMSetStatusWithChars( _broker, rc, 
			  CMPI_RC_ERR_FAILED, "Create CMPIObjectPath failed." ); 
    goto exit;
  }

  ci = CMNewInstance( _broker, op, rc);
  if( CMIsNullObject(ci) ) { 
    CMSetStatusWithChars( _broker, rc, 
			  CMPI_RC_ERR_FAILED, "Create CMPIInstance failed." ); 
    _OSBASE_TRACE(4,("--- _makeInst_ServiceProcess() failed : %s",CMGetCharPtr(rc->msg)));
    goto exit;
  }

  CMSetProperty( ci, _RefLeft, (CMPIValue*)&(service), CMPI_ref ); 
  CMSetProperty( ci, _RefRight, (CMPIValue*)&(process), CMPI_ref );

 exit:
  _OSBASE_TRACE(4,("--- _makeInst_ServiceProcess() exited"));
  return ci; 
}

/* ----- method to handle the association logic for Linux_ServiceProcess -----*/

int assoc_ServiceProcess( CMPIBroker * _broker,
			  CMPIContext * ctx,
			  CMPIResult * rslt,
			  CMPIObjectPath * ref,
			  int assocType,
			  CMPIStatus * rc )
{
  CMPIString     * sourceClass = NULL;
  CMPIData         serviceName;
  CMPIObjectPath * op          = NULL;
  CMPIObjectPath * rop         = NULL;
  CMPIInstance   * rci         = NULL;
  CMPIInstance   * sci         = NULL;
  LXS_Handle       lhdl;
  LXS_Service    * lxssvc      = NULL;
  int              i           = 0;

  _OSBASE_TRACE(2,("--- assoc_ServiceProcess() called"));

  /* check if source instance does exist */
  sci = CBGetInstance(_broker, ctx, ref, NULL, rc);
  if( sci == NULL ) {
    if( rc->rc == CMPI_RC_ERR_FAILED ) {
      CMSetStatusWithChars( _broker, rc,
			    CMPI_RC_ERR_FAILED, "GetInstance of source object failed.");
    }
    if( rc->rc == CMPI_RC_ERR_NOT_FOUND ) {
      CMSetStatusWithChars( _broker, rc,
			    CMPI_RC_ERR_NOT_FOUND, "Source object not found.");
    }
    _OSBASE_TRACE(2,("--- assoc_ServiceProcess() failed : %s",CMGetCharPtr(rc->msg)));
    return -1;
  }

  sourceClass = CMGetClassName(ref, rc);
  if (CMGetCharPtr(sourceClass) == NULL) {
    CMSetStatusWithChars( _broker, rc,
			  CMPI_RC_ERR_FAILED, "Could not get source class name.");
    _OSBASE_TRACE(2,("--- assoc_ServiceProcess() failed : %s",CMGetCharPtr(rc->msg)));
    return -1;
  }

  /* source class is Linux_UnixProcess; target class is Linux_Service */
  if( strcasecmp(CMGetCharPtr(sourceClass),_RefRightClass) == 0 ) {
    /* NOT_SUPPORTED - association is uni-directional */
  }
  /* source class is Linux_Service; target class is Linux_UnixProcess */
  else if( strcasecmp(CMGetCharPtr(sourceClass),_RefLeftClass) == 0 ) {

    serviceName = CMGetKey(ref, "Name", rc);
    if( serviceName.value.string == NULL ) { 
      CMSetStatusWithChars( _broker, rc, 
			    CMPI_RC_ERR_FAILED, "Could not get Name of Service" ); 
      _OSBASE_TRACE(2,("--- assoc_ServiceProcess() failed : %s",CMGetCharPtr(rc->msg)));
      return -1;
    }
    lhdl=LXS_InitFiltered(CMGetCharPtr(serviceName.value.string));
    if (lhdl==NULL) {
      _OSBASE_TRACE(2,("--- assoc_ServiceProcess() exited : Could not find associated Service."));
      return 0;
    }
    lxssvc=LXS_Next(lhdl);
    if (lxssvc==NULL) {
      _OSBASE_TRACE(2,("--- assoc_ServiceProcess() exited : Could not find associated Service."));
      return 0;
    }

    /* get the list of associated processe */
    for (i=0;lxssvc->svProcesses[i]!=NULL;i++) {

      op = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(ref,rc)), 
			    _RefRightClass, rc );
      if( CMIsNullObject(op) ) { 
	CMSetStatusWithChars( _broker, rc, 
			      CMPI_RC_ERR_FAILED, "Create CMPIObjectPath failed." ); 
	_OSBASE_TRACE(2,("--- assoc_ServiceProcess() failed : %s",CMGetCharPtr(rc->msg)));
	return -1;
      }
      CMAddKey(op, "CSCreationClassName", CSCreationClassName, CMPI_chars);
      CMAddKey(op, "CSName", get_system_name(), CMPI_chars);
      CMAddKey(op, "OSCreationClassName", OSCreationClassName, CMPI_chars);
      CMAddKey(op, "OSName", get_os_name(), CMPI_chars);
      CMAddKey(op, "CreationClassName", _RefRightClass, CMPI_chars);
      CMAddKey(op, "Handle", lxssvc->svProcesses[i], CMPI_chars);

      if (assocType&ATYPE_ASSOC) {
	/* the process (name) */
	if (assocType&ATYPE_NAME) {
	  CMReturnObjectPath(rslt, op); 
	}
	else {	  
	  rci = CBGetInstance(_broker, ctx, op, NULL, rc);
	  if(rci==NULL) { return -1; }
	  CMReturnInstance( rslt, rci );
	}
      } 
      else {
	/* the reference object (name) */
	if (assocType&ATYPE_NAME) {
	  rop = _makePath_ServiceProcess(_broker, ref, op, rc);
	  CMReturnObjectPath(rslt, rop); 
	} 
	else {
	  rci = _makeInst_ServiceProcess(_broker, ref, op, rc);
	  CMReturnInstance( rslt, rci );
	}
      }

    }
    LXS_Term(lhdl);
  }

  _OSBASE_TRACE(2,("--- assoc_ServiceProcess() exited"));
  return 0;
}


/* ---------------------------------------------------------------------------*/
/*                     end of cmpiOSBase_ServiceProcess.c                     */
/* ---------------------------------------------------------------------------*/

