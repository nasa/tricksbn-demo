/*
**
**  File Name: cfe_evs.c
**  $Id: cfe_evs.c 1.8 2014/05/30 13:26:02GMT-05:00 lwalling Exp  $
**
**      Copyright (c) 2004-2012, United States government as represented by the 
**      administrator of the National Aeronautics Space Administration.  
**      All rights reserved. This software(cFE) was created at NASA's Goddard 
**      Space Flight Center pursuant to government contracts.
**
**      This is governed by the NASA Open Source Agreement and may be used, 
**      distributed and modified only pursuant to the terms of that agreement.
**
**  Title: Event Services API library
**
**  Purpose: This module defines the library functions of the
**           Event Services API
**
*/

/* Include Files */
#include "cfe_evs.h"         /* EVS library function definitions */
#include "cfe_evs_task.h"    /* EVS internal definitions */
#include "cfe_evs_utils.h"   /* EVS utility function definitions */
#include "common_types.h"    /* Basic data types */
#include "cfe_es.h"          /* Executive Service definitions */
#include "cfe_error.h"       /* cFE error code definitions */

#include <stdarg.h>
#include <string.h>

/* External Data */

/* Local Function Prototypes */


/* Function Definitions */

/*
**             Function Prologue
**
** Function Name:      CFE_EVS_Register
**
** Purpose:  This routine registers an application for receiving event services.
**
** Assumptions and Notes:
**
*/
int32 CFE_EVS_Register (void *Filters, uint16 NumEventFilters, uint16 FilterScheme)
{
   uint16 FilterLimit;
   uint16 i;
   int32  Status = CFE_SUCCESS;
   uint32 AppID = CFE_EVS_UNDEF_APPID;
   CFE_EVS_BinFilter_t   *AppFilters;
   EVS_AppData_t         *AppDataPtr;

   /* Query and verify the caller's AppID */
   Status = EVS_GetAppID(&AppID);

   if (Status == CFE_SUCCESS)
   {
      /* Cleanup if already registered */
      if (CFE_EVS_GlobalData.AppData[AppID].RegisterFlag == TRUE)
      {
         memset(&CFE_EVS_GlobalData.AppData[AppID], 0, sizeof(EVS_AppData_t));
      }

      /* Verify filter arguments */
      if (FilterScheme != CFE_EVS_EventFilter_BINARY)
      {
         Status = CFE_EVS_UNKNOWN_FILTER;
      }
      else if ((NumEventFilters != 0) && (Filters == NULL))
      {
         Status = CFE_ES_ERR_BUFFER;
      }
      else
      {
         /* Initialize application event data */
         AppDataPtr = &CFE_EVS_GlobalData.AppData[AppID];
         AppDataPtr->RegisterFlag = TRUE;
         AppDataPtr->ActiveFlag   = TRUE;
         AppDataPtr->EventCount   = 0;
         AppDataPtr->EventTypesActiveFlag = CFE_PLATFORM_EVS_DEFAULT_TYPE_FLAG;

         /* Set limit for number of provided filters */
         if (NumEventFilters < CFE_PLATFORM_EVS_MAX_EVENT_FILTERS)
         {
            FilterLimit = NumEventFilters;
         }
         else
         {
            FilterLimit = CFE_PLATFORM_EVS_MAX_EVENT_FILTERS;
         }

         if (Filters != NULL)
         {
            AppFilters = (CFE_EVS_BinFilter_t *) Filters;

            /* Copy provided filters */
            for (i = 0; i < FilterLimit; i++)
            {
               AppDataPtr->BinFilters[i].EventID = AppFilters[i].EventID;
               AppDataPtr->BinFilters[i].Mask    = AppFilters[i].Mask;
               AppDataPtr->BinFilters[i].Count   = 0;
            }
         }

         /* Initialize remainder of filters as unused */
         for (i = FilterLimit; i < CFE_PLATFORM_EVS_MAX_EVENT_FILTERS; i++)
         {
            AppDataPtr->BinFilters[i].EventID = CFE_EVS_FREE_SLOT;
            AppDataPtr->BinFilters[i].Mask    = 0;
            AppDataPtr->BinFilters[i].Count   = 0;
         }
      }
   }

   return(Status);

} /* End CFE_EVS_Register */


/*
**             Function Prologue
**
** Function Name:      CFE_EVS_Unregister
**
** Purpose:  This routine unregisters an application for receiving event services.
**
** Assumptions and Notes:
**
*/
int32 CFE_EVS_Unregister(void)
{
   int32 Status = CFE_SUCCESS;
   uint32 AppID = CFE_EVS_UNDEF_APPID;

   /* Query and verify the caller's AppID */
   Status = EVS_GetAppID(&AppID);

   if (Status == CFE_SUCCESS)
   {
      /* Cleanup if already registered */
      if (CFE_EVS_GlobalData.AppData[AppID].RegisterFlag == TRUE)
      {
         memset(&CFE_EVS_GlobalData.AppData[AppID], 0, sizeof(EVS_AppData_t));
      }
   }

   return(Status);

} /* End CFE_EVS_Unregister */


