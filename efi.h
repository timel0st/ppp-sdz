#ifndef EFI_H
#define EFI_H
#include "uefi/uefi.h"
#include "utils.h"
#include "user.h"
#include "log.h"
#include "cfg.h"
#include "gui.h"

#define HEADER_STRING "Pretty Poor Privacy SDZ v.0.5.4"

#define cout ST->ConOut
#define cin ST->ConIn

#define IN
#define OUT
#define OPTIONAL
#define CONST const

#define FALSE 0
#define TRUE 1
typedef uint8_t  BOOLEAN;  // 0 = False, 1 = True
typedef int64_t  INTN;
typedef uint64_t UINTN;
typedef int8_t   INT8;
typedef uint8_t  UINT8;
typedef int16_t  INT16;
typedef uint16_t UINT16;
typedef int32_t  INT32;
typedef uint32_t UINT32;
typedef int64_t  INT64;
typedef uint64_t UINT64;
typedef char     CHAR8;

typedef void VOID;

typedef struct {
    UINT32 TimeLow;
    UINT16 TimeMid;
    UINT16 TimeHighAndVersion;
    UINT8  ClockSeqHighAndReserved;
    UINT8  ClockSeqLow;
    UINT8  Node[6];
} __attribute__ ((packed)) EFI_GUID;

typedef UINTN EFI_STATUS;
typedef VOID *EFI_HANDLE;
typedef VOID *EFI_EVENT;
typedef UINT64 EFI_LBA;
typedef UINTN EFI_TPL;

typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;


#define cout            ST->ConOut
#define cin             ST->ConIn


#endif