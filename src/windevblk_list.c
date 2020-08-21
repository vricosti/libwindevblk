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

#include "windevblk_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Strsafe.h>
#include <Setupapi.h>
#include <Ntddstor.h>
//Copy definition of LIST_ENTRY functions from WUDF (UserMode driver)
#include "list.h"
//Windows DDK
#include <Ntddscsi.h>
//#include <ata.h>

static PSMI_RAWDEV_ENTRY g_devblk_array[128];

void BuildDeviceList(void);
BOOL FindVolume(int diskno, PSMI_DEVBLK_ENTRY pDevBlkEntry);
char * flipAndCodeBytes(int iPos, int iFlip, const char * pcszStr, char * pcszBuf);

#define START_ERROR_CHK()           \
    DWORD error = ERROR_SUCCESS;    \
    DWORD failedLine;               \
    TCHAR failedApi[MAX_PATH+1];

#define CHK( expr, api )            \
    if ( !( expr ) ) {              \
        error = GetLastError( );    \
        failedLine = __LINE__;      \
        StringCchCopy(failedApi, MAX_PATH, ( api ) ); \
        goto Error_Exit;            \
    }

#define END_ERROR_CHK()             \
    error = ERROR_SUCCESS;          \
    Error_Exit:                     \
    if ( ERROR_SUCCESS != error ) { \
		printf("%s failed at %lu : Error Code - %lu\n", failedApi, failedLine, error); \
    }




DWORD GetDevBlkCount(PDWORD pRawDevCount)
{
    DWORD uiCount = 0;
    DWORD uiRawDevCount = 0;

    for (DWORD i = 0; i < 128; i++)
    {
        if (g_devblk_array[i])
        {
            uiRawDevCount++;
            for (DWORD j = 0; j < 129; j++)
            {
                if (g_devblk_array[i]->Partitions[j].RawDevice)
                {
                    uiCount++;
                }
            }
        }
    }

    if (pRawDevCount)
        *pRawDevCount = uiRawDevCount;

    return uiCount;
}

DWORD GetRawDevCount(VOID)
{
    DWORD uiCount = 0;

    for (DWORD iDevBlkNumber = 0; iDevBlkNumber < 128; iDevBlkNumber++)
    {
        if (g_devblk_array[iDevBlkNumber])
        {
            uiCount++;
        }
    }
    return uiCount;
}

PSMI_RAWDEV_ENTRY GetRawDevEntry(UINT iDevBlkNumber)
{
    PSMI_RAWDEV_ENTRY pRawDev = NULL;
    if (iDevBlkNumber >= 128)
        return NULL;

    pRawDev = g_devblk_array[iDevBlkNumber];

    return pRawDev;
}


BOOL GetDevBlkInfoFromPosixPath(const char* dev, UINT* piDevBlkNumber, UINT* piPartitionNumber)
{
    BOOL bRet = FALSE;

    if (!dev)
        return FALSE;

    for (DWORD i = 0; (i < 128) && !bRet; i++)
    {
        if (g_devblk_array[i])
        {
            for (DWORD j = 0; j < 129; j++)
            {
                if (g_devblk_array[i]->Partitions[j].RawDevice)
                {
                    if (!strcmp(g_devblk_array[i]->Partitions[j].PosixPath, dev))
                    {
                        bRet = TRUE;

                        if (piDevBlkNumber)
                            *piDevBlkNumber = i;
                        if (piPartitionNumber)
                            *piPartitionNumber = j;
                        break;
                    }
                }
            }
        }
    }

    return bRet;
}

PSMI_DEVBLK_ENTRY GetDevBlkEntry(UINT iDevBlkNumber, UINT iPartitionNumber)
{
    PSMI_DEVBLK_ENTRY pDevBlkEntry = NULL;

    //device count starts from 0
    //partition count starts at 1 and we use 0 for whole drive
    if (/*iDevBlkNumber < 0 ||*/ iDevBlkNumber >= 128)
        return NULL;
    if (/*iPartitionNumber < 0 ||*/ iPartitionNumber >= 129)
        return NULL;

    if (g_devblk_array[iDevBlkNumber])
    {
        if (g_devblk_array[iDevBlkNumber]->Partitions[iPartitionNumber].RawDevice)
        {
            pDevBlkEntry = &(g_devblk_array[iDevBlkNumber]->Partitions[iPartitionNumber]);
        }
    }
    return pDevBlkEntry;
}