/*
**             Function Prologue
**
** Function Name:      CFE_EVS_SendEvent
**
** Purpose:  This routine sends an event message, with message ID containing the current
**           processor/box and task ID as well as the eventID.
**
** Assumptions and Notes:
*/
int32 CFE_EVS_SendEvent (uint16 EventID, uint16 EventType, const char *Spec, ... )
{
   int32              Status = CFE_SUCCESS;
   uint32             AppID = CFE_EVS_UNDEF_APPID;
   CFE_TIME_SysTime_t Time;
   va_list            Ptr;

   /* Query and verify the caller's AppID */
   Status = EVS_GetAppID(&AppID);

   if (Status == CFE_SUCCESS)
   {
      if (CFE_EVS_GlobalData.AppData[AppID].RegisterFlag == FALSE)
      {
         /* Handler for events from apps not registered with EVS */
         Status = EVS_NotRegistered(AppID);
      }
      else if (EVS_IsFiltered(AppID, EventID, EventType) == FALSE)
      {
          /* Get current spacecraft time */
          Time = CFE_TIME_GetTime();

          /* Send the event packets */
          va_start(Ptr, Spec);
          EVS_GenerateEventTelemetry(AppID, EventID, EventType, &Time, Spec, Ptr);
          va_end(Ptr);
      }
   }

   return(Status);

} /* End CFE_EVS_SendEvent */


/*
**             Function Prologue
**
** Function Name:      CFE_EVS_SendEventWithAppID
**
** Purpose:  This routine sends an event message, with message ID containing the current
**           processor/box and task ID as well as the eventID. Also forces the user to
**           supply an AppID to be sent with the message.
**
** Assumptions and Notes:
*/
int32 CFE_EVS_SendEventWithAppID (uint16 EventID, uint16 EventType, uint32 AppID, const char *Spec, ... )
{
   int32              Status = CFE_SUCCESS;
   CFE_TIME_SysTime_t Time;
   va_list            Ptr;

   if (AppID >= CFE_PLATFORM_ES_MAX_APPLICATIONS)
   {
      Status = CFE_EVS_APP_ILLEGAL_APP_ID;
   }
   else if (CFE_EVS_GlobalData.AppData[AppID].RegisterFlag == FALSE)
   {
      /* Handler for events from apps not registered with EVS */
      Status = EVS_NotRegistered(AppID);
   }
   else if (EVS_IsFiltered(AppID, EventID, EventType) == FALSE)
   {
      /* Get current spacecraft time */
      Time = CFE_TIME_GetTime();

      /* Send the event packets */
      va_start(Ptr, Spec);
      EVS_GenerateEventTelemetry(AppID, EventID, EventType, &Time, Spec, Ptr);
      va_end(Ptr);
   }

   return Status;

} /* End CFE_EVS_SendEventWithAppID */


/*
**             Function Prologue
**
** Function Name:      CFE_EVS_SendTimedEvent
**
** Purpose:  This routine sends a timed event message, with message ID containing the
**           current processor/box and task ID as well as the eventID.
**
** Assumptions and Notes:
*/
int32 CFE_EVS_SendTimedEvent (CFE_TIME_SysTime_t Time, uint16 EventID, uint16 EventType, const char *Spec, ... )
{
   int32              Status = CFE_SUCCESS;
   uint32             AppID = CFE_EVS_UNDEF_APPID;
   va_list            Ptr;

   /* Query and verify the caller's AppID */
   Status = EVS_GetAppID(&AppID);

   if (Status == CFE_SUCCESS)
   {
      if (CFE_EVS_GlobalData.AppData[AppID].RegisterFlag == FALSE)
      {
         /* Handler for events from apps not registered with EVS */
         Status = EVS_NotRegistered(AppID);
      }
      else if (EVS_IsFiltered(AppID, EventID, EventType) == FALSE)
      {
         /* Send the event packets */
         va_start(Ptr, Spec);
         EVS_GenerateEventTelemetry(AppID, EventID, EventType, &Time, Spec, Ptr);
         va_end(Ptr);
      }
   }

   return(Status);

} /* End CFE_EVS_SendTimedEvent */


/*
**             Function Prologue
**
** Function Name:      CFE_EVS_ResetFilter
**
** Purpose:  This routine clears the filter counter for the calling application's given event_id.
**
** Assumptions and Notes:
**
*/
int32 CFE_EVS_ResetFilter (int16 EventID)
{
   int32            Status = CFE_SUCCESS;
   EVS_BinFilter_t *FilterPtr = NULL;
   uint32           AppID = CFE_EVS_UNDEF_APPID;

   /* Query and verify the caller's AppID */
   Status = EVS_GetAppID(&AppID);

   if (Status == CFE_SUCCESS)
   {
      if (CFE_EVS_GlobalData.AppData[AppID].RegisterFlag == FALSE)
      {
         Status = CFE_EVS_APP_NOT_REGISTERED;
      }
      else
      {
         FilterPtr = EVS_FindEventID(EventID, CFE_EVS_GlobalData.AppData[AppID].BinFilters);

         if (FilterPtr != NULL)
         {
            FilterPtr->Count = 0;
         }
         else
         {
            Status = CFE_EVS_EVT_NOT_REGISTERED;
         }
      }
   }

   return(Status);

} /* End CFE_EVS_ResetFilter */


/*
**             Function Prologue
**
** Function Name:      CFE_EVS_ResetAllFilters
**
** Purpose:  This routine clears all filter counters for the calling application
**
** Assumptions and Notes:
**
*/
int32 CFE_EVS_ResetAllFilters ( void )
{
   int32    Status = CFE_SUCCESS;
   uint32   AppID = CFE_EVS_UNDEF_APPID;
   uint32   i;

   /* Get and verify caller's AppID */
   Status = EVS_GetAppID(&AppID);

   if (Status == CFE_SUCCESS)
   {
      if (CFE_EVS_GlobalData.AppData[AppID].RegisterFlag == FALSE)
      {
         Status = CFE_EVS_APP_NOT_REGISTERED;
      }
      else
      {
         for (i = 0; i < CFE_PLATFORM_EVS_MAX_EVENT_FILTERS; i++)
         {
            CFE_EVS_GlobalData.AppData[AppID].BinFilters[i].Count = 0;
         }
      }
   }

   return(Status);

} /* End CFE_EVS_ResetAllFilters */


/* End CFE_EVS.C */
