/*
** $Id: cfe_time_utils.c 1.10 2012/10/01 16:37:48GMT-05:00 aschoeni Exp  $
**
**      Copyright (c) 2004-2012, United States government as represented by the 
**      administrator of the National Aeronautics Space Administration.  
**      All rights reserved. This software(cFE) was created at NASA's Goddard 
**      Space Flight Center pursuant to government contracts.
**
**      This is governed by the NASA Open Source Agreement and may be used, 
**      distributed and modified only pursuant to the terms of that agreement.
**
** Purpose:  cFE Time Services (TIME) library utilities source file
**
** Author:   S.Walling/Microtel
**
** Notes:
** 
*/

/*
** Required header files...
*/
#include "cfe_time_utils.h"
#include "cfe_msgids.h"
#include "private/cfe_es_resetdata_typedef.h"

#include <string.h>



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_LatchClock() -- query local clock                      */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

CFE_TIME_SysTime_t CFE_TIME_LatchClock(void)
{
    CFE_TIME_SysTime_t LatchTime;
    OS_time_t LocalTime;

    /*
    ** Get time in O/S format (seconds : microseconds)...
    */
    CFE_PSP_GetTime(&LocalTime);

    /*
    ** Convert time to cFE format (seconds : 1/2^32 subseconds)...
    */
    LatchTime.Seconds = LocalTime.seconds;
    LatchTime.Subseconds = CFE_TIME_Micro2SubSecs(LocalTime.microsecs);

    return(LatchTime);

} /* End of CFE_TIME_LatchClock() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_QueryResetVars() -- query contents of Reset Variables  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void CFE_TIME_QueryResetVars(void)
{

    CFE_TIME_ResetVars_t LocalResetVars;
    uint32 DefSubsMET;
    uint32 DefSubsSTCF;
    int32 status;
    
    uint32 resetAreaSize;
    cpuaddr resetAreaAddr;
    CFE_ES_ResetData_t  *CFE_TIME_ResetDataPtr;
   
    /*
    ** Get the pointer to the Reset area from the BSP
    */
    status = CFE_PSP_GetResetArea (&(resetAreaAddr), &(resetAreaSize));
      
    if (status != CFE_PSP_SUCCESS)
    {
        /* There is something wrong with the Reset Area */
        CFE_TIME_TaskData.DataStoreStatus = CFE_TIME_RESET_AREA_BAD;
    }
    
    else
    {
        CFE_TIME_ResetDataPtr = (CFE_ES_ResetData_t *)resetAreaAddr;
        
        /* Get the structure from the Reset Area */
        LocalResetVars = CFE_TIME_ResetDataPtr -> TimeResetVars;

        /*
        ** Verify TIME data signature and clock signal selection...
        **    (other data fields have no verifiable limits)
        */
        if ((LocalResetVars.Signature == CFE_TIME_RESET_SIGNATURE) &&
            ((LocalResetVars.ClockSignal == CFE_TIME_ToneSignalSelect_PRIMARY) ||
            (LocalResetVars.ClockSignal == CFE_TIME_ToneSignalSelect_REDUNDANT)))
        {
            /*
            ** Initialize TIME to valid  Reset Area values...
            */
            CFE_TIME_TaskData.AtToneMET    = LocalResetVars.CurrentMET;
            CFE_TIME_TaskData.AtToneSTCF   = LocalResetVars.CurrentSTCF;
            CFE_TIME_TaskData.AtToneDelay  = LocalResetVars.CurrentDelay;
            CFE_TIME_TaskData.AtToneLeapSeconds  = LocalResetVars.LeapSeconds;
            CFE_TIME_TaskData.ClockSignal  = LocalResetVars.ClockSignal;

            CFE_TIME_TaskData.DataStoreStatus  = CFE_TIME_RESET_AREA_EXISTING;
        }    
        else
        {   
            /*
            ** We got a blank area from the reset variables
            */
            CFE_TIME_TaskData.DataStoreStatus  = CFE_TIME_RESET_AREA_NEW;
        }

    }
    /*
    ** Initialize TIME to default values if no valid Reset data...
    */
    if (CFE_TIME_TaskData.DataStoreStatus != CFE_TIME_RESET_AREA_EXISTING)
    {
        DefSubsMET  = CFE_TIME_Micro2SubSecs(CFE_MISSION_TIME_DEF_MET_SUBS);
        DefSubsSTCF = CFE_TIME_Micro2SubSecs(CFE_MISSION_TIME_DEF_STCF_SUBS);

        CFE_TIME_TaskData.AtToneMET.Seconds      = CFE_MISSION_TIME_DEF_MET_SECS;
        CFE_TIME_TaskData.AtToneMET.Subseconds   = DefSubsMET;
        CFE_TIME_TaskData.AtToneSTCF.Seconds     = CFE_MISSION_TIME_DEF_STCF_SECS;
        CFE_TIME_TaskData.AtToneSTCF.Subseconds  = DefSubsSTCF;
        CFE_TIME_TaskData.AtToneLeapSeconds      = CFE_MISSION_TIME_DEF_LEAPS;
        CFE_TIME_TaskData.ClockSignal            = CFE_TIME_ToneSignalSelect_PRIMARY;
        CFE_TIME_TaskData.AtToneDelay.Seconds    = 0;
        CFE_TIME_TaskData.AtToneDelay.Subseconds = 0;
    }
    
    return;

} /* End of CFE_TIME_QueryResetVars() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_UpdateResetVars() -- update contents of Reset Variables*/
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void CFE_TIME_UpdateResetVars(const CFE_TIME_Reference_t *Reference)
{
    CFE_TIME_ResetVars_t LocalResetVars;
    uint32 resetAreaSize;
    cpuaddr resetAreaAddr;
    CFE_ES_ResetData_t  *CFE_TIME_ResetDataPtr;
    /*
    ** Update the data only if our Reset Area is valid...
    */
    if (CFE_TIME_TaskData.DataStoreStatus != CFE_TIME_RESET_AREA_ERROR)
    {
   
        /* Store all of our critical variables to a ResetVars_t
         * then copy that to the Reset Area */
        LocalResetVars.Signature = CFE_TIME_RESET_SIGNATURE;

        LocalResetVars.CurrentMET   = Reference->CurrentMET;
        LocalResetVars.CurrentSTCF  = Reference->AtToneSTCF;
        LocalResetVars.CurrentDelay = Reference->AtToneDelay;
        LocalResetVars.LeapSeconds  = Reference->AtToneLeapSeconds;

        LocalResetVars.ClockSignal  = CFE_TIME_TaskData.ClockSignal;
   
        /*
        ** Get the pointer to the Reset area from the BSP
        */
        if (CFE_PSP_GetResetArea (&(resetAreaAddr), &(resetAreaSize)) == CFE_PSP_SUCCESS)
        {
           CFE_TIME_ResetDataPtr = (CFE_ES_ResetData_t *)resetAreaAddr;
           CFE_TIME_ResetDataPtr -> TimeResetVars = LocalResetVars;
        }
    }

    return;

} /* End of CFE_TIME_UpdateResetVars() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_InitData() -- initialize global time task data         */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void CFE_TIME_InitData(void)
{
    uint32  i = 0;
    
    /*
    ** Initialize task command execution counters...
    */
    CFE_TIME_TaskData.CommandCounter = 0;
    CFE_TIME_TaskData.CommandErrorCounter = 0;

    /*
    ** Initialize task configuration data...
    */
    strcpy(CFE_TIME_TaskData.PipeName, CFE_TIME_TASK_PIPE_NAME);
    CFE_TIME_TaskData.PipeDepth = CFE_TIME_TASK_PIPE_DEPTH;
    
    /*
    ** Try to get values used to compute time from Reset Area...
    */
    CFE_TIME_QueryResetVars();

    /*
    ** Remaining data values used to compute time...
    */
    CFE_TIME_TaskData.AtToneLatch = CFE_TIME_LatchClock();

    /*
    ** Data values used to define the current clock state...
    */
    CFE_TIME_TaskData.ClockSetState  = CFE_TIME_SetState_NOT_SET;
    CFE_TIME_TaskData.ClockFlyState  = CFE_TIME_FlywheelState_IS_FLY;

#if (CFE_PLATFORM_TIME_CFG_SOURCE == TRUE)
    CFE_TIME_TaskData.ClockSource    = CFE_TIME_SourceSelect_EXTERNAL;
#else
    CFE_TIME_TaskData.ClockSource    = CFE_TIME_SourceSelect_INTERNAL;
#endif
    CFE_TIME_TaskData.ServerFlyState = CFE_TIME_FlywheelState_IS_FLY;

    /*
    ** Pending data values (from "time at tone" command data packet)...
    */
    CFE_TIME_TaskData.PendingMET.Seconds     = 0;
    CFE_TIME_TaskData.PendingMET.Subseconds  = 0;
    CFE_TIME_TaskData.PendingSTCF.Seconds    = 0;
    CFE_TIME_TaskData.PendingSTCF.Subseconds = 0;
    CFE_TIME_TaskData.PendingLeaps           = 0;
    CFE_TIME_TaskData.PendingState           = CFE_TIME_ClockState_INVALID;

    /*
    ** STCF adjustment values...
    */
    CFE_TIME_TaskData.OneTimeAdjust.Seconds    = 0;
    CFE_TIME_TaskData.OneTimeAdjust.Subseconds = 0;
    CFE_TIME_TaskData.OneHzAdjust.Seconds      = 0;
    CFE_TIME_TaskData.OneHzAdjust.Subseconds   = 0;

    CFE_TIME_TaskData.OneTimeDirection = CFE_TIME_AdjustDirection_ADD;
    CFE_TIME_TaskData.OneHzDirection   = CFE_TIME_AdjustDirection_ADD;
    CFE_TIME_TaskData.DelayDirection   = CFE_TIME_AdjustDirection_ADD;

    /*
    ** Local clock latch values...
    */
    CFE_TIME_TaskData.ToneSignalLatch.Seconds    = 0;
    CFE_TIME_TaskData.ToneSignalLatch.Subseconds = 0;
    CFE_TIME_TaskData.ToneDataLatch.Seconds      = 0;
    CFE_TIME_TaskData.ToneDataLatch.Subseconds   = 0;

    /*
    ** Miscellaneous counters...
    */
    CFE_TIME_TaskData.ToneMatchCounter  = 0;
    CFE_TIME_TaskData.ToneMatchErrorCounter = 0;
    CFE_TIME_TaskData.ToneSignalCounter = 0;
    CFE_TIME_TaskData.ToneDataCounter   = 0;
    CFE_TIME_TaskData.ToneIntCounter    = 0;
    CFE_TIME_TaskData.ToneIntErrorCounter   = 0;
    CFE_TIME_TaskData.ToneTaskCounter   = 0;
    CFE_TIME_TaskData.VirtualMET      = CFE_TIME_TaskData.AtToneMET.Seconds;
    CFE_TIME_TaskData.PendingVersionCounter  = 0;
    CFE_TIME_TaskData.CompleteVersionCounter = 0;
    CFE_TIME_TaskData.LocalIntCounter   = 0;
    CFE_TIME_TaskData.LocalTaskCounter  = 0;
    CFE_TIME_TaskData.InternalCount   = 0;
    CFE_TIME_TaskData.ExternalCount   = 0;

    /*
    ** Time window verification values...
    */
    CFE_TIME_TaskData.MinElapsed = CFE_TIME_Micro2SubSecs(CFE_MISSION_TIME_MIN_ELAPSED);
    CFE_TIME_TaskData.MaxElapsed = CFE_TIME_Micro2SubSecs(CFE_MISSION_TIME_MAX_ELAPSED);

    /*
    ** Range checking for external time source data...
    */
    #if (CFE_PLATFORM_TIME_CFG_SOURCE == TRUE)
    CFE_TIME_TaskData.MaxDelta.Seconds    = CFE_PLATFORM_TIME_MAX_DELTA_SECS;
    CFE_TIME_TaskData.MaxDelta.Subseconds = CFE_TIME_Micro2SubSecs(CFE_PLATFORM_TIME_MAX_DELTA_SUBS);
    #else
    CFE_TIME_TaskData.MaxDelta.Seconds    = 0;
    CFE_TIME_TaskData.MaxDelta.Subseconds = 0;
    #endif

    /*
    ** Maximum local clock value (before roll-over)...
    */
    CFE_TIME_TaskData.MaxLocalClock.Seconds    = CFE_PLATFORM_TIME_MAX_LOCAL_SECS;
    CFE_TIME_TaskData.MaxLocalClock.Subseconds = CFE_PLATFORM_TIME_MAX_LOCAL_SUBS;

    /*
    ** Range limits for time between tone signal interrupts...
    */
    CFE_TIME_TaskData.ToneOverLimit  = CFE_TIME_Micro2SubSecs(CFE_PLATFORM_TIME_CFG_TONE_LIMIT);
    CFE_TIME_TaskData.ToneUnderLimit = CFE_TIME_Micro2SubSecs((1000000 - CFE_PLATFORM_TIME_CFG_TONE_LIMIT));

    /*
    ** Clock state has been commanded into (CFE_TIME_ClockState_FLYWHEEL)...
    */
    CFE_TIME_TaskData.Forced2Fly = FALSE;

    /*
    ** Clock state has just transitioned into (CFE_TIME_ClockState_FLYWHEEL)...
    */
    CFE_TIME_TaskData.AutoStartFly = FALSE;
    
    /*
    ** Clear the Synch Callback Registry of any garbage
    */
    for (i=0; i < (sizeof(CFE_TIME_TaskData.SynchCallback) / sizeof(CFE_TIME_TaskData.SynchCallback[0])); ++i)
    {
        CFE_TIME_TaskData.SynchCallback[i].Ptr = NULL;
    }

    /*
    ** Initialize housekeeping packet (clear user data area)...
    */
    CFE_SB_InitMsg(&CFE_TIME_TaskData.HkPacket,
                    CFE_TIME_HK_TLM_MID,
                    sizeof(CFE_TIME_TaskData.HkPacket), TRUE);

    /*
    ** Initialize diagnostic packet (clear user data area)...
    */
    CFE_SB_InitMsg(&CFE_TIME_TaskData.DiagPacket,
                    CFE_TIME_DIAG_TLM_MID,
                    sizeof(CFE_TIME_TaskData.DiagPacket), TRUE);

    /*
    ** Initialize "time at the tone" signal command packet...
    */
    CFE_SB_InitMsg(&CFE_TIME_TaskData.ToneSignalCmd,
                    CFE_TIME_TONE_CMD_MID,
                    sizeof(CFE_TIME_TaskData.ToneSignalCmd), TRUE);

    /*
    ** Initialize "time at the tone" data command packet...
    */
    #if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
    CFE_SB_InitMsg(&CFE_TIME_TaskData.ToneDataCmd,
                    CFE_TIME_DATA_CMD_MID,
                    sizeof(CFE_TIME_TaskData.ToneDataCmd), TRUE);
    #endif

    /*
    ** Initialize simulated tone send message ("fake tone" mode only)...
    */
#if (CFE_MISSION_TIME_CFG_FAKE_TONE == TRUE)
    CFE_SB_InitMsg(&CFE_TIME_TaskData.ToneSendCmd,
                    CFE_TIME_SEND_CMD_MID,
                    sizeof(CFE_TIME_TaskData.ToneSendCmd), TRUE);
#endif

    /*
    ** Initialize local 1Hz "wake-up" command packet (optional)...
    */
    CFE_SB_InitMsg(&CFE_TIME_TaskData.Local1HzCmd,
                    CFE_TIME_1HZ_CMD_MID,
                    sizeof(CFE_TIME_TaskData.Local1HzCmd), TRUE);

    return;

} /* End of CFE_TIME_InitData() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_GetStateFlags() -- Convert state data to flag values   */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