#define MINORBITS      20
#define BLK_PARTN_BITS 2
#define BLK_PER_MAJOR (1U << (MINORBITS - BLK_PARTN_BITS))
#define	makedev(x,y)	((unsigned int)(((x)<<8) | (y)))

void SetPosixInfo(PSMI_DEVBLK_ENTRY pDevBlk, int* pMinor)
{
    int len;

    if (pDevBlk->DeviceNumber >= BLK_PER_MAJOR)
        return;

    len = sprintf(pDevBlk->PosixName, "sd");
    if (pDevBlk->DeviceNumber > 25) {
        if (pDevBlk->DeviceNumber > 701) {
            if (pDevBlk->DeviceNumber > 18277)
                len += sprintf(pDevBlk->PosixName + len, "%c",
                    'a' + (((pDevBlk->DeviceNumber - 18278)
                        / 17576) % 26));
            len += sprintf(pDevBlk->PosixName + len, "%c",
                'a' + (((pDevBlk->DeviceNumber - 702) / 676) % 26));
        }
        len += sprintf(pDevBlk->PosixName + len, "%c",
            'a' + (((pDevBlk->DeviceNumber - 26) / 26) % 26));
    }
    len += sprintf(pDevBlk->PosixName + len, "%c", 'a' + (pDevBlk->DeviceNumber % 26));

    if (pDevBlk->PartitionNumber != 0)
        sprintf(pDevBlk->PosixName + len, "%d", pDevBlk->PartitionNumber);
    
    snprintf(pDevBlk->PosixPath, sizeof(pDevBlk->PosixPath) / sizeof(pDevBlk->PosixPath[0]), "/dev/%s", pDevBlk->PosixName);

    pDevBlk->RootDev = makedev(8, *pMinor);
    (*pMinor)++;
}

