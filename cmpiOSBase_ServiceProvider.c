/*
 * cmpiOSBase_ServiceProvider.c
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
 * Description: This is the instance provider for services.
 * 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include <cimlxs.h>
#include "OSBase_Common.h"
#include "cmpiOSBase_Common.h"
#include "cmpiOSBase_Service.h"

static CMPIBroker * _broker;

/* ---------------------------------------------------------------------------*/
/* private declarations                                                       */

/* ---------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/*                      Instance Provider Interface                           */
/* ---------------------------------------------------------------------------*/


CMPIStatus OSBase_ServiceProviderCleanup( CMPIInstanceMI * mi, 
           CMPIContext * ctx) { 
  _OSBASE_TRACE(1,("--- %s CMPI Cleanup() called",_ClassName));
  _OSBASE_TRACE(1,("--- %s CMPI Cleanup() exited",_ClassName));
  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_ServiceProviderEnumInstanceNames( CMPIInstanceMI * mi, 
           CMPIContext * ctx, 
           CMPIResult * rslt, 
           CMPIObjectPath * ref) { 
  CMPIObjectPath * op      = NULL;
  CMPIString     * clsname = NULL;
  LXS_Handle       lhdl;
  LXS_Service    * lxssvc  = NULL;
  CMPIStatus       rc      = {CMPI_RC_OK, NULL};
  
  _OSBASE_TRACE(1,("--- %s CMPI EnumInstanceNames() called",_ClassName));

  clsname = CMGetClassName(ref, &rc);
  if( strcasecmp(CMGetCharPtr(clsname), _ClassName ) != 0 &&
      CMClassPathIsA(_broker,ref,_ClassName,&rc) == 0 ) {
    return rc;
  }

  /* init service listing */
  lhdl=LXS_Init();
  if (lhdl==NULL) {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Could not list services." ); 
    _OSBASE_TRACE(1,("--- %s CMPI EnumInstanceNames() failed : %s",_ClassName,CMGetCharPtr(rc.msg)));
    return rc;
  }

  /* loop through services */
  while ((lxssvc=LXS_Next(lhdl))) {
    op = _makePath_Service( _broker, ctx, ref, lxssvc, &rc );
    if( op == NULL ) {
      if( rc.msg != NULL )
	{ _OSBASE_TRACE(1,("--- %s CMPI EnumInstanceNames() failed : %s",_ClassName,CMGetCharPtr(rc.msg))); }
      else
	{ _OSBASE_TRACE(1,("--- %s CMPI EnumInstanceNames() failed",_ClassName)); }
      LXS_Term(lhdl);
      return rc;
    }
    CMReturnObjectPath(rslt, op);
  }
  
  /* terminate service listing */
  LXS_Term(lhdl);
  CMReturnDone(rslt);
  _OSBASE_TRACE(1,("--- %s CMPI EnumInstanceNames() exited",_ClassName));
  return rc;
}

CMPIStatus OSBase_ServiceProviderEnumInstances( CMPIInstanceMI * mi, 
           CMPIContext * ctx, 
           CMPIResult * rslt, 
           CMPIObjectPath * ref, 
           char ** properties) { 
  CMPIInstance * ci      = NULL; 
  CMPIString   * clsname = NULL; 
  LXS_Handle     lhdl;
  LXS_Service  * lxssvc  = NULL;
  CMPIStatus     rc      = {CMPI_RC_OK, NULL};

  _OSBASE_TRACE(1,("--- %s CMPI EnumInstances() called",_ClassName));
  
  clsname = CMGetClassName(ref, &rc);
  if( strcasecmp(CMGetCharPtr(clsname), _ClassName ) != 0 &&
      CMClassPathIsA(_broker,ref,_ClassName,&rc) == 0 ) {
    return rc;
  }
  
  /* init service listing */
  lhdl=LXS_Init();
  if (lhdl==NULL) {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Could not list services." ); 
    _OSBASE_TRACE(1,("--- %s CMPI EnumInstances() failed : %s",_ClassName,CMGetCharPtr(rc.msg)));
    return rc;
  }

  /* loop through services */
  while ((lxssvc=LXS_Next(lhdl))) {
    ci = _makeInst_Service( _broker, ctx, ref, (const char**)properties, lxssvc, &rc );
    if( ci == NULL ) {
      if( rc.msg != NULL )
	{ _OSBASE_TRACE(1,("--- %s CMPI EnumInstances() failed : %s",_ClassName,CMGetCharPtr(rc.msg))); }
      else
	{ _OSBASE_TRACE(1,("--- %s CMPI EnumInstances() failed",_ClassName)); }
      LXS_Term(lhdl);
      return rc;
    }
    CMReturnInstance( rslt, ci );
  } 
  
  /* terminate service listing */
  LXS_Term(lhdl);
  CMReturnDone( rslt );
  _OSBASE_TRACE(1,("--- %s CMPI EnumInstances() exited",_ClassName));
  return rc;
}

CMPIStatus OSBase_ServiceProviderGetInstance( CMPIInstanceMI * mi, 
           CMPIContext * ctx, 
           CMPIResult * rslt, 
           CMPIObjectPath * cop, 
           char ** properties) {
  CMPIInstance * ci     = NULL;
  CMPIString   * spec   = NULL;
  LXS_Handle     lhdl;
  LXS_Service  * lxssvc = NULL;
  CMPIStatus     rc     = {CMPI_RC_OK, NULL};

  _OSBASE_TRACE(1,("--- %s CMPI GetInstance() called",_ClassName));

  _check_system_key_value_pairs( _broker, cop, "SystemCreationClassName", "SystemName", &rc );
  if( rc.rc != CMPI_RC_OK ) { 
    _OSBASE_TRACE(1,("--- %s CMPI GetInstance() failed : %s",_ClassName,CMGetCharPtr(rc.msg)));
    return rc; 
  }
  
  spec = CMGetKey( cop, "Name", &rc).value.string;
  if( CMGetCharPtr(spec) == NULL ) {    
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Could not get Service Name." ); 
    _OSBASE_TRACE(1,("--- %s CMPI GetInstance() failed : %s",_ClassName,CMGetCharPtr(rc.msg)));
    return rc;
  }

  lhdl=LXS_InitFiltered(CMGetCharPtr(spec));
  if (lhdl==NULL) {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_FAILED, "Could not list services." ); 
    _OSBASE_TRACE(1,("--- %s CMPI GetInstance() failed : %s",_ClassName,CMGetCharPtr(rc.msg)));
    return rc;
  }

  /* only looking for one - the first - match */
  lxssvc=LXS_Next(lhdl);
  if (lxssvc != NULL) {
    ci = _makeInst_Service( _broker, ctx, cop, (const char**)properties, lxssvc, &rc );
  }
  else {
    CMSetStatusWithChars( _broker, &rc, 
			  CMPI_RC_ERR_NOT_FOUND, "Specified Service Instance not found." ); 
    _OSBASE_TRACE(1,("--- %s CMPI GetInstance() exited : %s",_ClassName,CMGetCharPtr(rc.msg)));
    LXS_Term(lhdl);
    return rc;
  }

  if( ci == NULL ) { 
    if( rc.msg != NULL ) {
      _OSBASE_TRACE(1,("--- %s CMPI GetInstance() failed : %s",_ClassName,CMGetCharPtr(rc.msg)));
    }
    else {
      _OSBASE_TRACE(1,("--- %s CMPI GetInstance() failed",_ClassName));
    }
    LXS_Term(lhdl);
    return rc;
  }

  LXS_Term(lhdl);
  CMReturnInstance( rslt, ci );
  CMReturnDone(rslt);
  _OSBASE_TRACE(1,("--- %s CMPI GetInstance() exited",_ClassName));
  return rc;
}

