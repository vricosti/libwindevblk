////////////////////////////////////////////////////////////
//
// libwindevblk - Library to read/write partitions
// Copyright (C) 2020 Vincent Richomme
//
// Licensed to the Apache Software Foundation(ASF) under one
// or more contributor license agreements.See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// 	"License"); you may not use this file except in compliance
// 	with the License.You may obtain a copy of the License at
// 
// 	http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.See the License for the
// specific language governing permissionsand limitations
// under the License.
//
////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include "windevblk.h"
#include "windevblk_list.h"
#include "list.h"

#include "get_osfhandle-nothrow.h"

// forward declarations
extern BOOL CreateOnceMsgLoop(void);
extern void BuildDeviceList(void);
extern UINT GetRawDevCount(VOID);
BOOL WINAPI DevBlkInit(void);

extern PSMI_RAWDEV_ENTRY g_devblk_array[128];
static CRITICAL_SECTION g_critSec;

static BOOL g_bIsInited = FALSE;
BOOL WINAPI DevBlkInit(void)
{
	BOOL bRet = FALSE;

    InitializeCriticalSection(&g_critSec);
    if (!g_bIsInited)
    {
        // The first time DevBlkOpen will be called it will create a msg loop 
        // to receive notifications when a usb key is added/removed
        g_bIsInited = TRUE;
        CreateOnceMsgLoop();
        BuildDeviceList();
    }
    DeleteCriticalSection(&g_critSec);
	
    return bRet;
}

DWORD DevBlkGetDriveCount()
{
    DWORD uiCount, uiRawDevCount;
    
    DevBlkInit();

    uiRawDevCount = 0;
    uiCount = GetDevBlkCount(&uiRawDevCount);
    return uiRawDevCount;
}

BOOL DevBlkEnumDriveInfo(
    _In_  DWORD uiIndex,
    _Out_ PSMI_DRIVE_INFO pDriveInfo
)
{
    BOOL bRet = FALSE;

    DWORD driveCount = DevBlkGetDriveCount();
    if (uiIndex > driveCount - 1)
        return FALSE;

    PSMI_RAWDEV_ENTRY pRawDevEntry = GetRawDevEntry(uiIndex);
    if (pRawDevEntry)
    {
        memcpy(pDriveInfo, &(pRawDevEntry->DriveInfo), sizeof(SMI_DRIVE_INFO));
        bRet = TRUE;
    }

    return bRet;
}

BOOL DevBlkEnumDevBlkInfo(
    _In_ DWORD dwDeviceNumber,
    _In_ DWORD uiDevBlkIndex,
    _Out_ PSMI_DEVBLK_INFO pDevBlkInfo)
{
    BOOL bRet = FALSE;

    DWORD driveCount = DevBlkGetDriveCount();
    if (dwDeviceNumber > driveCount - 1)
        return FALSE;

    PSMI_DEVBLK_ENTRY pDevBlkEntry = GetDevBlkEntry(dwDeviceNumber, uiDevBlkIndex);
    if (pDevBlkEntry)
    {
        if (pDevBlkInfo)
        {
            memcpy(pDevBlkInfo, pDevBlkEntry, sizeof(SMI_DEVBLK_ENTRY));
        }
        bRet = TRUE;
    }


    return bRet;
}

PDEVBLK_OBJECT GetDevBlkObjectPtr(HDEVBLK hDevBlk)
{
    PDEVBLK_OBJECT devBlkObj = NULL;

    __try
    {
        PDEVBLK_OBJECT devBlkObjTmp = (PDEVBLK_OBJECT)hDevBlk;
        if (devBlkObjTmp)
        {
            if ((devBlkObjTmp->Size == sizeof(DEVBLK_OBJECT)) && 
                (devBlkObjTmp->Type == 0))
            {
                devBlkObj = devBlkObjTmp;
            }
        }
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        devBlkObj = NULL;
    }

    return devBlkObj;
}