void BuildDeviceList(void)
{
	HDEVINFO diskClassDevices;
	GUID diskClassDeviceInterfaceGuid = GUID_DEVINTERFACE_DISK;
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	SP_DEVINFO_DATA deviceInfoData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData;
	DWORD requiredSize;
	DWORD deviceIndex;
    DWORD BytesPerSector;
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	STORAGE_DEVICE_NUMBER diskNumber;
	DWORD bytesReturned;
    PSMI_RAWDEV_ENTRY rawDevEntry = NULL;
    int minor;

	START_ERROR_CHK();

	//
	// Get the handle to the device information set for installed
	// disk class devices. Returns only devices that are currently
	// present in the system and have an enabled disk device
	// interface.
	//
	diskClassDevices = SetupDiGetClassDevs(&diskClassDeviceInterfaceGuid,
		NULL,
		NULL,
		DIGCF_PRESENT |
		DIGCF_DEVICEINTERFACE);
	CHK(INVALID_HANDLE_VALUE != diskClassDevices,
		"SetupDiGetClassDevs");

    ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	deviceIndex = 0;
    for (minor = 0; SetupDiEnumDeviceInterfaces(diskClassDevices,
        NULL,
        &diskClassDeviceInterfaceGuid,
        deviceIndex,
        &deviceInterfaceData); )
    {

        ++deviceIndex;

        SetupDiGetDeviceInterfaceDetail(diskClassDevices,
            &deviceInterfaceData,
            NULL,
            0,
            &requiredSize,
            NULL);
        CHK(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
            "SetupDiGetDeviceInterfaceDetail - 1");

        deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
        CHK(NULL != deviceInterfaceDetailData,
            "malloc");
        
        ZeroMemory(deviceInterfaceDetailData, requiredSize);
        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        CHK(SetupDiGetDeviceInterfaceDetail(diskClassDevices,
            &deviceInterfaceData,
            deviceInterfaceDetailData,
            requiredSize,
            &requiredSize,
            &deviceInfoData),
            "SetupDiGetDeviceInterfaceDetail - 2");

        hDevice = CreateFile(deviceInterfaceDetailData->DevicePath,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        CHK(INVALID_HANDLE_VALUE != hDevice,
            "CreateFile");

        CHK(DeviceIoControl(hDevice,
            IOCTL_STORAGE_GET_DEVICE_NUMBER,
            NULL,
            0,
            &diskNumber,
            sizeof(STORAGE_DEVICE_NUMBER),
            &bytesReturned,
            NULL),
            "IOCTL_STORAGE_GET_DEVICE_NUMBER");

        // If we are here it means everything was ok with this drive => get info
        rawDevEntry = calloc(1, sizeof(SMI_RAWDEV_ENTRY));
        InitializeListHead(&rawDevEntry->DevBlkObjList);
        
        rawDevEntry->DriveInfo.u32DeviceNumber = diskNumber.DeviceNumber;
        _tcsncpy(rawDevEntry->DriveInfo.szDevicePath, deviceInterfaceDetailData->DevicePath, MAX_PATH);
        _tcsncpy(rawDevEntry->DriveInfo.szShortDevicePath, deviceInterfaceDetailData->DevicePath, MAX_PATH);
        if (diskNumber.DeviceType == FILE_DEVICE_DISK) {
            snprintf(rawDevEntry->DriveInfo.szShortDevicePath, MAX_PATH, "\\\\.\\PhysicalDrive%lu", diskNumber.DeviceNumber);
        }
        rawDevEntry->DriveInfo.canBePartitioned = (diskNumber.PartitionNumber == 0);

#if 0
#define ATA_BUF_LEN (sizeof(ATA_PASS_THROUGH_EX) + sizeof(IDENTIFY_DEVICE_DATA))
        UCHAR vBuffer[ATA_BUF_LEN];
        PATA_PASS_THROUGH_EX pATARequest = (PATA_PASS_THROUGH_EX)(&vBuffer[0]);
        IDEREGS * ir = (IDEREGS*)pATARequest->CurrentTaskFile;
        pATARequest->AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_DRDY_REQUIRED;
        pATARequest->Length = sizeof(ATA_PASS_THROUGH_EX);
        pATARequest->DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);
        pATARequest->DataTransferLength = sizeof(IDENTIFY_DEVICE_DATA);
        pATARequest->TimeOutValue = 10;
        ir->bCommandReg = ID_CMD;

        ULONG ulBytesRead;
        if (DeviceIoControl(hDevice, IOCTL_ATA_PASS_THROUGH,
            &vBuffer[0], (ULONG)ATA_BUF_LEN,
            &vBuffer[0], (ULONG)ATA_BUF_LEN,
            &ulBytesRead, NULL) == FALSE)
        {
            printf("DeviceIoControl(IOCTL_ATA_PASS_THROUGH) failed.  LastError: %d\n", GetLastError());
            //iRet = -1;
        }
        else
        {
            // Fetch identity blob from output buffer.
            PIDENTIFY_DEVICE_DATA pIdentityBlob = (PIDENTIFY_DEVICE_DATA)(&vBuffer[sizeof(ATA_PASS_THROUGH_EX)]);
            if (pIdentityBlob)
            {
                printf("dqsdqds");
            }
        }
#undef ATA_BUF_LEN
#endif 
        /*
        WinAPI::IOCTL::StoragePropQuery propSeekPenalty(StorageDeviceSeekPenaltyProperty, PropertyStandardQuery);
        std::vector<char> propSeekPenaltyOut(8192);
        CHK((WinAPI::IOCTL::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
        &propSeekPenalty, sizeof(propSeekPenalty),
        propSeekPenaltyOut, &bytesReturned, NULL)),
        "IOCTL_STORAGE_QUERY_PROPERTY");
        PDEVICE_SEEK_PENALTY_DESCRIPTOR pSeekPenalty = (PDEVICE_SEEK_PENALTY_DESCRIPTOR)&propSeekPenalty[0];
        */


        //http://stackoverflow.com/questions/5070987/sending-ata-commands-directly-to-device-in-windows
        //http://fossies.org/linux/smartmontools/os_win32.cpp
        //http://stackoverflow.com/questions/12942606/ata-command-device-identify
        //https://github.com/DarthTon/SecureEraseWin/blob/master/src/PhysicalDisk.cpp
        //https://www.smartmontools.org/static/doxygen/os__win32_8cpp_source.html




        /*PSTORAGE_DEVICE_DESCRIPTOR pDevDesc = WinAPI::IOCTL::DeviceIoControl<PSTORAGE_DEVICE_DESCRIPTOR>(
        hDevice, &propQuery, &bytesReturned, NULL);*/

        /*std::array<char, 8192> propQueryOut;
        CHK((DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
        &propQuery, sizeof(propQuery),
        &propQueryOut[0], 1024, &bytesReturned, NULL)),
        "IOCTL_STORAGE_QUERY_PROPERTY");*/

        //How does disk defragmenter get the “no seek penalty” from device?
        //IOCTL_STORAGE_QUERY_PROPERTY
        //PropertyId = StorageDeviceSeekPenaltyProperty
        //DEVICE_SEEK_PENALTY_DESCRIPTOR
        //If the IncursSeekPenalty member is set to false, the disk is considered a SSD.

        //How does disk defragmenter get the “nominal media rotation rate” from device?
        //If the port driver does not return valid data for StorageDeviceSeekPenaltyProperty, 
        //Disk Defragmenter looks at the result of directly querying the device through the ATA IDENTIFY DEVICE command.
        //Defragmenter issues IOCTL_ATA_PASS_THROUGH request and checks IDENTIFY_DEVICE_DATA structure.
        //If the NomimalMediaRotationRate is set to 1, this disk is considered a SSD.
        //The latest SSDs will respond to the command by setting word 217 (which is used for reporting the nominal media rotation rate to 1).
        //The word 217 was introduced in 2007 in the ATA8 - ACS specification.

        //How does disk defragmenter get the “random read disc score” from the device ?
        //Disk Defragmenter uses the Windows System Assessment Tool(WinSAT) to evaluate the performance of the device.
        //The performance threshold was determined by some I / O heuristics through WinSAT to best distinguish SSD from rotating hard disks.

        STORAGE_PROPERTY_QUERY storagePropertyQuery;
        ZeroMemory(&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY));
        storagePropertyQuery.PropertyId = StorageDeviceProperty;
        storagePropertyQuery.QueryType = PropertyStandardQuery;

        char propQueryOut[8192] = { 0 };
		CHK((DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
			&storagePropertyQuery, sizeof(storagePropertyQuery),
			&propQueryOut, sizeof(propQueryOut), &bytesReturned, NULL)),
			"IOCTL_STORAGE_QUERY_PROPERTY");
		PSTORAGE_DEVICE_DESCRIPTOR pDevDesc = (STORAGE_DEVICE_DESCRIPTOR *)&propQueryOut[0];
		if (pDevDesc)
		{
			flipAndCodeBytes(pDevDesc->VendorIdOffset,
				0, (const char*)&propQueryOut[0], rawDevEntry->DriveInfo.szVendorId);
			flipAndCodeBytes(pDevDesc->ProductIdOffset,
				0, (const char*)&propQueryOut[0], rawDevEntry->DriveInfo.szModelNumber);
			flipAndCodeBytes(pDevDesc->ProductRevisionOffset,
				0, (const char*)&propQueryOut[0], rawDevEntry->DriveInfo.szProductRevision);
			flipAndCodeBytes(pDevDesc->SerialNumberOffset,
				0, (const char*)&propQueryOut[0], rawDevEntry->DriveInfo.szSerialNumber);
		}

		char propQueryOut2[sizeof(DISK_GEOMETRY_EX)];
		CHK((DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
			NULL, 0,
			&propQueryOut2, sizeof(DISK_GEOMETRY_EX), &bytesReturned, NULL)),
			"IOCTL_STORAGE_QUERY_PROPERTY");
        PDISK_GEOMETRY_EX geom = (PDISK_GEOMETRY_EX)&propQueryOut2[0];
        memcpy(&(rawDevEntry->DriveInfo.diskGeometry), geom, sizeof(DISK_GEOMETRY_EX));
        BytesPerSector = rawDevEntry->DriveInfo.diskGeometry.Geometry.BytesPerSector;

        char propQueryOut3[sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (128 - 1) * sizeof(PARTITION_INFORMATION_EX)];
		CHK(DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
			NULL, 0, &propQueryOut3, sizeof(propQueryOut3), &bytesReturned, NULL),
			"IOCTL_DISK_GET_DRIVE_LAYOUT_EX");

		PDRIVE_LAYOUT_INFORMATION_EX pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX*)&propQueryOut3[0];
        rawDevEntry->DriveInfo.LayoutInfo.PartitionStyle = pDriveLayout->PartitionStyle;
        rawDevEntry->DriveInfo.LayoutInfo.PartitionCount = pDriveLayout->PartitionCount;
        DWORD dwTestMbr, dwTestGpt;
        dwTestMbr = sizeof(DRIVE_LAYOUT_INFORMATION_MBR);
        dwTestGpt = sizeof(DRIVE_LAYOUT_INFORMATION_GPT);
		if (pDriveLayout->PartitionStyle == PARTITION_STYLE_MBR)
		{
			//maxPartitionCount = 4;
			memcpy(&(rawDevEntry->DriveInfo.LayoutInfo.Mbr), &(pDriveLayout->Mbr), sizeof(DRIVE_LAYOUT_INFORMATION_MBR));
		}
		else if (pDriveLayout->PartitionStyle == PARTITION_STYLE_GPT)
		{
			//maxPartitionCount = 128;
			memcpy(&(rawDevEntry->DriveInfo.LayoutInfo.Gpt), &(pDriveLayout->Gpt), sizeof(DRIVE_LAYOUT_INFORMATION_GPT));
		}

        // Partition starts at 1 (we keep 0 to put information about the whole disk (raw device))
        // Information is duplicated with DriveInfo but we don't care
        PSMI_DEVBLK_ENTRY pDevBlk = &(rawDevEntry->Partitions[0]);
        pDevBlk->RawDevice = rawDevEntry;
        pDevBlk->DeviceNumber = rawDevEntry->DriveInfo.u32DeviceNumber;
        pDevBlk->PartitionNumber = 0;
        pDevBlk->PartitionStyle = PARTITION_STYLE_RAW;
        pDevBlk->PartitionLength = rawDevEntry->DriveInfo.diskGeometry.DiskSize;
        pDevBlk->StartingOffset.QuadPart = 0;
        pDevBlk->EndingOffset.QuadPart = rawDevEntry->DriveInfo.diskGeometry.DiskSize.QuadPart;
        SetPosixInfo(pDevBlk, &minor);
        
		for (size_t iPart = 0; iPart < min(pDriveLayout->PartitionCount, 128); iPart++)
		{
			PARTITION_INFORMATION_EX winPartInfo = pDriveLayout->PartitionEntry[iPart];
			if (winPartInfo.PartitionLength.QuadPart > 0)
			{
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // On my USB drive partitioned with exotic partitions (ext2 ext4, HFS+,exFAT) PartitionNumber is wrong
                // TODO: Find if it's because of the USB Drive or if it comes from the partitions
                // Update: it comes from the fact it's an USB
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                DWORD PartitionNumber = winPartInfo.PartitionNumber;
                if (rawDevEntry->DriveInfo.diskGeometry.Geometry.MediaType == RemovableMedia)
                {
                    PartitionNumber = (DWORD)(iPart + 1);
                }
                
                pDevBlk = &(rawDevEntry->Partitions[PartitionNumber]);
                
                pDevBlk->RawDevice = rawDevEntry;
                pDevBlk->DeviceNumber = rawDevEntry->DriveInfo.u32DeviceNumber;
                pDevBlk->PartitionNumber = PartitionNumber;
                pDevBlk->RewritePartition = winPartInfo.RewritePartition;
                pDevBlk->PartitionLength = winPartInfo.PartitionLength;
                pDevBlk->StartingOffset = winPartInfo.StartingOffset;
                pDevBlk->EndingOffset.QuadPart = winPartInfo.StartingOffset.QuadPart + winPartInfo.PartitionLength.QuadPart;
                SetPosixInfo(pDevBlk, &minor);

                pDevBlk->PartitionStyle = winPartInfo.PartitionStyle;
                if (winPartInfo.PartitionStyle == PARTITION_STYLE_MBR)
                {
                    memcpy(&(pDevBlk->Mbr), &(winPartInfo.Mbr), sizeof(PARTITION_INFORMATION_MBR));
                }
                else if (pDriveLayout->PartitionStyle == PARTITION_STYLE_GPT)
                {
                    memcpy(&(pDevBlk->Gpt), &(winPartInfo.Gpt), sizeof(PARTITION_INFORMATION_GPT));
                }
                

                //PPARTITION_INFORMATION_EX pPartInfo = &(rawDevEntry->Partitions[PartitionNumber].PartitionInfo);
				//memcpy(pPartInfo, &winPartInfo, sizeof(PARTITION_INFORMATION_EX));
				if (FindVolume(rawDevEntry->DriveInfo.u32DeviceNumber, &(rawDevEntry->Partitions[PartitionNumber])))
				{

				}
			}
		}


		/*requiredSize = 0;
		STORAGE_PROPERTY_QUERY query;
		ZeroMemory(&query, sizeof(STORAGE_PROPERTY_QUERY));
		query.PropertyId = StorageDeviceProperty;
		query.QueryType = PropertyStandardQuery;
		st::vector<BYTE> propquery; propquery.resize(sizeof(STORAGE_PROPERTY_QUERY));
		STORAGE_PROPERTY_QUERY *pq = (STORAGE_PROPERTY_QUERY*)vectorptr(propquery);*/


		//OLECHAR szGUID[64] = { 0 };
		//StringFromGUID2(DeviceInterfaceData.InterfaceClassGuid, szGUID, 64);

        
        
        

		CloseHandle(hDevice); 
        hDevice = INVALID_HANDLE_VALUE;
        
        
        // if local variable is not NULL it will be deallocated below
        if (rawDevEntry->DriveInfo.u32DeviceNumber < 127)
        {
            g_devblk_array[rawDevEntry->DriveInfo.u32DeviceNumber] = rawDevEntry;
            rawDevEntry = NULL;
        }
        else
        {
            free(rawDevEntry);
            rawDevEntry = NULL;
        }

	}// while
	CHK(ERROR_NO_MORE_ITEMS == GetLastError(),
		"SetupDiEnumDeviceInterfaces");

	END_ERROR_CHK();

    if (rawDevEntry) { free(rawDevEntry); }
	if (INVALID_HANDLE_VALUE != diskClassDevices) {
		SetupDiDestroyDeviceInfoList(diskClassDevices);
	}

	if (INVALID_HANDLE_VALUE != hDevice) {
		CloseHandle(hDevice);
	}
}