CMPIStatus OSBase_ServiceProviderCreateInstance( CMPIInstanceMI * mi, 
           CMPIContext * ctx, 
           CMPIResult * rslt, 
           CMPIObjectPath * cop, 
           CMPIInstance * ci) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};

  _OSBASE_TRACE(1,("--- %s CMPI CreateInstance() called",_ClassName));

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 

  _OSBASE_TRACE(1,("--- %s CMPI CreateInstance() exited",_ClassName));
  return rc;
}

CMPIStatus OSBase_ServiceProviderSetInstance( CMPIInstanceMI * mi, 
           CMPIContext * ctx, 
           CMPIResult * rslt, 
           CMPIObjectPath * cop,
           CMPIInstance * ci, 
           char **properties) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};

  _OSBASE_TRACE(1,("--- %s CMPI SetInstance() called",_ClassName));

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 

  _OSBASE_TRACE(1,("--- %s CMPI SetInstance() exited",_ClassName));
  return rc;
}

CMPIStatus OSBase_ServiceProviderDeleteInstance( CMPIInstanceMI * mi, 
           CMPIContext * ctx, 
           CMPIResult * rslt, 
           CMPIObjectPath * cop) {
  CMPIStatus rc = {CMPI_RC_OK, NULL}; 

  _OSBASE_TRACE(1,("--- %s CMPI DeleteInstance() called",_ClassName));

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 

  _OSBASE_TRACE(1,("--- %s CMPI DeleteInstance() exited",_ClassName));
  return rc;
}

