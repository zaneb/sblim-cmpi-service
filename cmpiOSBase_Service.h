#ifndef _CMPIOSBASE_SERVICE_H_
#define _CMPIOSBASE_SERVICE_H_

/*
 * cmpiOSBase_Service.h
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
 * CIM class Linux_Service.
 *
*/


/* ---------------------------------------------------------------------------*/

#include "cmpidt.h"
#include <cimlxs.h>

/* ---------------------------------------------------------------------------*/

static char * _ClassName = "Linux_Service";

/* ---------------------------------------------------------------------------*/

/* method to create a CMPIObjectPath of Linux_Service                         */

CMPIObjectPath * _makePath_Service( CMPIBroker * _broker,
                 CMPIContext * ctx, 
                 CMPIObjectPath * cop,
		 LXS_Service * lxssvc,
                 CMPIStatus * rc);


/* method to create a CMPIInstance of Linux_Service                           */

CMPIInstance * _makeInst_Service( CMPIBroker * _broker,
               CMPIContext * ctx, 
               CMPIObjectPath * cop,
	       const char ** properties,
	       LXS_Service * lxssvc,
               CMPIStatus * rc);

/* ---------------------------------------------------------------------------*/

void serviceDup(LXS_Service*,LXS_Service*);
int  serviceChanged(LXS_Service*,LXS_Service*);

/* ---------------------------------------------------------------------------*/

#endif