BOOL FindVolume(int diskno, PSMI_DEVBLK_ENTRY pDevBlkEntry)
{
	HANDLE vol;
	BOOL success;
    TCHAR szNextVolName[MAX_PATH+1];
    TCHAR szNextVolNameNoBSlash[MAX_PATH+1];

	vol = FindFirstVolume(szNextVolName, MAX_PATH);
	success = (vol != INVALID_HANDLE_VALUE);
	while (success)
	{
		//We are now enumerating volumes. In order for this function to work, we need to get partitions that compose this volume
		HANDLE volH;
		PVOLUME_DISK_EXTENTS vde;
		DWORD bret;

        //For this CreateFile, volume must be without trailing backslash
        StringCchCopy(szNextVolNameNoBSlash, MAX_PATH, szNextVolName);
        szNextVolNameNoBSlash[_tcslen(szNextVolNameNoBSlash) - 1] = _T('\0'); 

		volH = CreateFile(szNextVolNameNoBSlash,
			FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, 0);

		if (volH != INVALID_HANDLE_VALUE)
		{
			bret = sizeof(VOLUME_DISK_EXTENTS) + 256 * sizeof(DISK_EXTENT);
			vde = (PVOLUME_DISK_EXTENTS)malloc(bret);
            if (DeviceIoControl(volH, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, (void *)vde, bret, &bret, NULL))
            {
                for (unsigned i = 0; i < vde->NumberOfDiskExtents; i++)
                {
                    if (vde->Extents[i].DiskNumber == diskno &&
                        vde->Extents[i].StartingOffset.QuadPart == pDevBlkEntry->StartingOffset.QuadPart &&
                        vde->Extents[i].ExtentLength.QuadPart == pDevBlkEntry->PartitionLength.QuadPart)
                    {
                        CHAR szVolumeName[MAX_PATH + 1] = { 0 };
                        CHAR szFileSystemName[MAX_PATH + 1] = { 0 };
                        DWORD dwSerialNumber = 0;
                        DWORD dwMaxFileNameLength = 0;
                        DWORD dwFileSystemFlags = 0;

                        if (GetVolumeInformation(szNextVolName,
                            szVolumeName, _countof(szVolumeName),
                            &dwSerialNumber,
                            &dwMaxFileNameLength,
                            &dwFileSystemFlags,
                            szFileSystemName,
                            _countof(szFileSystemName)))
                        {
                            _tcsncpy(pDevBlkEntry->szRootPathName, szNextVolName, MAX_PATH);
                            _tcsncpy(pDevBlkEntry->szVolumeName, szVolumeName, MAX_PATH);
                            _tcsncpy(pDevBlkEntry->szFileSystemName, szFileSystemName, MAX_PATH);
                            pDevBlkEntry->dwSerialNumber = dwSerialNumber;
                            pDevBlkEntry->dwFileSystemFlags = dwFileSystemFlags;
                        }
                        else
                        {
                            DWORD dwErr = GetLastError();
                            printf("%lu", dwErr);
                        }

                        DWORD length = 0;
                        TCHAR pathnames[MAX_PATH + 1] = { 0 };
                        if (GetVolumePathNamesForVolumeName(szNextVolName, (LPTSTR)pathnames, MAX_PATH, &length))
                        {
                            _tcsncpy(pDevBlkEntry->szVolumePathName, pathnames, MAX_PATH);
                        }

                        free(vde);
                        CloseHandle(volH);
                        FindVolumeClose(vol);
                        return TRUE;
                    } 
                }//for
			}
			free(vde);
			CloseHandle(volH);
		}

		success = FindNextVolume(vol, szNextVolName, MAX_PATH) != 0;
	}
	FindVolumeClose(vol);
	return FALSE;
}

