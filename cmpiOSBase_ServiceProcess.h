#ifndef _CMPIOSBASE_SERVICEPROCESS_H_
#define _CMPIOSBASE_SERVICEPROCESS_H_

/*
 * cmpiOSBase_ServiceProcess.h
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
 * Author:       Heidi Neumann <heidineu@de.ibm.com>
 * Contributors: Viktor Mihajlovski <mihajlov@de.ibm.com>
 *
 * Interface Type : Common Magabeablity Programming Interface ( CMPI )
 *
 * Description:
 * This file defines the interfaces for the factory implementation of the
 * CIM association Linux_ServiceProcess.
 *
*/


/* ---------------------------------------------------------------------------*/

#include "cmpidt.h"
#include <cimlxs.h>

/* ---------------------------------------------------------------------------*/

static char * _ClassName     = "Linux_ServiceProcess";
static char * _RefLeft       = "Service";
static char * _RefRight      = "Process";
static char * _RefLeftClass  = "Linux_Service";
static char * _RefRightClass = "Linux_UnixProcess";

#define ATYPE_NONE    0x00000000
#define ATYPE_ASSOC   0x00000001
#define ATYPE_NAME    0x00000002

/* ---------------------------------------------------------------------------*/

/* method to create a CMPIObjectPath of Linux_ServiceProcess                  */

CMPIObjectPath * _makePath_ServiceProcess( CMPIBroker * _broker, 
					   CMPIObjectPath * service,
					   CMPIObjectPath * process,
					   CMPIStatus * rc);


/* method to create a CMPIInstance of Linux_ServiceProcess                    */

CMPIInstance * _makeInst_ServiceProcess( CMPIBroker * _broker,
					 CMPIObjectPath * service,
					 CMPIObjectPath * process,
					 CMPIStatus * rc);


/* ---------------------------------------------------------------------------*/

/* method to handle the association logic for Linux_ServiceProcess            */

int assoc_ServiceProcess( CMPIBroker * _broker,
			  CMPIContext * ctx,
			  CMPIResult * rslt,
			  CMPIObjectPath * ref,
			  int assocType,
			  CMPIStatus * rc );

/* ---------------------------------------------------------------------------*/

#endif