HDEVBLK WINAPI DevBlkOpen(
	_In_ UINT iDevBlkNumber,
	_In_ UINT iPartitionNumber,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode
)
{
    PDEVBLK_OBJECT devBlkObj = NULL;
    PSMI_DEVBLK_ENTRY devBlkEntry;
    HANDLE hDev;
    LARGE_INTEGER newPos;

    DevBlkInit();
    devBlkEntry = GetDevBlkEntry(iDevBlkNumber, iPartitionNumber);
    if (devBlkEntry)
    {
        hDev = CreateFile(devBlkEntry->RawDevice->DriveInfo.szShortDevicePath,
            dwDesiredAccess,
            dwShareMode,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        if (hDev != INVALID_HANDLE_VALUE)
        {
            //PPARTITION_INFORMATION_EX pPartInfo = (&devBlkEntry->PartitionInfo);
            if (SetFilePointerEx(hDev, devBlkEntry->StartingOffset, &newPos, FILE_BEGIN))
            {
                // MAXIMUM_CHECK: if everything goes well the new position is the start position
                if (newPos.QuadPart == devBlkEntry->StartingOffset.QuadPart)
                {
                    //TODO: Acquire the spinlock to insert into our list of devblk objects
                    InitializeCriticalSection(&g_critSec);
                    devBlkObj = calloc(1, sizeof(DEVBLK_OBJECT));
                    if (devBlkObj)
                    {
                        devBlkObj->Type = 0;
                        devBlkObj->Size = sizeof(DEVBLK_OBJECT);
                        devBlkObj->DevBlk = devBlkEntry;
                        devBlkObj->DriveHandle = hDev;
                        devBlkObj->CurAbsPos = newPos;
                        devBlkObj->CurRelPos.QuadPart = newPos.QuadPart - devBlkEntry->StartingOffset.QuadPart;
                        
                        //Insert in our list of devblk handles
                        InsertTailList(&(devBlkEntry->RawDevice->DevBlkObjList), &(devBlkObj->ListEntry));
                    }
                    DeleteCriticalSection(&g_critSec);

                }
            }
        }

    }


	return ((HDEVBLK)devBlkObj);
}

HDEVBLK WINAPI DevBlkFromDiskHandle(
    _In_      HANDLE        hDrive)
{
    HDEVBLK hDevBlk = NULL;
    BOOL bFound = FALSE;

    if (hDrive != INVALID_HANDLE_VALUE)
    {
        InitializeCriticalSection(&g_critSec);

        for (int iRawDevNumber = 0; (iRawDevNumber < 128) && (!bFound); iRawDevNumber++)
        {
            PSMI_RAWDEV_ENTRY pRawDev = GetRawDevEntry(iRawDevNumber);
            if (pRawDev)
            {
                PLIST_ENTRY pEntry = pRawDev->DevBlkObjList.Flink;
                while (pEntry != &(pRawDev->DevBlkObjList))
                {
                    PDEVBLK_OBJECT pDevBlkObj;
                    pDevBlkObj = (PDEVBLK_OBJECT)CONTAINING_RECORD(pEntry, DEVBLK_OBJECT, ListEntry);
                    if (pDevBlkObj->DriveHandle == hDrive)
                    {
                        hDevBlk = (HDEVBLK)pDevBlkObj;
                        bFound = TRUE;
                        break;
                    }
                    pEntry = pEntry->Flink;
                }
            }
        }

        DeleteCriticalSection(&g_critSec);
    }

    return hDevBlk;
}

HANDLE WINAPI DevBlkGetDiskHandle(
    _In_      HDEVBLK        hDevBlk
)
{
    HANDLE hDrive = INVALID_HANDLE_VALUE;

    PDEVBLK_OBJECT devBlkObj = GetDevBlkObjectPtr(hDevBlk);
    if (devBlkObj)
    {
        hDrive = devBlkObj->DriveHandle;
    }

    return hDrive;
}

BOOL WINAPI DevBlkGetFileSizeEx(
    _In_  HDEVBLK        hDevBlk,
    _Out_ PLARGE_INTEGER lpFileSize
)
{
    BOOL bRet = FALSE;

    PDEVBLK_OBJECT devBlkObj = GetDevBlkObjectPtr(hDevBlk);
    if (devBlkObj)
    {
        bRet = TRUE;
        if (lpFileSize)
        {
            *lpFileSize = devBlkObj->DevBlk->PartitionLength;
        }
    }

    return bRet;
}

BOOL WINAPI DevBlkIsValidHandle(HDEVBLK hDevBlk)
{
    BOOL bRet = FALSE;

    PDEVBLK_OBJECT devBlkObj = GetDevBlkObjectPtr(hDevBlk);
    if (devBlkObj)
    {
        InitializeCriticalSection(&g_critSec);
        PLIST_ENTRY pEntry = devBlkObj->DevBlk->RawDevice->DevBlkObjList.Flink;
        while(pEntry != &(devBlkObj->DevBlk->RawDevice->DevBlkObjList))
        {
            PDEVBLK_OBJECT pDevBlkObj;

            pDevBlkObj = (PDEVBLK_OBJECT)CONTAINING_RECORD(pEntry, DEVBLK_OBJECT, ListEntry);
            if (pDevBlkObj == devBlkObj)
            {
                bRet = TRUE;
                break;
            }
            pEntry = pEntry->Flink;
        }
        DeleteCriticalSection(&g_critSec);
    }

    return bRet;
}

DWORD WINAPI DevBlkSetPointer(
    _In_ HDEVBLK hDevBlk,
    _In_ LONG lDistanceToMove,
    _Inout_opt_ PLONG lpDistanceToMoveHigh,
    _In_ DWORD dwMoveMethod)
{
    DWORD dwRet = INVALID_SET_FILE_POINTER;
    LARGE_INTEGER lDist, lNewPos;
    BOOL bRet;

    PDEVBLK_OBJECT devBlkObj = GetDevBlkObjectPtr(hDevBlk);
    if (devBlkObj)
    {
        lDist.LowPart = lDistanceToMove;
        if (lpDistanceToMoveHigh)
            lDist.HighPart = *lpDistanceToMoveHigh;
        bRet = DevBlkSetPointerEx(hDevBlk, lDist, &lNewPos, dwMoveMethod);
        if (bRet)
        {
            dwRet = lNewPos.LowPart;
            if (lpDistanceToMoveHigh)
                *lpDistanceToMoveHigh = lNewPos.HighPart;
        }
    }

    return dwRet;
}

BOOL WINAPI DevBlkSetPointerEx(
    _In_      HDEVBLK        hDevBlk,
    _In_      LARGE_INTEGER  liDistanceToMove,
    _Out_opt_ PLARGE_INTEGER lpNewFilePointer,
    _In_      DWORD          dwMoveMethod)
{
    BOOL bRet = FALSE;
    LARGE_INTEGER lStartOffset, lEndOffset;
    LARGE_INTEGER lDist, lPos;
    
    PDEVBLK_OBJECT devBlkObj = GetDevBlkObjectPtr(hDevBlk);
    if (devBlkObj)
    {
        // First get current position
        lDist.QuadPart = 0;
        bRet = SetFilePointerEx(devBlkObj->DriveHandle, lDist, &lPos, FILE_CURRENT);
        if (bRet)
        {
            // Calculate Start and end position of the current devblk
            //PPARTITION_INFORMATION_EX pPartInfo = &(devBlkObj->DevBlk->PartitionInfo);
            //lStartOffset = devBlkObj->DevBlk->StartingOffset;
            //lEndOffset.QuadPart = pPartInfo->StartingOffset.QuadPart + pPartInfo->PartitionLength.QuadPart - 1;
            lStartOffset = devBlkObj->DevBlk->StartingOffset;
            lEndOffset = devBlkObj->DevBlk->EndingOffset;

            switch (dwMoveMethod)
            {
            case FILE_BEGIN:
                lDist.QuadPart = lStartOffset.QuadPart + liDistanceToMove.QuadPart;
                break;
            case FILE_CURRENT:
                lDist.QuadPart = lPos.QuadPart + liDistanceToMove.QuadPart;
                break;
            case FILE_END:
                lDist.QuadPart = lEndOffset.QuadPart + liDistanceToMove.QuadPart;
                break;
            default:
                break;
            }

            // We cannot move outside the current partition
            if (lDist.QuadPart < lStartOffset.QuadPart)
            {
                lDist.QuadPart = lStartOffset.QuadPart;
            }
            else if (lDist.QuadPart > lEndOffset.QuadPart)
            {
                lDist.QuadPart = lEndOffset.QuadPart;
            }
            
            // It's ok so goes to the position
            bRet = SetFilePointerEx(devBlkObj->DriveHandle, lDist, &lPos, FILE_BEGIN);
            if (bRet)
            {
                devBlkObj->CurAbsPos = lPos;
                devBlkObj->CurRelPos.QuadPart = lPos.QuadPart - lStartOffset.QuadPart;
                if (lpNewFilePointer)
                {
                    *lpNewFilePointer = devBlkObj->CurRelPos;
                }
            }

            
        }
    }

    return bRet;
}

BOOL WINAPI DevBlkRead(
    _In_        HDEVBLK      hDevBlk,
    _Out_       LPVOID       lpBuffer,
    _In_        DWORD        nNumberOfBytesToRead,
    _Out_opt_   LPDWORD      lpNumberOfBytesRead,
    _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
    BOOL bRet = FALSE;
    PUCHAR pBuffer = lpBuffer;
    PUCHAR pAlignedBuf = NULL;
    BOOL bIsSectorAligned;
    DWORD dwBytesToRead;
    DWORD dwReadBytes;
    LONG lReadBytesDiff;
    DWORD dwBytesPerSector;
    DWORD dwNbSectors;
    LARGE_INTEGER lDist, lPos;
   
    PDEVBLK_OBJECT devBlkObj = GetDevBlkObjectPtr(hDevBlk);
    if (devBlkObj)
    {
        // Check if the wanted value is a multiple of bytesPerSector
        dwBytesPerSector = devBlkObj->DevBlk->RawDevice->DriveInfo.diskGeometry.Geometry.BytesPerSector;
        bIsSectorAligned = ((nNumberOfBytesToRead % dwBytesPerSector) == 0);

        // Check that if we read bytes we don't go outside the current partition
        // We calculate the length between the current pos and the end of the partition
        // and we check nNumberOfBytesToRead is smaller.
        dwBytesToRead = nNumberOfBytesToRead;
        lDist.QuadPart = devBlkObj->DevBlk->EndingOffset.QuadPart - devBlkObj->CurAbsPos.QuadPart;
        if (dwBytesToRead > lDist.QuadPart)
        {
            dwBytesToRead = (DWORD)lDist.QuadPart;
        }

        // If len to read is not a multiple of sector size => we need to read sector aligned
        if (!bIsSectorAligned && (dwBytesToRead > 0))
        {
            dwNbSectors = (dwBytesToRead / dwBytesPerSector) + (bIsSectorAligned ? 0 : 1);
            dwBytesToRead = dwBytesPerSector * dwNbSectors;
            if (lpBuffer)
            {
                pAlignedBuf = malloc(dwBytesToRead);
                if (!pAlignedBuf) { goto ExitOnError; }
                pBuffer = pAlignedBuf;
            }
        }
       
        dwReadBytes = 0;
        bRet = ReadFile(devBlkObj->DriveHandle, pBuffer, dwBytesToRead, &dwReadBytes, lpOverlapped);
        if (bRet)
        {
            // We cannot return to the caller more bytes than he asked 
            // even if in reality we might have read more.
            // It also implies that if we read more bytes we need to set the file pointer back to the 
            // corresponding position
            if (dwBytesToRead > 0)
            {
                if (dwReadBytes > nNumberOfBytesToRead)
                {
                    lReadBytesDiff = nNumberOfBytesToRead - dwReadBytes;
                    // Move back
                    lDist.QuadPart = lReadBytesDiff;
                    bRet = SetFilePointerEx(devBlkObj->DriveHandle, lDist, &lPos, FILE_CURRENT);
                    if (!bRet) { goto ExitOnError; }
                    devBlkObj->CurAbsPos = lPos;
                    devBlkObj->CurRelPos.QuadPart = lPos.QuadPart - devBlkObj->DevBlk->StartingOffset.QuadPart;
                    dwReadBytes = nNumberOfBytesToRead;
                }
                else
                {
                    // Get current position
                    lDist.QuadPart = 0;
                    bRet = SetFilePointerEx(devBlkObj->DriveHandle, lDist, &lPos, FILE_CURRENT);
                    if (!bRet) { goto ExitOnError; }
                    devBlkObj->CurAbsPos = lPos;
                    devBlkObj->CurRelPos.QuadPart = lPos.QuadPart - devBlkObj->DevBlk->StartingOffset.QuadPart;
                }
            }


            if (pAlignedBuf && lpBuffer && nNumberOfBytesToRead > 0)
            {
                memcpy(lpBuffer, pAlignedBuf, dwReadBytes);
            }

            if (lpNumberOfBytesRead)
            {
                *lpNumberOfBytesRead = dwReadBytes;
            }
        }
    }

ExitOnError:
    if (pAlignedBuf) { free(pAlignedBuf); }
    return bRet;
}

BOOL WINAPI DevBlkWrite(
    _In_        HDEVBLK      hDevBlk,
    _In_        LPCVOID      lpBuffer,
    _In_        DWORD        nNumberOfBytesToWrite,
    _Out_opt_   LPDWORD      lpNumberOfBytesWritten)
{
    BOOL bRet = FALSE;

    

    return bRet;
}

BOOL WINAPI DevBlkClose(_In_ HDEVBLK hDevBlk)
{
    BOOL bRet = FALSE;

    PDEVBLK_OBJECT devBlkObj = GetDevBlkObjectPtr(hDevBlk);
    if (devBlkObj)
    {
        
        bRet = CloseHandle(devBlkObj->DriveHandle);
        if (bRet)
        {
            InitializeCriticalSection(&g_critSec);

            //Remove devblk obj from our list
            PLIST_ENTRY pEntry = devBlkObj->DevBlk->RawDevice->DevBlkObjList.Flink;
            while (pEntry != &(devBlkObj->DevBlk->RawDevice->DevBlkObjList))
            {
                PDEVBLK_OBJECT pDevBlkObj;

                pDevBlkObj = (PDEVBLK_OBJECT)CONTAINING_RECORD(pEntry, DEVBLK_OBJECT, ListEntry);
                if (pDevBlkObj == devBlkObj)
                {
                    bRet = RemoveEntryList(&pDevBlkObj->ListEntry);
                }
                pEntry = pEntry->Flink;
            }

            free(devBlkObj);
            DeleteCriticalSection(&g_critSec);
        }
        
    }

    return bRet;
}
