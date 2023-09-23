/*In the name of God*/
/**
 * @file FileManager.h
 * @author Reza Dehghan
 * @brief
 * @version 0.1
 * @date 2023-01-23
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef _FILE_MANAGER_H_
#define _FILE_MANAGER_H_

#ifdef _cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "Queue.h"
#include "StreamBuffer.h"
#include "DateTime.h"

#define   FILE_MANAGER_TIMEOUT            1000
//#define   FILE_CHECK_ENABLE             0
#define   FILE_MANAGER_USE_FOR_LOGGER     1
#define   END_OF_FILE                     -1
/*New*/
#define   MAX_PATH_LENGTH                 50
#define   FILE_MANAGER_PATH_FORMAT       "%u-%s-%02u%02u%02u-%02u%02u.txt"
/*End*/

#define   MAXIMUM_ERASE_BUFFER_SIZE        64           ///// if u use big Number it possible Stack OverFlow

typedef   void      FileManager_Fil;
typedef   uint32_t  FileManager_Timestamp;


typedef enum {
    FileManager_OK = 0,              /* (0) Succeeded */
    FileManager_DISK_ERR,            /* (1) A hard error occurred in the low level disk I/O layer */
    FileManager_INT_ERR,             /* (2) Assertion failed */
    FileManager_NOT_READY,           /* (3) The physical drive cannot work */
    FileManager_NO_FILE,             /* (4) Could not find the file */
    FileManager_NO_PATH,             /* (5) Could not find the path */
    FileManager_INVALID_NAME,        /* (6) The path name format is invalid */
    FileManager_DENIED,              /* (7) Access denied due to prohibited access or directory full */
    FileManager_EXIST,               /* (8) Access denied due to prohibited access */
    FileManager_INVALID_OBJECT,      /* (9) The file/directory object is invalid */
    FileManager_WRITE_PROTECTED,     /* (10) The physical drive is write protected */
    FileManager_INVALID_DRIVE,       /* (11) The logical drive number is invalid */
    FileManager_NOT_ENABLED,         /* (12) The volume has no work area */
    FileManager_NO_FILESYSTEM,       /* (13) There is no valid FAT volume */
    FileManager_MKFS_ABORTED,        /* (14) The f_mkfs() aborted due to any problem */
    FileManager_TIMEOUT,             /* (15) Could not get a grant to access the volume within defined period */
    FileManager_LOCKED,              /* (16) The operation is rejected according to the file sharing policy */
    FileManager_NOT_ENOUGH_CORE,     /* (17) LFN working buffer could not be allocated */
    FileManager_TOO_MANY_OPEN_FILES, /* (18) Number of open files > _FS_LOCK */
    FileManager_INVALID_PARAMETER    /* (19) Given parameter is invalid */
} FileManager_Result;


typedef enum {
    FileManager_FileIsClose      = 0x00,  
    FileManager_FileIsOpen       = 0x01,
} FileManager_FileStatus;


typedef enum {
    FileManager_Const            = 0,
    FileManager_Var              = 1,
} FileManager_Type;              
                                 
                                 
typedef enum {                   
    FileManager_WriteMode        = 0x00,
    FileManager_ReadMode         = 0x01,
    FileManager_WriteHeaderMode  = 0x02,
    FileManager_LoggerReadMode   = 0x03,
} FileManager_Mode;              
                                 
                                 
                                 
typedef enum {                   
    FileManager_OpenExisting     = 0x00,
    FileManager_Read             = 0x01,
    FileManager_Write            = 0x02,
    FileManager_CreateNew        = 0x04,
    FileManager_CreateAlways     = 0x08,
    FileManager_OpenAlways       = 0x10,
    FileManager_OpenAppend       = 0x30,
} FileManager_OpenMethod;



typedef struct {
    uint16_t  MaxSS;
} FileManager_Config;



/**
 * @brief 
 */
typedef struct {    
    DateTime_X             DT;
    int32_t                Addr;
    int32_t                Len;
    uint8_t                DataType;
    uint8_t                Mode;
} FileManager_CommandHeader;



