# libwindevblk
Library to easily read/write windows partitions or disk drives

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

This library has been tested with CMake and VS2019.
