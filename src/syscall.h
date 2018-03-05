/*
Copyright (C) 2017 The University of Notre Dame
This software is distributed under the GNU General Public License.
See the file LICENSE for details.
*/

#ifndef SYSCALL_H
#define SYSCALL_H

#include "kerneltypes.h"

typedef enum {
    SYSCALL_DEBUG,
    SYSCALL_YIELD,
    SYSCALL_EXIT,
    SYSCALL_OPEN,
    SYSCALL_READ,
    SYSCALL_WRITE,
    SYSCALL_LSEEK,
    SYSCALL_CLOSE,
    SYSCALL_KEYBOARD_READ_CHAR,
    SYSCALL_DRAW_COLOR,
    SYSCALL_DRAW_LINE,
    SYSCALL_DRAW_RECT,
    SYSCALL_DRAW_CLEAR,
    SYSCALL_DRAW_CHAR,
    SYSCALL_DRAW_STRING,
    SYSCALL_DRAW_CREATE,
    SYSCALL_DRAW_WRITE,
    SYSCALL_SLEEP,
    SYSCALL_GETTIMEOFDAY,
    SYSCALL_GETPID,
    SYSCALL_GETPPID,
    SYSCALL_MOUNT,
    SYSCALL_CHDIR,
    SYSCALL_SBRK,
    SYSCALL_PROCESS_RUN,
    SYSCALL_PROCESS_KILL,
    SYSCALL_PROCESS_WAIT,
    SYSCALL_PROCESS_REAP,
    SYSCALL_PROCESS_SELF,
	  SYSCALL_PROCESS_PARENT,
	  SYSCALL_MKDIR,
	  SYSCALL_RMDIR,
	  SYSCALL_READDIR,
	  SYSCALL_PWD,
} syscall_t;

typedef enum {
    ENOENT=-1,
    EINVAL=-2,
    EACCES=-3,
    ENOSYS=-4
} syscall_error_t;

uint32_t syscall( syscall_t s, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e );

#endif
