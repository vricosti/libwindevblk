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
#ifndef _WINDEVBLK_
#define _WINDEVBLK_
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winioctl.h>

#include <sys/types.h>  
#include <sys/stat.h>  

#ifndef HAVE_POSIX4MSVC
#ifndef mode_t
typedef unsigned short mode_t;
#endif //!mode_t
#endif !HAVE_POSIX4MSVC

#ifdef  __cplusplus
extern "C" {
#endif

    typedef void* HDEVBLK;

    /* 
        Structs below mirrors corresponding windows structure found inside ddk
    */
    typedef struct _SMI_DISK_GEOMETRY {
            LARGE_INTEGER Cylinders;
            UINT MediaType;
            DWORD TracksPerCylinder;
            DWORD SectorsPerTrack;
            DWORD BytesPerSector;

    } SMI_DISK_GEOMETRY, * PSMI_DISK_GEOMETRY;

    typedef enum SMI_PARTITION_STYLE {
        SMI_PARTITION_STYLE_MBR,
        SMI_PARTITION_STYLE_GPT,
        SMI_PARTITION_STYLE_RAW
    } SMI_PARTITION_STYLE;

    typedef struct _SMI_DISK_GEOMETRY_EX {
        SMI_DISK_GEOMETRY Geometry;
        LARGE_INTEGER DiskSize;                                 
        BYTE  Data[1];                                                  
    } SMI_DISK_GEOMETRY_EX, * PSMI_DISK_GEOMETRY_EX;

    typedef struct SMI_DRIVE_LAYOUT_INFORMATION_MBR {
            DWORD Signature;
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
        DWORD CheckSum;
#endif
    } SMI_DRIVE_LAYOUT_INFORMATION_MBR, * PSMI_DRIVE_LAYOUT_INFORMATION_MBR;

    typedef struct SMI_DRIVE_LAYOUT_INFORMATION_GPT {
            
        GUID DiskId;
        LARGE_INTEGER StartingUsableOffset;
        LARGE_INTEGER UsableLength;
        DWORD MaxPartitionCount;

    } SMI_DRIVE_LAYOUT_INFORMATION_GPT, * PSMI_DRIVE_LAYOUT_INFORMATION_GPT;

    typedef struct SMI_PARTITION_INFORMATION_MBR {

            BYTE  PartitionType;
            BOOLEAN BootIndicator;
            BOOLEAN RecognizedPartition;
            DWORD HiddenSectors;

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
            GUID PartitionId;
#endif

    } SMI_PARTITION_INFORMATION_MBR, * PSMI_PARTITION_INFORMATION_MBR;

    typedef struct SMI_PARTITION_INFORMATION_GPT {

            GUID PartitionType;                 // Partition type. See table 16-3.
            GUID PartitionId;                   // Unique GUID for this partition.
            DWORD64 Attributes;                 // See table 16-4.
            WCHAR Name[36];                    // Partition Name in Unicode.

    } SMI_PARTITION_INFORMATION_GPT, * PSMI_PARTITION_INFORMATION_GPT;


    typedef struct SMI_PARTITION_INFORMATION_EX {

            PARTITION_STYLE PartitionStyle;
            LARGE_INTEGER StartingOffset;
            LARGE_INTEGER PartitionLength;
            DWORD PartitionNumber;
            BOOLEAN RewritePartition;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS3)
            BOOLEAN IsServicePartition;
#endif
        union {

            SMI_PARTITION_INFORMATION_MBR Mbr;
            SMI_PARTITION_INFORMATION_GPT Gpt;

        } DUMMYUNIONNAME;

    } SMI_PARTITION_INFORMATION_EX, * PSMI_PARTITION_INFORMATION_EX;

    typedef struct SMI_DRIVE_LAYOUT_INFORMATION_EX {
            DWORD PartitionStyle;
            DWORD PartitionCount;

        union {
            SMI_DRIVE_LAYOUT_INFORMATION_MBR Mbr;
            SMI_DRIVE_LAYOUT_INFORMATION_GPT Gpt;

        } DUMMYUNIONNAME;
        
        SMI_PARTITION_INFORMATION_EX PartitionEntry[1];

    } SMI_DRIVE_LAYOUT_INFORMATION_EX, * PSMI_DRIVE_LAYOUT_INFORMATION_EX;

    typedef struct _SMI_DRIVE_INFO
    {
        DWORD u32DeviceNumber;
        CHAR szSerialNumber[1000];
        CHAR szModelNumber[1000];
        CHAR szVendorId[1000];
        CHAR szProductRevision[1000];
        CHAR szDevicePath[1000];
        CHAR szShortDevicePath[MAX_PATH + 1];
        int	canBePartitioned;
        SMI_DISK_GEOMETRY_EX diskGeometry;
        SMI_DRIVE_LAYOUT_INFORMATION_EX LayoutInfo;
    } SMI_DRIVE_INFO, * PSMI_DRIVE_INFO;


    typedef struct _SMI_DEVBLK_INFO
    {
        PVOID Unused;
        DWORD DeviceNumber;
        DWORD PartitionNumber;
        BOOLEAN RewritePartition;
        INT PartitionStyle;
        LARGE_INTEGER PartitionLength;
        LARGE_INTEGER StartingOffset;
        LARGE_INTEGER EndingOffset;
        union {
            SMI_PARTITION_INFORMATION_MBR Mbr;
            SMI_PARTITION_INFORMATION_GPT Gpt;
        } DUMMYUNIONNAME;

        // Posix info
        CHAR    PosixName[64];    // ex sda2
        CHAR    PosixPath[64];    // ex /dev/sda2
        UINT    RootDev;          // Major/Minor

        // Volume info
        CHAR    szRootPathName[MAX_PATH + 1];
        CHAR	szVolumeName[MAX_PATH + 1];
        CHAR	szVolumePathName[MAX_PATH + 1];
        CHAR	szFileSystemName[MAX_PATH + 1];
        DWORD	dwSerialNumber;
        DWORD	dwFileSystemFlags;
    }	SMI_DEVBLK_INFO, *PSMI_DEVBLK_INFO;

    ///////////////////////////////////////////////////////////////////////////////
    // Note: Work in progress to replace  DevBlkEnumDrives and DevBlkEnumDevices
    // typedef void* HDEVBLKSET;
    // HDEVBLKSET GetDevBlkDrives();
    // BOOL DevBlkEnumDrive(HDEVBLKSET devBlkset, PSMI_ENUMDRIVE_INFO pDriveInfo, DWORD dwIndex);
    // BOOL DestroyDevBlkDrives(HDEVBLKSET devBlkset);
    // Finally I am doing something simpler (see DevBlkEnumDriveInfo)
    ///////////////////////////////////////////////////////////////////////////////
    DWORD WINAPI DevBlkGetDriveCount();
    
    BOOL WINAPI DevBlkEnumDriveInfo(
        _In_  DWORD uiIndex,
        _Out_ PSMI_DRIVE_INFO pDriveInfo
    );

    BOOL WINAPI DevBlkEnumDevBlkInfo(
        _In_  DWORD dwDeviceNumber,
        _In_  DWORD uiDevBlkIndex,
        _Out_ PSMI_DEVBLK_INFO pDevBlkInfo
    );

	HDEVBLK WINAPI DevBlkOpen(
		_In_ UINT iDevBlkNumber, /* iDevBlkNumber is the device number ie 0 opens \\\\.\\PhysicalDrive0 */
		_In_ UINT iPartitionNumber,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode
	);

    // BIG WARNING: Do not call
    HANDLE WINAPI DevBlkGetDiskHandle(
        _In_      HDEVBLK        hDevBlk
    );

    HDEVBLK WINAPI DevBlkFromDiskHandle(
        _In_      HANDLE        hDrive);

	BOOL WINAPI DevBlkIsValidHandle(
		_In_      HDEVBLK        hDevBlk
	);

    BOOL WINAPI DevBlkGetFileSizeEx(
        _In_  HDEVBLK        hFile,
        _Out_ PLARGE_INTEGER lpFileSize
    );

   DWORD WINAPI DevBlkSetPointer(
        _In_ HDEVBLK hDevBlk,
        _In_ LONG lDistanceToMove,
        _Inout_opt_ PLONG lpDistanceToMoveHigh,
        _In_ DWORD dwMoveMethod
        );

	BOOL WINAPI DevBlkSetPointerEx(
		_In_      HDEVBLK        hDevBlk,
		_In_      LARGE_INTEGER  liDistanceToMove,
		_Out_opt_ PLARGE_INTEGER lpNewFilePointer,
		_In_      DWORD          dwMoveMethod
	);

	BOOL WINAPI DevBlkRead(
		_In_        HDEVBLK      hDevBlk,
		_Out_       LPVOID       lpBuffer,
		_In_        DWORD        nNumberOfBytesToRead,
		_Out_opt_   LPDWORD      lpNumberOfBytesRead,
        _Inout_opt_ LPOVERLAPPED lpOverlapped
	);

	BOOL WINAPI DevBlkWrite(
		_In_        HDEVBLK      hDevBlk,
		_In_        LPCVOID      lpBuffer,
		_In_        DWORD        nNumberOfBytesToWrite,
		_Out_opt_   LPDWORD      lpNumberOfBytesWritten
	);

	BOOL WINAPI DevBlkClose(
		_In_ HDEVBLK hObject
	);

#ifdef  __cplusplus
}
#endif






#endif /*_WINDEVBLK_*/