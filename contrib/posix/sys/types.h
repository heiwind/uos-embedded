#ifndef __SYS_TYPES_H__
#define __SYS_TYPES_H__

#include <runtime/lib.h>
#include <posix-port.h>

typedef int sigset_t;
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
typedef int mode_t;
typedef int dev_t;
typedef int ino_t;
typedef int nlink_t;
typedef long off_t;
typedef long blksize_t;
typedef long blkcnt_t;

#ifndef unixtime_t
typedef unsigned long unixtime_t;
#define unixtime_t unixtime_t
#endif
#define time_t unixtime_t
#define __time_t_defined

#endif
