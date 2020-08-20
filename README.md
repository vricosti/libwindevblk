# libwindevblk
Library to easily read/write windows partitions or disk drives

libwindevblk provides a simplified Windows API to access disk(HDD, SSD, ...) or removable media (USB, SDCard, ...).
The API is close to the traditional one used to manipulate files (CreateFile, Read, Write, Close,...):

```
* HDEVBLK DevBlkOpen(UINT iDevBlkNumber, UINT iPartitionNumber, ...);

* BOOL DevBlkRead(HDEVBLK hDevBlk, ...);

* BOOL DevBlkSetPointerEx(HDEVBLK hDevBlk, ...);

* BOOL DevBlkWrite(HDEVBLK hDevBlk, ...);

* BOOL Close(HDEVBLK hDevBlk);
```

The argument iDevBlkNumber corresponds to the index of the device, you can use the provided utility to know it or use diskpart.
The argument iPartitionNumber corresponds to the partition index. The index 0 corresponds to the whole device.


The library has some functions used to enumerate devices and their partitions:

```
* BOOL DevBlkEnumDriveInfo(...)

* BOOL DevBlkEnumDevBlkInfo(...)
```


There is also a work in progress to propose a posix api that allows to open a device using its linux name (ex: /dev/sda)
