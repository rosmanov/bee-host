/**
* Alternative mkstemps implementation.
*
* Based on https://github.com/ClickHouse/ClickHouse/blob/3203fa4c34ac66990393e846621c89352fd4ac42/base/glibc-compatibility/musl/mkstemps.c
* Original code was wrapped in an #ifndef preprocessor block.
*
* Modified by Ruslan Osmanov in 2025 to remove dependency on clock_gettime
* and support Windows-compatible entropy source (GetSystemTimeAsFileTime).
*
* Copyright 2016-2021 Yandex LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef HAVE_MKSTEMPS

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
# include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <stdint.h>

static uint64_t win_time_entropy()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}
#endif

/* This assumes that a check for the
   template size has already been made */
static char * __randname(char * template)
{
    int i;
    unsigned long r;

#ifdef _WIN32
    r = win_time_entropy();
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    r = (ts.tv_nsec * 65537) ^ ((((intptr_t)(&ts)) / 16) + ((intptr_t)template));
#endif
    for (i = 0; i < 6; i++, r >>= 5)
        template[i] = 'A' + (r & 15) + (r & 16) * 2;

    return template;
}

int mkstemps(char * template, int len)
{
    size_t l = strlen(template);
    if (l < 6 || len > l - 6 || memcmp(template + l - len - 6, "XXXXXX", 6))
    {
        errno = EINVAL;
        return -1;
    }

    int fd, retries = 100;
    do
    {
        __randname(template + l - len - 6);
        if ((fd = open(template, O_RDWR | O_CREAT | O_EXCL, 0600)) >= 0)
            return fd;
    } while (--retries && errno == EEXIST);

    memcpy(template + l - len - 6, "XXXXXX", 6);
    return -1;
}

#endif /*! HAVE_MKSTEMPS */