typedef struct {
    char*                 Indicator;
    int16_t               DeviceId;
} FileManager_RecFrame;



/**
 * @brief 
 */
typedef enum {
    FileManager_DoNotMountNow  = 0x00,        //Do not mount now (to be mounted on the first access to the volume) (in USB we usual Mount with This Method)
    FileManager_ForceMount     = 0x01,        //Force mounted the volume to check if it is ready to work (in SdCard we usual Mount with This Method)
} FileManager_MountMethod;


/****PreDefined Struct****/
struct          _FileManager;
typedef struct  _FileManager  FileManager;


typedef void (*FileManager_ReadCallbackFn)       (FileManager* file, Stream* stream, FileManager_CommandHeader* command);
typedef void (*FileManager_noDetectSDCallbackFn)     (void);
typedef void (*FileManager_createFileCallbackFn) (FileManager* file);
//typedef void (*FileManager_changePathCallbackFn) (FileManager* file);
typedef void (*FileManager_getAddressFn)         (FileManager* file);



typedef struct {
    FileManager_ReadCallbackFn        onRead;   //This callbacks occur when the data is read From the SdCard 
    FileManager_noDetectSDCallbackFn  onNotDetect;
    FileManager_createFileCallbackFn  onCreateFile;
    FileManager_getAddressFn          onGetAddress;
} FileManager_Callbacks;


struct _FileManager {
    struct _FileManager*      Previous;
    FileManager_Fil*          Context;  
    const FileManager_Config* Config;
    uint8_t*                  Path;
    uint8_t*                  ConstVal;
    uint32_t                  PendingByte;
    FileManager_Callbacks     Callbacks;
    FileManager_CommandHeader CommandHeaderInProcess;               
    Queue                     CommandQueue;            
    Queue                     ReadQueue;               
    Stream                    WriteStream;             
    Stream                    ReadStream;
    Stream                    TempStream;
    Stream                    HeaderStream;
    void*                     Args;         /*Logger*/
    /*New*/
    void*                     Args1;        /*Logger Argument*/
    //uint32_t                  FileSize;
    /*End*/
    FileManager_Timestamp     NextTick;
    int16_t                   TempLen;
    uint8_t                   UseForLogger : 1;
    uint8_t                   FirstTimeRun : 1;
    uint8_t                   Overflow     : 1;
    uint8_t                   InProcess    : 1;
    uint8_t                   Enabled      : 1;
    uint8_t                   FileStatus   : 1;
    uint8_t                   Reserved     : 2;
};



#define  FILE_MANAGER_NULL  ((FileManager*)0)


FileManager_Result FileManager_add    (FileManager* file, FileManager_Fil* fil, const FileManager_Config* config,  uint8_t* path);
void               File_init          (FileManager* file, uint8_t* commandQBuffer, uint16_t commandQLen, uint8_t* qReadBuffer, uint16_t qReadLen, uint8_t* streamWriteBuffer, uint16_t streamWriteLen, uint8_t* streamReadBuffer, uint16_t streamReadLen);
FileManager_Result FileManager_handle (void);
FileManager_Result File_writeBlocking (FileManager* file, int32_t addr, uint8_t* data, int32_t len);
FileManager_Result File_readBlocking  (FileManager* file, int32_t addr, uint8_t* data, int32_t len);
FileManager_Result File_write         (FileManager* file, int32_t addr, uint8_t* data, int32_t len, FileManager_Type type);
FileManager_Result File_read          (FileManager* file, int32_t addr, int32_t len);
FileManager_Result File_erase         (FileManager* file); 

void                  FileManager_setArgs                (FileManager* file, void* arg);
void*                 FileManager_getArgs                (FileManager* file);
void                  File_writeHeader                   (FileManager* file, uint8_t* data, uint16_t len);
FileManager*          FileManager_getLastFile            (void);
FileManager_Timestamp FileManager_getTimeStamp           (void);
FileManager_Result    FileManager_setNewPath             (FileManager* file, uint8_t* newPath);


