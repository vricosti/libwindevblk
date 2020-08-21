# libwindevblk
Windows Library to easily read/write partitions or disk drives

libwindevblk provides a simplified Windows API to detect and access disk(HDD, SSD, ...) or removable media (USB, SDCard, ...).
The API is close to the traditional one used to manipulate files (CreateFile, Read, Write, Close,...):

```
* HDEVBLK DevBlkOpen(UINT iDevBlkNumber, UINT iPartitionNumber, ...);

* BOOL DevBlkRead(HDEVBLK hDevBlk, ...);

* BOOL DevBlkSetPointerEx(HDEVBLK hDevBlk, ...);

* BOOL DevBlkWrite(HDEVBLK hDevBlk, ...);

* BOOL Close(HDEVBLK hDevBlk);
```

The argument iDevBlkNumber corresponds to the index of the device.
The argument iPartitionNumber corresponds to the partition index. The index 0 corresponds to the whole device.

The library has some functions used to enumerate devices and their partitions:

```
* BOOL DevBlkEnumDriveInfo(...)

* BOOL DevBlkEnumDevBlkInfo(...)
```


There is also a work in progress to propose a posix api that allows to open a device using its linux name (ex: /dev/sda):

```
int __cdecl devblk_open(const char* pathname, int flags, mode_t mode);
int __cdecl devblk_fstat(int fd, struct _stat *buf);
int __cdecl devblk_close(int fd);
//TODO
```

This library has been tested on Windows 10 with CMake and VS2019.

Here is an example of information provided by the library:

```
Drive 0:
   SerialNumber: 2009E28F2B15
   ModelNumber: CT2000MX500SSD1
   VendorId:
   ProductRevision: M3CR032
   DevicePath: \\?\scsi#disk&ven_&prod_ct2000mx500ssd1#4&2c9020cd&0&010000#{53f56307-b6bf-11d0-94f2-00a0c91efb8b}
   ShortDevicePath: \\.\PhysicalDrive0
   canBePartitioned: 1
   PartitionStyle: 1
   PartitionCount: 2
   Gpt.DiskId: {62918EA9-2F28-43A2-8EDC-BC10C35F5387}
   Gpt.StartingUsableOffset: 17408
   Gpt.UsableLength: 2000398899712
   Gpt.MaxPartitionCount: 128
   *Partition 0 (PosixPath = /dev/sda):
      PartitionLength: 2000398934016
      StartingOffset: 0
      EndingOffset: 2000398934016
      szFileSystemName:
      szRootPathName:
      szVolumeName:
      szVolumePathName:
   *Partition 1 (PosixPath = /dev/sda1):
      PartitionLength: 16759808
      StartingOffset: 17408
      EndingOffset: 16777216
      Gpt.PartitionType: {E3C9E316-0B5C-4DB8-817D-F92DF00215AE}
      Gpt.PartitionId: {25E93B6F-88EE-4304-9C17-EEDD5E0CEFB3}
      szFileSystemName:
      szRootPathName:
      szVolumeName:
      szVolumePathName:
   *Partition 2 (PosixPath = /dev/sda2):
      PartitionLength: 1685808218112
      StartingOffset: 16777216
      EndingOffset: 1685824995328
      Gpt.PartitionType: {EBD0A0A2-B9E5-4433-87C0-68B6B72699C7}
      Gpt.PartitionId: {A435C8BB-B860-430A-958A-B6E887AEC36B}
      szFileSystemName: NTFS
      szRootPathName: \\?\Volume{a435c8bb-b860-430a-958a-b6e887aec36b}\
      szVolumeName: Data
      szVolumePathName: D:\
```