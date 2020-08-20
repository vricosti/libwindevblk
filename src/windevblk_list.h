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
#ifndef _WINDEVBLK_LIST
#define _WINDEVBLK_LIST
#pragma once

#include "windevblk.h"

// Forward declarations
typedef struct _SMI_RAWDEV_ENTRY SMI_RAWDEV_ENTRY, *PSMI_RAWDEV_ENTRY;


//typedef struct _SMI_DRIVE_INFO
//{
//    DWORD u32DeviceNumber;
//    CHAR szSerialNumber[1000];
//    CHAR szModelNumber[1000];
//    CHAR szVendorId[1000];
//    CHAR szProductRevision[1000];
//    CHAR szDevicePath[1000];
//    CHAR szShortDevicePath[MAX_PATH + 1];
//    int	canBePartitioned;
//    DISK_GEOMETRY_EX diskGeometry;
//    DRIVE_LAYOUT_INFORMATION_EX LayoutInfo;
//} SMI_DRIVE_INFO, *PSMI_DRIVE_INFO;



typedef struct _SMI_DEVBLK_ENTRY
{
    PSMI_RAWDEV_ENTRY RawDevice;
    DWORD DeviceNumber;
    DWORD PartitionNumber;
    BOOLEAN RewritePartition;
    PARTITION_STYLE PartitionStyle;
    LARGE_INTEGER PartitionLength;
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER EndingOffset;
    union {
        PARTITION_INFORMATION_MBR Mbr;
        PARTITION_INFORMATION_GPT Gpt;
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
}	SMI_DEVBLK_ENTRY, *PSMI_DEVBLK_ENTRY;


typedef struct _DEVBLK_OBJECT
{
	SHORT Type;
	SHORT Size;
    PSMI_DEVBLK_ENTRY DevBlk;
    HANDLE DriveHandle;
    LARGE_INTEGER CurAbsPos;    // Current position from start of drive (in bytes)
	LARGE_INTEGER CurRelPos;    // Current position from start of partition (in bytes)
    LIST_ENTRY ListEntry;

} DEVBLK_OBJECT, *PDEVBLK_OBJECT;

//typedef struct _DEVBLK_OBJECTS {
//	LIST_ENTRY ListEntry;
//	PDEVBLK_OBJECT DevObject;
//
//} DEVBLK_OBJECTS, *PDEVBLK_OBJECTS;

typedef struct _SMI_RAWDEV_ENTRY
{
	SMI_DRIVE_INFO DriveInfo;
    SMI_DEVBLK_ENTRY Partitions[128+1];
    LIST_ENTRY DevBlkObjList;

}	SMI_RAWDEV_ENTRY, *PSMI_RAWDEV_ENTRY;


PDEVBLK_OBJECT GetDevBlkObjectPtr(HDEVBLK hDevBlk);

PSMI_RAWDEV_ENTRY GetRawDevEntry(UINT iDevBlkNumber);
PSMI_DEVBLK_ENTRY GetDevBlkEntry(UINT iDevBlkNumber, UINT iPartitionNumber);
DWORD GetDevBlkEntryCount(VOID);
DWORD GetDevBlkCount(PDWORD pRawDevCount);

// Posix stuff
BOOL GetDevBlkInfoFromPosixPath(const char* dev, UINT* iDevBlkNumber, UINT* iPartitionNumber);

#endif 