/* config.h */
/* $Id: config.h,v 1.12 2000-11-06 18:38:57 sdennis Exp $ */

#ifndef CONFIG_H
#define CONFIG_H

/* Compile time options */

#define CONF_FILE "netmux.conf" /* Default config file */

#define SIDE_EFFECT_FUNCTIONS   /* Those neat funcs that should be commands */
                 
#define PLAYER_NAME_LIMIT   22  /* Max length for player names */
#define NUM_ENV_VARS        10  /* Number of env vars (%0 et al) */
#define MAX_ARG             100 /* max # args from command processor */
#define MAX_GLOBAL_REGS     10  /* r() registers */

#define OUTPUT_BLOCK_SIZE   16384
#define StringCopy          strcpy
#define StringCopyTrunc     strncpy

/* ---------------------------------------------------------------------------
 * Database R/W flags.
 */

#define MANDFLAGS       (V_LINK|V_PARENT|V_XFLAGS|V_ZONE|V_POWERS|V_3FLAGS|V_QUOTED)

#define OFLAGS1     (V_GDBM|V_ATRKEY)   /* GDBM has these */

#define OFLAGS2     (V_ATRNAME|V_ATRMONEY)

#define OUTPUT_VERSION  1           /* Version 1 */
#ifdef MEMORY_BASED
#define OUTPUT_FLAGS    (MANDFLAGS)
#else
#define OUTPUT_FLAGS    (MANDFLAGS|OFLAGS1|OFLAGS2)
                        /* format for dumps */
#endif /* MEMORY_BASED */

#define UNLOAD_VERSION  1           /* verison for export */
#define UNLOAD_OUTFLAGS (MANDFLAGS)     /* format for export */

/* magic lock cookies */
#define NOT_TOKEN   '!'
#define AND_TOKEN   '&'
#define OR_TOKEN    '|'
#define LOOKUP_TOKEN    '*'
#define NUMBER_TOKEN    '#'
#define INDIR_TOKEN '@'     /* One of these two should go. */
#define CARRY_TOKEN '+'     /* One of these two should go. */
#define IS_TOKEN    '='
#define OWNER_TOKEN '$'

/* matching attribute tokens */
#define AMATCH_CMD  '$'
#define AMATCH_LISTEN   '^'

/* delimiters for various things */
#define EXIT_DELIMITER  ';'
#define ARG_DELIMITER   '='
#define ARG_LIST_DELIM  ','

/* These chars get replaced by the current item from a list in commands and
 * functions that do iterative replacement, such as @apply_marked, dolist,
 * the eval= operator for @search, and iter().
 */

#define BOUND_VAR   "##"
#define LISTPLACE_VAR   "#@"

/* This token is used to denote a null output delimiter. */

#define NULL_DELIM_VAR  "@@"

/* This is used to indent output from pretty-printing. */

#define INDENT_STR  "  "

/* amount of object endowment, based on cost */
#define OBJECT_ENDOWMENT(cost) (((cost)/mudconf.sacfactor) +mudconf.sacadjust)

/* !!! added for recycling, return value of object */
#define OBJECT_DEPOSIT(pennies) \
    (((pennies)-mudconf.sacadjust)*mudconf.sacfactor)

#ifdef WIN32
#define DCL_CDECL __cdecl
#define DCL_INLINE __inline

typedef __int64 INT64;
typedef unsigned __int64 UINT64;
#define INT64_MAX_VALUE  9223372036854775807i64
#define INT64_MIN_VALUE  (-9223372036854775807i64 - 1)
#define UINT64_MAX_VALUE 0xffffffffffffffffui64

#define SIZEOF_PATHNAME (_MAX_PATH + 1)
#define SOCKET_WRITE(s,b,n,f) send(s,b,n,f)
#define SOCKET_READ(s,b,n,f) recv(s,b,n,f)
#define SOCKET_CLOSE(s) closesocket(s)
#define IS_SOCKET_ERROR(cc) ((cc) == SOCKET_ERROR)
#define IS_INVALID_SOCKET(s) ((s) == INVALID_SOCKET)
#define popen _popen
#define pclose _pclose

#else // WIN32

#define DCL_CDECL
#define DCL_INLINE inline
#define INVALID_HANDLE_VALUE (-1)
#define O_BINARY 0
typedef int BOOL;
#define TRUE    1
#define FALSE   0
typedef int HANDLE;

typedef long long INT64;
typedef unsigned long long UINT64;
#define INT64_MAX_VALUE  9223372036854775807LL
#define INT64_MIN_VALUE  (-9223372036854775807LL - 1)
#define UINT64_MAX_VALUE 0xffffffffffffffffULL

typedef int SOCKET;
#ifdef PATH_MAX
#define SIZEOF_PATHNAME (PATH_MAX + 1)
#else
#define SIZEOF_PATHNAME (4095 + 1)
#endif
#define SOCKET_WRITE(s,b,n,f) write(s,b,n)
#define SOCKET_READ(s,b,n,f) read(s,b,n)
#define SOCKET_CLOSE(s) close(s)
#define IS_SOCKET_ERROR(cc) ((cc) < 0)
#define IS_INVALID_SOCKET(s) ((s) < 0)
#define INVALID_SOCKET (-1)
#define SD_BOTH (2)

#endif // WIN32

extern BOOL AssertionFailed(const char *SourceFile, unsigned int LineNo);
#define Tiny_Assert(exp) (void)( (exp) || (AssertionFailed(__FILE__, __LINE__), 0) )

extern BOOL OutOfMemory(const char *SourceFile, unsigned int LineNo);
#define ISOUTOFMEMORY(exp) (!(exp) && OutOfMemory(__FILE__, __LINE__))

#define MEMALLOC(size)        malloc((size))
#define MEMFREE(ptr)          free((ptr))
#define MEMREALLOC(ptr, size) realloc((ptr), (size))

// If it's Hewlett Packard, then getrusage is provided a different
// way.
//
#ifdef hpux
#define HAVE_GETRUSAGE 1
#include <sys/syscall.h>
#define getrusage(x,p)   syscall(SYS_GETRUSAGE,x,p)
#endif

#endif // CONFIG_H