char * flipAndCodeBytes(int iPos, int iFlip, const char * pcszStr, char * pcszBuf)
{
    int iI;
    int iJ = 0;
    int iK = 0;

    pcszBuf[0] = '\0';
    if (iPos <= 0)
        return pcszBuf;

    if (!iJ)
    {
        char cP = 0;
        // First try to gather all characters representing hex digits only.
        iJ = 1;
        iK = 0;
        pcszBuf[iK] = 0;
        for (iI = iPos; iJ && !(pcszStr[iI] == '\0'); ++iI)
        {
            char cC = tolower(pcszStr[iI]);
            if (isspace(cC))
                cC = '0';
            ++cP;
            pcszBuf[iK] <<= 4;

            if (cC >= '0' && cC <= '9')
                pcszBuf[iK] |= (char)(cC - '0');
            else if (cC >= 'a' && cC <= 'f')
                pcszBuf[iK] |= (char)(cC - 'a' + 10);
            else
            {
                iJ = 0;
                break;
            }

            if (cP == 2)
            {
                //if ((pcszBuf[iK] != '\0') && !isprint(pcszBuf[iK]))
                if (pcszBuf[iK] < -1 || pcszBuf[iK] > 255)
                {
                    iJ = 0;
                    break;
                }
                ++iK;
                cP = 0;
                pcszBuf[iK] = 0;
            }

        }
    }

    if (!iJ)
    {
        // There are non-digit characters, gather them as is.
        iJ = 1;
        iK = 0;
        for (iI = iPos; iJ && (pcszStr[iI] != '\0'); ++iI)
        {
            char cC = pcszStr[iI];

            if (!isprint(cC))
            {
                iJ = 0;
                break;
            }

            pcszBuf[iK++] = cC;
        }
    }

    if (!iJ)
    {
        // The characters are not there or are not printable.
        iK = 0;
    }

    pcszBuf[iK] = '\0';

    if (iFlip)
        // Flip adjacent characters
        for (iJ = 0; iJ < iK; iJ += 2)
        {
            char t = pcszBuf[iJ];
            pcszBuf[iJ] = pcszBuf[iJ + 1];
            pcszBuf[iJ + 1] = t;
        }

    // Trim any beginning and end space
    iI = iJ = -1;
    for (iK = 0; (pcszBuf[iK] != '\0'); ++iK)
    {
        if (!isspace(pcszBuf[iK]))
        {
            if (iI < 0)
                iI = iK;
            iJ = iK;
        }
    }

    if ((iI >= 0) && (iJ >= 0))
    {
        for (iK = iI; (iK <= iJ) && (pcszBuf[iK] != '\0'); ++iK)
            pcszBuf[iK - iI] = pcszBuf[iK];
        pcszBuf[iK - iI] = '\0';
    }

    return pcszBuf;
}