uint16 CFE_TIME_GetStateFlags(void)
{
    uint16 StateFlags = 0;

    /*
    ** Spacecraft time has been set...
    */
    if (CFE_TIME_TaskData.ClockSetState == CFE_TIME_SetState_WAS_SET)
    {
        StateFlags |= CFE_TIME_FLAG_CLKSET;
    }
    /*
    ** This instance of Time Service is in FLYWHEEL mode...
    */
    if (CFE_TIME_TaskData.ClockFlyState == CFE_TIME_FlywheelState_IS_FLY)
    {
        StateFlags |= CFE_TIME_FLAG_FLYING;
    }
    /*
    ** Clock source set to "internal"...
    */
    if (CFE_TIME_TaskData.ClockSource == CFE_TIME_SourceSelect_INTERNAL)
    {
        StateFlags |= CFE_TIME_FLAG_SRCINT;
    }
    /*
    ** Clock signal set to "primary"...
    */
    if (CFE_TIME_TaskData.ClockSignal == CFE_TIME_ToneSignalSelect_PRIMARY)
    {
        StateFlags |= CFE_TIME_FLAG_SIGPRI;
    }
    /*
    ** Time Server is in FLYWHEEL mode...
    */
    if (CFE_TIME_TaskData.ServerFlyState == CFE_TIME_FlywheelState_IS_FLY)
    {
        StateFlags |= CFE_TIME_FLAG_SRVFLY;
    }
    /*
    ** This instance of Time Services commanded into FLYWHEEL...
    */
    if (CFE_TIME_TaskData.Forced2Fly)
    {
        StateFlags |= CFE_TIME_FLAG_CMDFLY;
    }
    /*
    ** One time STCF adjustment direction...
    */
    if (CFE_TIME_TaskData.OneTimeDirection == CFE_TIME_AdjustDirection_ADD)
    {
        StateFlags |= CFE_TIME_FLAG_ADDADJ;
    }
    /*
    ** 1 Hz STCF adjustment direction...
    */
    if (CFE_TIME_TaskData.OneHzDirection == CFE_TIME_AdjustDirection_ADD)
    {
        StateFlags |= CFE_TIME_FLAG_ADD1HZ;
    }
    /*
    ** Time Client Latency adjustment direction...
    */
    if (CFE_TIME_TaskData.DelayDirection == CFE_TIME_AdjustDirection_ADD)
    {
        StateFlags |= CFE_TIME_FLAG_ADDTCL;
    }
    /*
    ** This instance of Time Service is a "server"...
    */
    #if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
    StateFlags |= CFE_TIME_FLAG_SERVER;
    #endif

    return(StateFlags);

} /* End of CFE_TIME_GetStateFlags() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_GetHkData() -- Report local housekeeping data          */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void CFE_TIME_GetHkData(const CFE_TIME_Reference_t *Reference)
{

    /*
    ** Get command execution counters...
    */
    CFE_TIME_TaskData.HkPacket.Payload.CommandCounter = CFE_TIME_TaskData.CommandCounter;
    CFE_TIME_TaskData.HkPacket.Payload.CommandErrorCounter = CFE_TIME_TaskData.CommandErrorCounter;

    /*
    ** Current "as calculated" clock state...
    */
    CFE_TIME_TaskData.HkPacket.Payload.ClockStateAPI = (int16) CFE_TIME_CalculateState(Reference);

    /*
    ** Current clock state flags...
    */
    CFE_TIME_TaskData.HkPacket.Payload.ClockStateFlags = CFE_TIME_GetStateFlags();

    /*
    ** Leap Seconds...
    */
    CFE_TIME_TaskData.HkPacket.Payload.LeapSeconds = Reference->AtToneLeapSeconds;

    /*
    ** Current MET and STCF time values...
    */
    CFE_TIME_TaskData.HkPacket.Payload.SecondsMET = Reference->CurrentMET.Seconds;
    CFE_TIME_TaskData.HkPacket.Payload.SubsecsMET = Reference->CurrentMET.Subseconds;

    CFE_TIME_TaskData.HkPacket.Payload.SecondsSTCF = Reference->AtToneSTCF.Seconds;
    CFE_TIME_TaskData.HkPacket.Payload.SubsecsSTCF = Reference->AtToneSTCF.Subseconds;

    /*
    ** 1Hz STCF adjustment values (server only)...
    */
    #if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
    CFE_TIME_TaskData.HkPacket.Payload.Seconds1HzAdj = CFE_TIME_TaskData.OneHzAdjust.Seconds;
    CFE_TIME_TaskData.HkPacket.Payload.Subsecs1HzAdj = CFE_TIME_TaskData.OneHzAdjust.Subseconds;
    #endif

    /*
    ** Time at tone delay values (client only)...
    */
    #if (CFE_PLATFORM_TIME_CFG_CLIENT == TRUE)
    CFE_TIME_TaskData.HkPacket.Payload.SecondsDelay = CFE_TIME_TaskData.AtToneDelay.Seconds;
    CFE_TIME_TaskData.HkPacket.Payload.SubsecsDelay = CFE_TIME_TaskData.AtToneDelay.Subseconds;
    #endif



    return;

} /* End of CFE_TIME_GetHkData() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_GetDiagData() -- Report diagnostics data               */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void CFE_TIME_GetDiagData(void)
{
    CFE_TIME_Reference_t Reference;
    CFE_TIME_SysTime_t TempTime;

    /*
    ** Get reference time values (local time, time at tone, etc.)...
    */
    CFE_TIME_GetReference(&Reference);

    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.AtToneMET, &Reference.AtToneMET);
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.AtToneSTCF, &Reference.AtToneSTCF);
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.AtToneDelay, &Reference.AtToneDelay);
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.AtToneLatch, &Reference.AtToneLatch);

    CFE_TIME_TaskData.DiagPacket.Payload.AtToneLeapSeconds   = Reference.AtToneLeapSeconds;
    CFE_TIME_TaskData.DiagPacket.Payload.ClockStateAPI = CFE_TIME_CalculateState(&Reference);

    /*
    ** Data values that reflect the time (right now)...
    */
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.TimeSinceTone, &Reference.TimeSinceTone);
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.CurrentLatch, &Reference.CurrentLatch);
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.CurrentMET, &Reference.CurrentMET);
    TempTime = CFE_TIME_CalculateTAI(&Reference);
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.CurrentTAI, &TempTime);
    TempTime = CFE_TIME_CalculateUTC(&Reference);
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.CurrentUTC, &TempTime);

    /*
    ** Data values used to define the current clock state...
    */
    CFE_TIME_TaskData.DiagPacket.Payload.ClockSetState  = Reference.ClockSetState;
    CFE_TIME_TaskData.DiagPacket.Payload.ClockFlyState  = Reference.ClockFlyState;
    CFE_TIME_TaskData.DiagPacket.Payload.ClockSource    = CFE_TIME_TaskData.ClockSource;
    CFE_TIME_TaskData.DiagPacket.Payload.ClockSignal    = CFE_TIME_TaskData.ClockSignal;
    CFE_TIME_TaskData.DiagPacket.Payload.ServerFlyState = CFE_TIME_TaskData.ServerFlyState;
    CFE_TIME_TaskData.DiagPacket.Payload.Forced2Fly     = (int16) CFE_TIME_TaskData.Forced2Fly;

    /*
    ** Clock state flags...
    */
    CFE_TIME_TaskData.DiagPacket.Payload.ClockStateFlags = CFE_TIME_GetStateFlags();

    /*
    ** STCF adjustment direction values...
    */
    CFE_TIME_TaskData.DiagPacket.Payload.OneTimeDirection = CFE_TIME_TaskData.OneTimeDirection;
    CFE_TIME_TaskData.DiagPacket.Payload.OneHzDirection   = CFE_TIME_TaskData.OneHzDirection;
    CFE_TIME_TaskData.DiagPacket.Payload.DelayDirection   = CFE_TIME_TaskData.DelayDirection;

    /*
    ** STCF adjustment values...
    */
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.OneTimeAdjust, &CFE_TIME_TaskData.OneTimeAdjust);
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.OneHzAdjust, &CFE_TIME_TaskData.OneHzAdjust);

    /*
    ** Most recent local clock latch values...
    */
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.ToneSignalLatch, &CFE_TIME_TaskData.ToneSignalLatch);
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.ToneDataLatch, &CFE_TIME_TaskData.ToneDataLatch);

    /*
    ** Miscellaneous counters (subject to reset command)...
    */
    CFE_TIME_TaskData.DiagPacket.Payload.ToneMatchCounter  = CFE_TIME_TaskData.ToneMatchCounter;
    CFE_TIME_TaskData.DiagPacket.Payload.ToneMatchErrorCounter = CFE_TIME_TaskData.ToneMatchErrorCounter;
    CFE_TIME_TaskData.DiagPacket.Payload.ToneSignalCounter = CFE_TIME_TaskData.ToneSignalCounter;
    CFE_TIME_TaskData.DiagPacket.Payload.ToneDataCounter   = CFE_TIME_TaskData.ToneDataCounter;
    CFE_TIME_TaskData.DiagPacket.Payload.ToneIntCounter    = CFE_TIME_TaskData.ToneIntCounter;
    CFE_TIME_TaskData.DiagPacket.Payload.ToneIntErrorCounter   = CFE_TIME_TaskData.ToneIntErrorCounter;
    CFE_TIME_TaskData.DiagPacket.Payload.ToneTaskCounter   = CFE_TIME_TaskData.ToneTaskCounter;
    CFE_TIME_TaskData.DiagPacket.Payload.VersionCounter    = CFE_TIME_TaskData.CompleteVersionCounter;
    CFE_TIME_TaskData.DiagPacket.Payload.LocalIntCounter   = CFE_TIME_TaskData.LocalIntCounter;
    CFE_TIME_TaskData.DiagPacket.Payload.LocalTaskCounter  = CFE_TIME_TaskData.LocalTaskCounter;

    /*
    ** Miscellaneous counters (not subject to reset command)...
    */
    CFE_TIME_TaskData.DiagPacket.Payload.VirtualMET = CFE_TIME_TaskData.VirtualMET;

    /*
    ** Time window verification values (converted from micro-secs)...
    **
    ** Regardless of whether the tone follows the time packet, or vice
    **    versa, these values define the acceptable window of time for
    **    the second event to follow the first.  The minimum value may
    **    be as little as zero, and the maximum must be something less
    **    than a second.
    */
    CFE_TIME_TaskData.DiagPacket.Payload.MinElapsed = CFE_TIME_TaskData.MinElapsed;
    CFE_TIME_TaskData.DiagPacket.Payload.MaxElapsed = CFE_TIME_TaskData.MaxElapsed;

    /*
    ** Maximum local clock value (before roll-over)...
    */
    CFE_TIME_Copy(&CFE_TIME_TaskData.DiagPacket.Payload.MaxLocalClock, &CFE_TIME_TaskData.MaxLocalClock);

    /*
    ** Tone signal tolerance limits...
    */
    CFE_TIME_TaskData.DiagPacket.Payload.ToneOverLimit  = CFE_TIME_TaskData.ToneOverLimit;
    CFE_TIME_TaskData.DiagPacket.Payload.ToneUnderLimit = CFE_TIME_TaskData.ToneUnderLimit;

    /*
    ** Reset Area access status...
    */
    CFE_TIME_TaskData.DiagPacket.Payload.DataStoreStatus  = CFE_TIME_TaskData.DataStoreStatus;

    return;

} /* End of CFE_TIME_GetDiagData() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_GetReference() -- get reference data (time at "tone")  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void CFE_TIME_GetReference(CFE_TIME_Reference_t *Reference)
{
    CFE_TIME_SysTime_t TimeSinceTone;
    CFE_TIME_SysTime_t CurrentMET;
    uint32 VersionCounter;

    /*
    ** VersionCounter is incremented when reference data is modified...
    */
    do
    {
        VersionCounter = CFE_TIME_TaskData.CompleteVersionCounter;

        Reference->CurrentLatch = CFE_TIME_LatchClock();

        Reference->AtToneMET    = CFE_TIME_TaskData.AtToneMET;
        Reference->AtToneSTCF   = CFE_TIME_TaskData.AtToneSTCF;
        Reference->AtToneLeapSeconds  = CFE_TIME_TaskData.AtToneLeapSeconds;
        Reference->AtToneDelay  = CFE_TIME_TaskData.AtToneDelay;
        Reference->AtToneLatch  = CFE_TIME_TaskData.AtToneLatch;

        Reference->ClockSetState  = CFE_TIME_TaskData.ClockSetState;
        Reference->ClockFlyState  = CFE_TIME_TaskData.ClockFlyState;

    } while (VersionCounter != CFE_TIME_TaskData.PendingVersionCounter);

    /*
    ** Compute the amount of time "since" the tone...
    */
    if (CFE_TIME_Compare(Reference->CurrentLatch, Reference->AtToneLatch) == CFE_TIME_A_LT_B)
    {
        /*
        ** Local clock has rolled over since last tone...
        */
        TimeSinceTone = CFE_TIME_Subtract(CFE_TIME_TaskData.MaxLocalClock, Reference->AtToneLatch);
        TimeSinceTone = CFE_TIME_Add(TimeSinceTone, Reference->CurrentLatch);
    }
    else
    {
        /*
        ** Normal case -- local clock is greater than latch at tone...
        */
        TimeSinceTone = CFE_TIME_Subtract(Reference->CurrentLatch, Reference->AtToneLatch);
    }

    Reference->TimeSinceTone = TimeSinceTone;

    /*
    ** Add in the MET at the tone...
    */
    CurrentMET = CFE_TIME_Add(TimeSinceTone, Reference->AtToneMET);


    /*
    ** Synchronize "this" time client to the time server...
    */
    #if (CFE_PLATFORM_TIME_CFG_CLIENT == TRUE)
    if (CFE_TIME_TaskData.DelayDirection == CFE_TIME_AdjustDirection_ADD)
    {
        CurrentMET = CFE_TIME_Add(CurrentMET, Reference->AtToneDelay);
    }
    else
    {
        CurrentMET = CFE_TIME_Subtract(CurrentMET, Reference->AtToneDelay);
    }
    #endif

    Reference->CurrentMET = CurrentMET;

    return;

} /* End of CFE_TIME_GetReference() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_CalculateTAI() -- calculate TAI from reference data    */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

