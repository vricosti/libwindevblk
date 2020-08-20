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
#ifndef _WINDEVBLK_POSIX_
#define _WINDEVBLK_POSIX_
#pragma once


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

    // Posix subsystem
    int __cdecl devblk_open(const char* pathname, int flags, mode_t mode);
    int __cdecl devblk_fstat(int fd, struct _stat *buf);
    int __cdecl devblk_close(int fd);

#ifdef  __cplusplus
}
#endif






#endif /*_WINDEVBLK_POSIX_*/