#if FILE_MANAGER_USE_FOR_LOGGER
Stream*            File_beginWrite    (FileManager* file, Stream* tempStream, int32_t len);
void               File_endWrite      (FileManager* file, int32_t addr, Stream* tempStream);
Stream*            File_beginRead     (FileManager* file, int32_t addr, Stream* tempStream, int32_t len);
void               File_endRead       (FileManager* file, Stream* tempStream);         
FileManager_Result File_loggerRead    (FileManager* file, DateTime_X* dateTime, int32_t addr, int32_t len);
#endif



typedef FileManager_Result (*FileManager_openFn)              (FileManager* file, uint8_t* path, FileManager_OpenMethod openMethod);
typedef FileManager_Result (*FileManager_writeFn)             (FileManager* file, void* data, int32_t len);
typedef FileManager_Result (*FileManager_readFn)              (FileManager* file, void* data, int32_t len);
typedef FileManager_Result (*FileManager_mountFn)             (FileManager_MountMethod mountStatus);
typedef FileManager_Result (*FileManager_unMountFn)           (void);
typedef FileManager_Result (*FileManager_lseekFn)             (FileManager* file, int32_t addr);
typedef FileManager_Result (*FileManager_closeFn)             (FileManager* file);
typedef uint8_t            (*FileManager_isOpen)              (FileManager* file);
typedef uint32_t           (*FileManager_sizeFn)              (FileManager* file);
typedef uint8_t            (*FileManager_BSP_SD_IsDetectedFn) (void);
typedef FileManager_Result (*FileManager_unLinkFileFn)        (uint8_t* path);
typedef uint32_t           (*FileManager_getTimestampFn)      (void);

typedef struct {
    FileManager_openFn              Open;              //// open File in sdCard
    FileManager_writeFn             Write;             //// Write Data in sdCard
    FileManager_readFn              Read;              //// Read  Data in sdCard
    FileManager_mountFn             Mount;             //// Mount SdCard 
    FileManager_unMountFn           UnMount;           //// UnMount SdCard
    FileManager_lseekFn             Lseek;             //// Lseek Address in sdCard 
    FileManager_closeFn             Close;             //// Close one File in SdCard
    FileManager_isOpen              IsOpen;            //// Check File is Open or Not
    FileManager_sizeFn              FileSize;          //// get Size of One File in SdCard
    FileManager_BSP_SD_IsDetectedFn IsDetected;        //// check your SdCard is detect or Not
    FileManager_unLinkFileFn        UnLink;            //// UnLink(erase) file 
    FileManager_getTimestampFn      GetTimestamp;      //// get timeStamp of your MCU
} FileManager_Driver;


void FileManager_Init (const FileManager_Driver* driver);

FileManager_Result File_writeUInt8Blocking  (FileManager* file, uint8_t val,  int32_t addr);
uint8_t File_readUInt8Blocking   (FileManager* file, int32_t addr);
FileManager_Result File_writeUInt16Blocking (FileManager* file, uint16_t val, int32_t addr);
uint16_t File_readUInt16Blocking (FileManager* file, int32_t addr);
FileManager_Result File_writeUInt32Blocking (FileManager* file, uint32_t val, int32_t addr);
uint32_t File_readUInt32Blocking (FileManager* file, int32_t addr);
FileManager_Result File_writeUInt64Blocking (FileManager* file, uint64_t val, int32_t addr);
uint64_t File_readUInt64Blocking (FileManager* file, int32_t addr);


void   File_onRead       (FileManager* file, FileManager_ReadCallbackFn        cb);
void   File_onNotDetect  (FileManager* file, FileManager_noDetectSDCallbackFn  cb);
void   File_onCreateFile (FileManager* file, FileManager_createFileCallbackFn  cb);
void   File_onGetAddress (FileManager* file, FileManager_getAddressFn          cb);
int8_t FileManager_assertMemory (uint8_t* arr1, uint8_t* arr2, uint16_t len);



#ifdef __cplusplus
};
#endif

#endif /* _FILE_MANAGER_H_ */

