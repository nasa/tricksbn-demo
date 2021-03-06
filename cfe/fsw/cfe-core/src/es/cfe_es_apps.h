/*
**  File: 
**  cfe_es_apps.h
**
**
**
**      Copyright (c) 2004-2012, United States government as represented by the 
**      administrator of the National Aeronautics Space Administration.  
**      All rights reserved. This software(cFE) was created at NASA's Goddard 
**      Space Flight Center pursuant to government contracts.
**
**      This is governed by the NASA Open Source Agreement and may be used, 
**      distributed and modified only pursuant to the terms of that agreement.
**
**  Purpose:
**  This file contains the Internal interface for the cFE Application control functions of ES.
**  These functions and data structures manage the Applications and Child tasks in the cFE.
**
**  References:
**     Flight Software Branch C Coding Standard Version 1.0a
**     cFE Flight Software Application Developers Guide
**
*/


#ifndef _cfe_es_apps_
#define _cfe_es_apps_

/*
** Include Files
*/
#include "common_types.h"
#include "osapi.h"

/*
** Macro Definitions
*/
#define CFE_ES_STARTSCRIPT_MAX_TOKENS_PER_LINE      8

/*
** Type Definitions
*/

/*
** CFE_ES_AppState_t is a structure of information for External cFE Apps.
**   This information is used to control/alter the state of External Apps.
**   The fields in this structure are not needed or used for the cFE Core Apps.
*/
typedef struct
{
   uint32     AppControlRequest;              /* What the App should be doing next */
   uint32     AppState;                       /* Is the app running, or stopped, or waiting? */
   int32      AppTimer;                       /* Countdown timer for killing an app */

} CFE_ES_AppState_t;


/*
** CFE_ES_AppStartParams_t is a structure of information used when an application is
**   created in the system. It is stored in the cFE ES App Table
*/
typedef struct
{
  char                  Name[OS_MAX_API_NAME];
  char                  EntryPoint[OS_MAX_API_NAME];
  char                  FileName[OS_MAX_PATH_LEN];

  uint32                StackSize;
  cpuaddr               StartAddress;
  uint32                ModuleId;

  uint16                ExceptionAction;
  uint16                Priority;
   
} CFE_ES_AppStartParams_t;

/*
** CFE_ES_MainTaskInfo_t is a structure of information about the main
** task and child tasks in a cFE application. This structure is just used in the
** cFE_ES_AppRecord_t structure. 
*/
typedef struct
{
   uint32   MainTaskId;                     /* The Application's Main Task ID */
   char     MainTaskName[OS_MAX_API_NAME];  /* The Application's Main Task ID */
   uint32   NumOfChildTasks;                /* Number of Child tasks for an App */

} CFE_ES_MainTaskInfo_t;


/*
** CFE_ES_AppRecord_t is an internal structure used to keep track of
** CFE Applications that are active in the system.
*/
typedef struct
{
   boolean                 RecordUsed;                  /* Is the record used(1) or available(0) ? */
   uint32                  Type;                        /* The type of App: CORE or EXTERNAL */
   CFE_ES_AppStartParams_t StartParams;                 /* The start parameters for an App */
   CFE_ES_AppState_t       StateRecord;                 /* The State info for External cFE Apps */
   CFE_ES_MainTaskInfo_t   TaskInfo;                    /* Information about the Tasks */
      
} CFE_ES_AppRecord_t;


/*
** CFE_ES_TaskRecord_t is an internal structure used to keep track of
** CFE Tasks that are active in the system.
*/
typedef struct
{
   boolean   RecordUsed;                      /* Is the record used(1) or available(0) */
   uint32    AppId;                           /* The parent Application's App ID */
   uint32    TaskId;                          /* Task ID */
   uint32    ExecutionCounter;                /* The execution counter for the Child task */
   char      TaskName[OS_MAX_API_NAME];       /* Task Name */
   
   
} CFE_ES_TaskRecord_t;

/*
** CFE_ES_LibRecord_t is an internal structure used to keep track of
** CFE Shared Libraries that are loaded in the system.
*/
typedef struct
{
   boolean   RecordUsed;                      /* Is the record used(1) or available(0) */
   char      LibName[OS_MAX_API_NAME];        /* Library Name */
} CFE_ES_LibRecord_t;

/*****************************************************************************/
/*
** Function prototypes
*/

/*
** Internal function start applications based on the startup script
*/
void  CFE_ES_StartApplications(uint32 ResetType, const char *StartFilePath );

/*
** Internal function to parse/execute a line of the cFE application startup 'script'
*/
int32 CFE_ES_ParseFileEntry(const char **TokenList, uint32 NumTokens);

/*
 * Internal function to set the state of an app
 * All state changes should go through this function rather than directly writing to the control block
 */
void CFE_ES_SetAppState(uint32 AppID, uint32 TargetState);


/*
** Internal function to create/start a new cFE app
** based on the parameters passed in
*/
int32 CFE_ES_AppCreate(uint32 *ApplicationIdPtr,
                       const char   *FileName,
                       const void   *EntryPointData,
                       const char   *AppName,
                       uint32  Priority,
                       uint32  StackSize,
                       uint32  ExceptionAction);
/*
** Internal function to load a a new cFE shared Library
*/
int32 CFE_ES_LoadLibrary(uint32 *LibraryIdPtr,
                       const char   *Path,
                       const void   *EntryPointData,
                       const char   *Name);

/*
** Get Application List
*/
int32 CFE_ES_AppGetList(uint32 AppIdArray[], uint32 ArraySize);

/*
** Dump All application properties to a file
**  Note: Do we want to specify a file here?
*/
int32 CFE_ES_AppDumpAllInfo(void);

/*
** Scan the Application Table for actions to take
*/
void CFE_ES_ScanAppTable(void);

/*
** Perform the requested control action for an application
*/
void CFE_ES_ProcessControlRequest(uint32 AppID);

/*
** Clean up all app resources and delete it
*/
int32 CFE_ES_CleanUpApp(uint32 AppId);

/*
** Clean up all Task resources and detete the task
*/
int32 CFE_ES_CleanupTaskResources(uint32 TaskId);

/*
** Debug function to print out resource utilization 
*/
int32 CFE_ES_ListResourcesDebug(void);

/*
** Populate the cFE_ES_AppInfo structure with the data for an app
** This is an internal function for use in ES.
** The newer external API is : CFE_ES_GetAppInfo
*/
void CFE_ES_GetAppInfoInternal(uint32 AppId, CFE_ES_AppInfo_t *AppInfoPtr );

#endif  /* _cfe_es_apps_ */
