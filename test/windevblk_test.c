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

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <tchar.h>

#include "windevblk.h"
#include "windevblk_posix.h"

BOOL GUIDToString(GUID* pguid, TCHAR* pBuffer, UINT size);

BOOL GUIDToString(GUID* pguid, TCHAR* pBuffer, UINT size)
{
    BOOL bRet = FALSE;

    if (!pBuffer || size < 39)
        return FALSE;

    _sntprintf(pBuffer, size, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        pguid->Data1, pguid->Data2, pguid->Data3,
        pguid->Data4[0], pguid->Data4[1], pguid->Data4[2], pguid->Data4[3],
        pguid->Data4[4], pguid->Data4[5], pguid->Data4[6], pguid->Data4[7]);

    return bRet;
}

int main()
{
    BOOL bRet;
    DWORD dwReadBytes;
    LARGE_INTEGER lSize, lDist,lPos;
    TCHAR tszGUID[39];

    UINT uiCount = DevBlkGetDriveCount();
    if (uiCount > 0)
    {
        for (UINT i = 0; i < uiCount; i++)
        {
            SMI_DRIVE_INFO smiDriveInfo;
            bRet = DevBlkEnumDriveInfo(i, &smiDriveInfo);
            printf("Drive %d:\n", smiDriveInfo.u32DeviceNumber);
            printf("   SerialNumber: %s\n", smiDriveInfo.szSerialNumber);
            printf("   ModelNumber: %s\n", smiDriveInfo.szModelNumber);
            printf("   VendorId: %s\n", smiDriveInfo.szVendorId);
            printf("   ProductRevision: %s\n", smiDriveInfo.szProductRevision);
            printf("   DevicePath: %s\n", smiDriveInfo.szDevicePath);
            printf("   ShortDevicePath: %s\n", smiDriveInfo.szShortDevicePath);
            printf("   canBePartitioned: %d\n", smiDriveInfo.canBePartitioned);
            printf("   PartitionStyle: %d\n", smiDriveInfo.LayoutInfo.PartitionStyle);
            printf("   PartitionCount: %d\n", smiDriveInfo.LayoutInfo.PartitionCount);
            if (smiDriveInfo.LayoutInfo.PartitionStyle == SMI_PARTITION_STYLE_MBR)
            {
                printf("   Mbr.CheckSum: 0x%x\n", smiDriveInfo.LayoutInfo.Mbr.CheckSum);
                printf("   Mbr.Signature: 0x%x\n", smiDriveInfo.LayoutInfo.Mbr.Signature);

            }
            else if (smiDriveInfo.LayoutInfo.PartitionStyle == SMI_PARTITION_STYLE_GPT)
            {
                GUIDToString(&(smiDriveInfo.LayoutInfo.Gpt.DiskId), tszGUID, sizeof(tszGUID)/sizeof(TCHAR));
                printf("   Gpt.DiskId: %s\n", tszGUID);
                printf("   Gpt.StartingUsableOffset: %lld\n", smiDriveInfo.LayoutInfo.Gpt.StartingUsableOffset.QuadPart);
                printf("   Gpt.UsableLength: %lld\n", smiDriveInfo.LayoutInfo.Gpt.UsableLength.QuadPart);
                printf("   Gpt.MaxPartitionCount: %d\n", smiDriveInfo.LayoutInfo.Gpt.MaxPartitionCount);
                
            }
            else
            {
                printf("   Raw partition... Nothing to say:\n");
            }

            // Now Display partition info
            for (DWORD i = 0; i <= smiDriveInfo.LayoutInfo.PartitionCount; i++)
            {
                SMI_DEVBLK_INFO devBlkInfo;
                bRet = DevBlkEnumDevBlkInfo(smiDriveInfo.u32DeviceNumber, i, &devBlkInfo);
                if (bRet)
                {
                    printf("   *Partition %d (PosixPath = %s):\n", i, devBlkInfo.PosixPath);

                    printf("      PartitionLength: %lld\n", devBlkInfo.PartitionLength.QuadPart);
                    printf("      StartingOffset: %lld\n", devBlkInfo.StartingOffset.QuadPart);
                    printf("      EndingOffset: %lld\n", devBlkInfo.EndingOffset.QuadPart);
                    if (devBlkInfo.PartitionStyle == SMI_PARTITION_STYLE_MBR)
                    {
                        printf("      Mbr.BootIndicator: 0x%x\n", devBlkInfo.Mbr.BootIndicator);
                        printf("      Mbr.RecognizedPartition: %d\n", devBlkInfo.Mbr.RecognizedPartition);
                        GUIDToString(&(devBlkInfo.Mbr.PartitionId), tszGUID, sizeof(tszGUID) / sizeof(TCHAR));
                        printf("      Mbr.PartitionId: %s\n", tszGUID);

                    }
                    else if (devBlkInfo.PartitionStyle == SMI_PARTITION_STYLE_GPT)
                    {
                        GUIDToString(&(devBlkInfo.Gpt.PartitionType), tszGUID, sizeof(tszGUID) / sizeof(TCHAR));
                        printf("      Gpt.PartitionType: %s\n", tszGUID);
                        GUIDToString(&(devBlkInfo.Gpt.PartitionId), tszGUID, sizeof(tszGUID) / sizeof(TCHAR));
                        printf("      Gpt.PartitionId: %s\n", tszGUID);
                    }
                    else
                    {

                    }
                    //printf("      PosixPath %s:\n", devBlkInfo.PosixPath);
                    printf("      szFileSystemName: %s\n", devBlkInfo.szFileSystemName);
                    printf("      szRootPathName: %s\n", devBlkInfo.szRootPathName);
                    printf("      szVolumeName: %s\n", devBlkInfo.szVolumeName);
                    printf("      szVolumePathName: %s\n", devBlkInfo.szVolumePathName);
                    
                }
            }
        }
    }
    

    char* buffer = calloc(100000000, 1);
    HDEVBLK hDevBlk = DevBlkOpen(0, 1, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (hDevBlk != NULL)
    {
        HDEVBLK hDevBlk2 = DevBlkOpen(0, 1, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE);
        HDEVBLK hDevBlk3 = DevBlkOpen(0, 1, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE);
        bRet = DevBlkIsValidHandle(hDevBlk);

        HANDLE hDisk = DevBlkGetDiskHandle(hDevBlk2);
        
        bRet = DevBlkGetFileSizeEx(hDevBlk2, &lSize);
        hDisk = DevBlkGetDiskHandle(hDevBlk2);
        HDEVBLK hDevtest = DevBlkFromDiskHandle(hDisk);


        // First try to get partition size
        lDist.QuadPart = 0;
        DevBlkSetPointerEx(hDevBlk, lDist, &lPos, FILE_END);
        
        // try to move beyond the partition
        lDist.QuadPart = 1;
        DevBlkSetPointerEx(hDevBlk, lDist, &lPos, FILE_CURRENT);
        
        
        // go back to begining
        lDist.QuadPart = 0;
        DevBlkSetPointerEx(hDevBlk, lDist, &lPos, FILE_BEGIN);

        

        bRet = DevBlkRead(hDevBlk, buffer, 0, &dwReadBytes, NULL);

        // go back to begining
        lDist.QuadPart = 0;
        DevBlkSetPointerEx(hDevBlk, lDist, &lPos, FILE_BEGIN);

        DWORD dwLen = 26214400;
        bRet = DevBlkRead(hDevBlk, buffer, dwLen, &dwReadBytes, NULL);

        bRet = DevBlkRead(hDevBlk, buffer+ dwLen, dwLen, &dwReadBytes, NULL);

        bRet = DevBlkRead(hDevBlk, buffer, 1, &dwReadBytes, NULL);
        
        lDist.QuadPart = 0;
        DevBlkSetPointerEx(hDevBlk, lDist, &lPos, FILE_CURRENT);

        DevBlkClose(hDevBlk);
    }
    free(buffer);

    // testing posix
    int fd = devblk_open("/dev/sdb", 0, 0);
    if (fd != -1)
        devblk_close(fd);


    printf("\nPress a key to exit\n");
    _getch();

    return 0;
}