CMPIStatus OSBase_ServiceProviderExecQuery( CMPIInstanceMI * mi, 
           CMPIContext * ctx, 
           CMPIResult * rslt, 
           CMPIObjectPath * ref, 
           char * lang, 
           char * query) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};

  _OSBASE_TRACE(1,("--- %s CMPI ExecQuery() called",_ClassName));

  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 

  _OSBASE_TRACE(1,("--- %s CMPI ExecQuery() exited",_ClassName));
  return rc;
}


/* ---------------------------------------------------------------------------*/
/*                        Method Provider Interface                           */
/* ---------------------------------------------------------------------------*/


CMPIStatus OSBase_ServiceProviderMethodCleanup( CMPIMethodMI * mi, 
           CMPIContext * ctx) {
  _OSBASE_TRACE(1,("--- %s CMPI MethodCleanup() called",_ClassName));
  _OSBASE_TRACE(1,("--- %s CMPI MethodCleanup() exited",_ClassName));
  CMReturn(CMPI_RC_OK);
}

CMPIStatus OSBase_ServiceProviderInvokeMethod( CMPIMethodMI * mi,
           CMPIContext * ctx,
           CMPIResult * rslt,
           CMPIObjectPath * ref,
           char * methodName,
           CMPIArgs * in,
           CMPIArgs * out) {
#undef  FREESTUFF  
#define FREESTUFF() free(clsname), \
                    free(servicename)
  CMPIValue    valrc;
  CMPIString * class       = NULL; 
  CMPIString * service     = NULL; 
  char       * clsname     = NULL;
  char       * servicename = NULL;
  CMPIStatus   rc          = {CMPI_RC_OK, NULL};

  _OSBASE_TRACE(1,("--- %s CMPI InvokeMethod() called",_ClassName));

  class = CMGetClassName(ref, &rc);
  service = CMGetKey(ref, "Name", &rc).value.string;
  if( rc.rc != CMPI_RC_OK ) { return rc; }

  clsname = CMGetCharPtr(class);
  servicename = CMGetCharPtr(service);

  if (strcasecmp(clsname,"linux_service") || servicename==NULL) {
    CMSetStatusWithChars( _broker, &rc, CMPI_RC_ERR_INVALID_PARAMETER, 
			  "invalid class/service name" );
    return rc;
  }

  if( strcasecmp("StartService",methodName) == 0 || 
      strcasecmp("StopService",methodName)  == 0  ) {
    valrc.uint32 = LXS_ServiceOp(servicename, methodName);
    CMReturnData(rslt, &valrc, CMPI_uint32);
    CMReturnDone(rslt);
  }
  else {
    CMSetStatusWithChars( _broker, &rc,
			  CMPI_RC_ERR_METHOD_NOT_FOUND, methodName );
  }

  _OSBASE_TRACE(1,("--- %s CMPI InvokeMethod() exited",_ClassName)); 
  return rc;
}


/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/

CMInstanceMIStub( OSBase_ServiceProvider, 
                  OSBase_ServiceProvider, 
                  _broker, 
                  CMNoHook);

CMMethodMIStub( OSBase_ServiceProvider, 
		OSBase_ServiceProvider, 
                _broker, 
                CMNoHook);

/* ---------------------------------------------------------------------------*/
/*                end of cmpiOSBase_ServiceProvider                           */
/* ---------------------------------------------------------------------------*/