CFE_TIME_SysTime_t CFE_TIME_CalculateTAI(const CFE_TIME_Reference_t *Reference)
{
    CFE_TIME_SysTime_t TimeAsTAI;

    TimeAsTAI = CFE_TIME_Add(Reference->CurrentMET, Reference->AtToneSTCF);

    return(TimeAsTAI);

} /* End of CFE_TIME_CalculateTAI() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_CalculateUTC() -- calculate UTC from reference data    */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

CFE_TIME_SysTime_t CFE_TIME_CalculateUTC(const CFE_TIME_Reference_t *Reference)
{
    CFE_TIME_SysTime_t TimeAsUTC;

    TimeAsUTC = CFE_TIME_Add(Reference->CurrentMET, Reference->AtToneSTCF);
    TimeAsUTC.Seconds -= Reference->AtToneLeapSeconds;

    return(TimeAsUTC);

} /* End of CFE_TIME_CalculateUTC() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                         */
/* CFE_TIME_CalculateState() -- determine current time state (per API)     */
/*                                                                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int16 CFE_TIME_CalculateState(const CFE_TIME_Reference_t *Reference)
{
    int16 ClockState;

    /*
    ** Determine the current clock state...
    */
    if (Reference->ClockSetState == CFE_TIME_SetState_WAS_SET)
    {
        if (Reference->ClockFlyState == CFE_TIME_FlywheelState_NO_FLY)
        {
            /*
            ** CFE_TIME_ClockState_VALID = clock set and not fly-wheeling...
            */
            ClockState = CFE_TIME_ClockState_VALID;

            /*
            ** If the server is fly-wheel then the client must also
            **    report fly-wheel (even if it is not)...
            */
            #if (CFE_PLATFORM_TIME_CFG_CLIENT == TRUE)
            if (CFE_TIME_TaskData.ServerFlyState == CFE_TIME_FlywheelState_IS_FLY)
            {
                ClockState = CFE_TIME_ClockState_FLYWHEEL;
            }
            #endif
        }
        else
        {
            /*
            ** CFE_TIME_ClockState_FLYWHEEL = clock set and fly-wheeling...
            */
            ClockState = CFE_TIME_ClockState_FLYWHEEL;
        }
    }
    else
    {
        /*
        ** CFE_TIME_ClockState_INVALID = clock not set...
        */
        ClockState = CFE_TIME_ClockState_INVALID;
    }


    return(ClockState);

} /* End of CFE_TIME_CalculateState() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_SetState() -- set clock state                          */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void CFE_TIME_SetState(int16 NewState)
{
    /*
    ** Ensure that change is made without interruption...
    */
    uint32 IntFlags;

    IntFlags = CFE_TIME_StartReferenceUpdate();

    /*
    ** If we get a command to set the clock to "flywheel" mode, then
    **    set a global flag so that we can choose to ignore time
    **    updates until we get another clock state command...
    */
    if (NewState == CFE_TIME_ClockState_FLYWHEEL)
    {
        CFE_TIME_TaskData.Forced2Fly    = TRUE;
        CFE_TIME_TaskData.ClockFlyState = CFE_TIME_FlywheelState_IS_FLY;
        #if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
        CFE_TIME_TaskData.ServerFlyState = CFE_TIME_FlywheelState_IS_FLY;
        #endif
    }
    else if (NewState == CFE_TIME_ClockState_VALID)
    {
        CFE_TIME_TaskData.Forced2Fly    = FALSE;
        CFE_TIME_TaskData.ClockSetState = CFE_TIME_SetState_WAS_SET;
    }
    else
    {
        CFE_TIME_TaskData.Forced2Fly    = FALSE;
        CFE_TIME_TaskData.ClockSetState = CFE_TIME_SetState_NOT_SET;
    }

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_SetState() */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_SetSource() -- set clock source                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if (CFE_PLATFORM_TIME_CFG_SOURCE == TRUE)
void CFE_TIME_SetSource(int16 NewSource)
{
    uint32 IntFlags;
    /*
    ** Ensure that change is made without interruption...
    */
    IntFlags = CFE_TIME_StartReferenceUpdate();

    CFE_TIME_TaskData.ClockSource = NewSource;

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_SetSource() */
#endif /* CFE_PLATFORM_TIME_CFG_SOURCE */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_SetSignal() -- set tone signal (pri vs red)            */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if (CFE_PLATFORM_TIME_CFG_SIGNAL == TRUE)
void CFE_TIME_SetSignal(int16 NewSignal)
{
    uint32 IntFlags;
    /*
    ** Select primary vs redundant tone interrupt signal...
    */
    OS_SelectTone(NewSignal);

    /*
    ** Ensure that change is made without interruption...
    */
    IntFlags = CFE_TIME_StartReferenceUpdate();

    /*
    ** Maintain current tone signal selection for telemetry...
    */
    CFE_TIME_TaskData.ClockSignal = NewSignal;

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_SetSignal() */
#endif /* CFE_PLATFORM_TIME_CFG_SIGNAL */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_SetDelay() -- set tone delay (time client only)        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if (CFE_PLATFORM_TIME_CFG_CLIENT == TRUE)
void CFE_TIME_SetDelay(CFE_TIME_SysTime_t NewDelay, int16 Direction)
{
    uint32 IntFlags;

    /*
    ** Ensure that change is made without interruption...
    */
    IntFlags = CFE_TIME_StartReferenceUpdate();

    CFE_TIME_TaskData.AtToneDelay = NewDelay;
    CFE_TIME_TaskData.DelayDirection = Direction;

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_SetDelay() */
#endif /* CFE_PLATFORM_TIME_CFG_CLIENT */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_SetTime() -- set time (time server only)               */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
void CFE_TIME_SetTime(CFE_TIME_SysTime_t NewTime)
{
    uint32 IntFlags;

    /*
    ** The input to this function is a time value that includes MET
    **     and STCF.  If the default time format is UTC, the input
    **     time value has had leaps seconds removed from the total.
    */
    CFE_TIME_Reference_t Reference;
    CFE_TIME_SysTime_t NewSTCF;

    /*
    ** Get reference time values (local time, time at tone, etc.)...
    */
    CFE_TIME_GetReference(&Reference);

    /*
    ** Remove current MET from the new time value (leaves STCF)...
    */
    NewSTCF = CFE_TIME_Subtract(NewTime, Reference.CurrentMET);

    /*
    ** Restore leap seconds if default time format is UTC...
    */
    #if (CFE_MISSION_TIME_CFG_DEFAULT_UTC == TRUE)
    NewSTCF.Seconds += Reference.AtToneLeapSeconds;
    #endif

    /*
    ** Ensure that change is made without interruption...
    */
    IntFlags = CFE_TIME_StartReferenceUpdate();

    CFE_TIME_TaskData.AtToneSTCF = NewSTCF;

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_SetTime() */
#endif /* CFE_PLATFORM_TIME_CFG_SERVER */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_SetMET() -- set MET (time server only)                 */
/*                                                                 */
/* Note: This command will not have lasting effect if configured   */
/*       to get external time of type MET.  Also, there cannot     */
/*       be a local h/w MET and an external MET since both would   */
/*       need to be synchronized to the same tone signal.          */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
void CFE_TIME_SetMET(CFE_TIME_SysTime_t NewMET)
{
    uint32 IntFlags;

    /*
    ** Ensure that change is made without interruption...
    */
    IntFlags = CFE_TIME_StartReferenceUpdate();

    /*
    ** Update reference values used to compute current time...
    */
    CFE_TIME_TaskData.AtToneMET    = NewMET;
    CFE_TIME_TaskData.VirtualMET   = NewMET.Seconds;
    CFE_TIME_TaskData.AtToneLatch  = CFE_TIME_LatchClock();

    /*
    ** Update h/w MET register...
    */
    #if (CFE_PLATFORM_TIME_CFG_VIRTUAL != TRUE)
    OS_SetLocalMET(NewMET.Seconds);
    #endif

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_SetMET() */
#endif /* CFE_PLATFORM_TIME_CFG_SERVER */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_SetSTCF() -- set STCF (time server only)               */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
void CFE_TIME_SetSTCF(CFE_TIME_SysTime_t NewSTCF)
{
    uint32 IntFlags;

    /*
    ** Ensure that change is made without interruption...
    */
    IntFlags = CFE_TIME_StartReferenceUpdate();

    CFE_TIME_TaskData.AtToneSTCF = NewSTCF;

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_SetSTCF() */
#endif /* CFE_PLATFORM_TIME_CFG_SERVER */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_SetLeapSeconds() -- set leap seconds (time server only)      */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
void CFE_TIME_SetLeapSeconds(int16 NewLeaps)
{
    uint32 IntFlags;

    /*
    ** Ensure that change is made without interruption...
    */
    IntFlags = CFE_TIME_StartReferenceUpdate();

    CFE_TIME_TaskData.AtToneLeapSeconds = NewLeaps;

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_SetLeapSeconds() */
#endif /* CFE_PLATFORM_TIME_CFG_SERVER */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_SetAdjust() -- one time STCF adjustment (server only)  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
void CFE_TIME_SetAdjust(CFE_TIME_SysTime_t NewAdjust, int16 Direction)
{
    uint32 IntFlags;
    CFE_TIME_SysTime_t NewSTCF;

    /*
    ** Ensure that change is made without interruption...
    */
    IntFlags = CFE_TIME_StartReferenceUpdate();

    CFE_TIME_TaskData.OneTimeAdjust    = NewAdjust;
    CFE_TIME_TaskData.OneTimeDirection = Direction;

    if (Direction == CFE_TIME_AdjustDirection_ADD)
    {
        NewSTCF = CFE_TIME_Add(CFE_TIME_TaskData.AtToneSTCF, NewAdjust);
    }
    else
    {
        NewSTCF = CFE_TIME_Subtract(CFE_TIME_TaskData.AtToneSTCF, NewAdjust);
    }

    CFE_TIME_TaskData.AtToneSTCF = NewSTCF;

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_SetAdjust() */
#endif /* CFE_PLATFORM_TIME_CFG_SERVER */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_Set1HzAdj() -- 1Hz STCF adjustment (time server only)  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if (CFE_PLATFORM_TIME_CFG_SERVER == TRUE)
void CFE_TIME_Set1HzAdj(CFE_TIME_SysTime_t NewAdjust, int16 Direction)
{
    uint32 IntFlags;

    /*
    ** Ensure that change is made without interruption...
    */
    IntFlags = CFE_TIME_StartReferenceUpdate();

    /*
    ** Store values for 1Hz adjustment...
    */
    CFE_TIME_TaskData.OneHzAdjust     = NewAdjust;
    CFE_TIME_TaskData.OneHzDirection  = Direction;

    /*
    ** Time has changed, force anyone reading time to retry...
    */
    CFE_TIME_FinishReferenceUpdate(IntFlags);

    return;

} /* End of CFE_TIME_Set1HzAdj() */
#endif /* CFE_PLATFORM_TIME_CFG_SERVER */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* CFE_TIME_CleanUpApp() -- Free resources associated with App     */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int32 CFE_TIME_CleanUpApp(uint32 AppId)
{
    int32 Status;

    if (AppId < (sizeof(CFE_TIME_TaskData.SynchCallback) / sizeof(CFE_TIME_TaskData.SynchCallback[0])))
    {
        CFE_TIME_TaskData.SynchCallback[AppId].Ptr = NULL;
        Status = CFE_SUCCESS;
    }
    else
    {
        Status = CFE_TIME_CALLBACK_NOT_REGISTERED;
    }
    
    return Status;
}

/************************/
/*  End of File Comment */
/************************/

