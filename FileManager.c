
#include "FileManager.h"

/* Private Variable */
FileManager* lastFile     = FILE_MANAGER_NULL;
const FileManager_Driver*   fileManagerDriver;




/**
 * @brief this Function use for Initial the Driver
 * 
 * @param driver Address of FileManagerDriver 
 */
void FileManager_Init(const FileManager_Driver* driver) {
    fileManagerDriver = driver;
}




/**
 * @brief This Function use to add FileManger Stuct into LinkedList
 * 
 * @param file Address Of FileManager Struct
 * @param fil Address Of FIL struct(FatFs Library struct)
 * @param config Address of your FileManager Config
 * @param path the Path of File in SdCard ( Like "FileName.txt")
 * @return FileManager_Result 
 */
FileManager_Result FileManager_add (FileManager* file, FileManager_Fil* fil, const FileManager_Config* config, uint8_t* path) {
    if (FILE_MANAGER_NULL == file) {
        return FileManager_NO_FILE;
    }
    file->Previous                    = lastFile;
    lastFile                          = file;
    file->Path                        = path;
    file->Config                      = config;
    file->Context                     = fil;
    file->CommandHeaderInProcess.Len  = 0;
    file->CommandHeaderInProcess.Addr = 0;
    file->Enabled                     = 1;
    return FileManager_OK;
}




/**
 * @brief FileManager Initial
 * 
 * @param file              Address of FileManager Struct
 * @param commandQBuffer    Address of CommandQBuffer
 * @param commandQLen       Sizeof CommandQBuffer
 * @param qReadBuffer       Address of QReadBuffer
 * @param qReadLen          SizeOf QReadBuffer
 * @param streamWriteBuffer Address of streamWriteBuffer 
 * @param streamWriteLen    sizeof streamWriteBuffer
 * @param streamReadBuffer  Address of streamReadBuffer 
 * @param streamReadLen     sizeof streamReadBuffer
 */
void File_init(FileManager* file, uint8_t* commandQBuffer, uint16_t commandQLen, uint8_t* qReadBuffer, uint16_t qReadLen, uint8_t* streamWriteBuffer, uint16_t streamWriteLen, uint8_t* streamReadBuffer, uint16_t streamReadLen) {
    Queue_init (&file->CommandQueue, commandQBuffer,    commandQLen, sizeof(FileManager_CommandHeader)); 
    Queue_init (&file->ReadQueue,    qReadBuffer,       qReadLen,    sizeof(FileManager_CommandHeader));  
    Stream_init(&file->WriteStream,  streamWriteBuffer, streamWriteLen);
    Stream_init(&file->ReadStream,   streamReadBuffer,  streamReadLen);
    file->CommandHeaderInProcess.Addr      = 0;
    file->CommandHeaderInProcess.Len       = 0;
    file->CommandHeaderInProcess.DataType  = 0;
    file->CommandHeaderInProcess.Mode      = 0;
    memset(&file->CommandHeaderInProcess.DT, 0, sizeof(DateTime_X));
    file->PendingByte                      = 0;
}








/**
 * @brief This Function use for Blocking Write (Write until Complete)
 * 
 * @param file Address of FileManager struct
 * @param addr Address in SdCard File u want to write here
 * @param data Address of Data u want to write in SdCard
 * @param len  Length Of Data u want to Write into SdCard 
 * @return FileManager_Result 
 */
FileManager_Result File_writeBlocking (FileManager* file, int32_t addr, uint8_t* data, int32_t len) {
    uint32_t  time             = 0;
    uint16_t tempLen           = 0;
    uint8_t  overPage          = 0;
    uint16_t size              = 0;
    FileManager_Result         fatFsResult;
    FileManager_CommandHeader  cacheHeader;
    memset (&cacheHeader.DT, 0, sizeof(cacheHeader.DT));
    cacheHeader.Addr           = addr;
    cacheHeader.Len            = len;
    cacheHeader.Mode           = FileManager_WriteMode;
    cacheHeader.DataType       = FileManager_Var;
    if (cacheHeader.Len < 1) {
        return FileManager_INVALID_PARAMETER;
    }
    file->InProcess            = 1;
    if (fileManagerDriver->IsDetected() != 0) {
      fatFsResult = fileManagerDriver->Mount(FileManager_ForceMount);
      if (fileManagerDriver->Open (file, file->Path, FileManager_OpenAlways | FileManager_Write) == FileManager_OK) {
        size = fileManagerDriver->FileSize(file);
        
        if (cacheHeader.Addr != END_OF_FILE) {
            fatFsResult = fileManagerDriver->Lseek(file, cacheHeader.Addr); 
        }
        else {
            fatFsResult = fileManagerDriver->Lseek(file, size); 
        }
        while (cacheHeader.Len > 0  && fatFsResult == FileManager_OK) {
            overPage       =  cacheHeader.Len > file->Config->MaxSS  ? 1                   : 0;
            tempLen        =  overPage                               ? file->Config->MaxSS : cacheHeader.Len;
            time           = fileManagerDriver->GetTimestamp();
            fatFsResult    =  fileManagerDriver->Write(file, data, tempLen);
            while(file->PendingByte < tempLen - 1) {
            }
            if (time + FILE_MANAGER_TIMEOUT < fileManagerDriver->GetTimestamp()) {
                return FileManager_TIMEOUT;
            }
            cacheHeader.Len -= tempLen;
            data            += tempLen;
        }    
     }
     fatFsResult = fileManagerDriver->Close(file);
     file->InProcess = 0;
   }
   else {
        if (file->Callbacks.onNotDetect != NULL) {
            file->Callbacks.onNotDetect();
        }
        fatFsResult = FileManager_DISK_ERR;
   }
    return fatFsResult;
}





/**
 * @brief This Function is use For Blocking Read from SdCard (Read Until Complete)
 * 
 * @param file Address of FileManager Struct
 * @param addr SdCard FileAddress u want to Read From that
 * @param data Address of Data u want to Read in SdCard
 * @param len  Length Of Data u want to Read from SdCard
 * @return FileManager_Result 
 */
FileManager_Result File_readBlocking (FileManager* file, int32_t addr, uint8_t* data, int32_t len) {
    uint16_t                       tempLen     = 0;
    uint8_t                        overFlow    = 0;
    FileManager_Result             fatFSResult = 0; 
    
    FileManager_CommandHeader      cacheHeader;
    memset(&cacheHeader.DT, 0, sizeof(cacheHeader.DT));
    cacheHeader.Addr               = addr;
    cacheHeader.DataType           = FileManager_Var;
    cacheHeader.Mode               = FileManager_ReadMode;
    cacheHeader.Len                = len; 
    uint8_t* pData = data;
    
    if (cacheHeader.Len < 1) {
        return FileManager_INVALID_PARAMETER;
    }
    file->InProcess = 1;
    if (fileManagerDriver->IsDetected() == 1) {
        fileManagerDriver->Mount(FileManager_ForceMount);
        if (fileManagerDriver->Open(file, file->Path, FileManager_OpenAlways | FileManager_Read) == FileManager_OK) {
            fatFSResult = fileManagerDriver->Lseek(file, cacheHeader.Addr);
            while(cacheHeader.Len > 0 && fatFSResult == FileManager_OK) {
                overFlow           = cacheHeader.Len > file->Config->MaxSS ? 1                   : 0;
                tempLen            = overFlow                              ? file->Config->MaxSS : cacheHeader.Len;
                fatFSResult        = fileManagerDriver->Read(file, pData, tempLen);
                while(file->PendingByte < tempLen - 1) {
                }
                cacheHeader.Len    -= tempLen;
                pData              += tempLen;
            }
            
         fatFSResult = fileManagerDriver->Close(file);
         file->InProcess           = 0;
        }
        else {
            fatFSResult = FileManager_INT_ERR;
        }
    }
    else {
        if (file->Callbacks.onNotDetect != NULL) {
            file->Callbacks.onNotDetect();
        }
       fatFSResult = FileManager_DISK_ERR;
    }
    return fatFSResult;
}



/**
 * @brief this function Return Timestamp
 * 
 * @return FileManager_Timestamp 
 */
FileManager_Timestamp  FileManager_getTimeStamp (void) {
   return (FileManager_Timestamp)fileManagerDriver->GetTimestamp();
}






/**
 * @brief NonBlocking Write Data into SdCard
 * 
 * @param file Address of FileManager Struct
 * @param addr SdCard FileAddress u want to Write From that
 * @param data Address of Data u want to write in SdCard
 * @param len  Length Of Data u want to Write into SdCard
 * @param type Type of your Data FileManager_Const/Var
 * @return FileManager_Result 
 */
FileManager_Result File_write (FileManager* file, int32_t addr, uint8_t* data, int32_t len, FileManager_Type type) {
    FileManager_CommandHeader cacheHeader;
    memset(&cacheHeader.DT, 0, sizeof(cacheHeader.DT));
    cacheHeader.Addr           = addr;
    cacheHeader.Len            = len;
    cacheHeader.DataType       = type;
    cacheHeader.Mode           = FileManager_WriteMode;
    
    if(cacheHeader.Len > 0) {
        Queue_writeItem(&file->CommandQueue, &cacheHeader);
    }
    else {
        return FileManager_INVALID_PARAMETER;
    }
    switch (cacheHeader.DataType) {
        case FileManager_Var:
            Stream_writeBytes(&file->WriteStream, data, len);
            break;
        case FileManager_Const:
            Stream_writeBytes(&file->WriteStream, (uint8_t*)&data, sizeof(data));
            break;
    }
    return FileManager_OK;
}






#if FILE_MANAGER_USE_FOR_LOGGER
/**
 * @brief this Fuction use in Serialize Function in Logger library for assume Space in stream for Serialize     
 * 
 * @param file 
 * @param addr 
 * @param tempStream 
 * @param len 
 * @return Stream* 
 */
Stream* File_beginWrite (FileManager* file, Stream* tempStream, int32_t len) {
    
    if (len > 0) {
        Stream_lockWrite(&file->WriteStream, tempStream, len);
    }
    else {
        Stream_lockWrite(&file->WriteStream, tempStream, Stream_space(&file->WriteStream));
    }
    return tempStream;
}





/**
 * @brief this function use in Serialize Function in Logger Library for send Command and unlock Stream 
 * 
 * @param file Address of FileManager
 * @param addr 
 * @param tempStream 
 */
void File_endWrite (FileManager* file, int32_t addr, Stream* tempStream) {
    FileManager_CommandHeader cacheHeader;
    memset(&cacheHeader.DT, 0, sizeof(cacheHeader.DT));
    cacheHeader.Addr     = addr;
    cacheHeader.Len      = Stream_available(tempStream);
    cacheHeader.DataType = FileManager_Var;
    cacheHeader.Mode     = FileManager_WriteMode;
    Queue_writeItem(&file->CommandQueue, &cacheHeader);
    Stream_unlockWrite(&file->WriteStream, tempStream);
}





/**
 * @brief this Function use in DeSerialize Function in Logger Library
 * 
 * @param file Address of FileManger 
 * @param addr Address in File u Want Read From that
 * @param tempStream Address of Stream
 * @param len Length 
 * @return Stream* 
 */
Stream* File_beginRead (FileManager* file, int32_t addr, Stream* tempStream, int32_t len) {
    FileManager_CommandHeader cacheHeader;
    memset(&cacheHeader.DT, 0, sizeof(cacheHeader.DT));
    cacheHeader.Addr           = addr;
    cacheHeader.Len            = len;
    cacheHeader.DataType       = FileManager_Var;
    cacheHeader.Mode           = FileManager_ReadMode;
    Queue_writeItem (&file->CommandQueue, &cacheHeader);
    Stream_lockRead (&file->ReadStream, tempStream, len);
    return tempStream;
}




/**
 * @brief this Function use in DeSerialize Function in Logger Library
 * 
 * @param file Address Of FileManager
 * @param tempStream  Address of Stream
 */
void File_endRead (FileManager* file, Stream* tempStream) {
    Stream_unlockRead(&file->ReadStream, tempStream);
}



/**
 * @brief This Fuction use in Logger Library to get Log 
 * 
 * @param file Address of FileManager
 * @param dateTime Address Of DateTime
 * @param addr Address In File  
 * @param len Data Length
 * @return FileManager_Result 
 */
FileManager_Result File_loggerRead (FileManager* file, DateTime_X* dateTime, int32_t addr, int32_t len) {
    FileManager_CommandHeader cacheHeader;
    cacheHeader.DT.Date     = dateTime->Date;
    cacheHeader.DT.Time     = dateTime->Time;
    cacheHeader.Addr        = addr;
    cacheHeader.Len         = len;
    cacheHeader.DataType    = FileManager_Var;
    cacheHeader.Mode        = FileManager_LoggerReadMode;

    if (cacheHeader.Len < 1) {
        return FileManager_INVALID_PARAMETER;
    }
    else {
        Queue_writeItem (&file->CommandQueue, &cacheHeader);
    }
    return FileManager_OK;
}

#endif




/**
 * @brief This Function is use for NonBlocking Read from SdCard
 * 
 * @param file Address of FileManager Struct
 * @param addr SdCard FileAddress u want to Read From that
 * @param len  Length Of Data u want to Read from SdCard
 * @return FileManager_Result 
 */
FileManager_Result File_read (FileManager* file, int32_t addr, int32_t len) {
    FileManager_CommandHeader cacheHeader;
    memset(&cacheHeader.DT, 0, sizeof(cacheHeader.DT));
    cacheHeader.Addr           = addr;
    cacheHeader.Len            = len;
    cacheHeader.DataType       = FileManager_Var;
    cacheHeader.Mode           = FileManager_ReadMode;

    if(cacheHeader.Len < 1) {
        return FileManager_INVALID_PARAMETER;
    }
    else {
        Queue_writeItem(&file->CommandQueue, &cacheHeader);
    }
    return FileManager_OK;
}




FileManager_Result FileManager_setNewPath (FileManager* file, uint8_t* newPath) {
    file->Path = newPath;
}


/**
 * @brief this Fuction return lastFile Value
 * 
 * @return FileManager* 
 */
FileManager* FileManager_getLastFile (void) {
    return lastFile;
}




//In usb    Mount -> 0
//In sdCard Mount -> 1

/**
 * @brief u must use this Function in While(1) or Interrupt Function to Handle the NonBlocking Function
 * 
 * @return FileManager_Result 
 */
FileManager_Result FileManager_handle (void) {
    FileManager*              pFile = lastFile;
    FileManager_CommandHeader readCommand;///after Read
    char                      pathBuffer[MAX_PATH_LENGTH];
    Stream                    readTempStream;
    FileManager_Result        fatFsResult;
    uint16_t                  len = 0;
    while (pFile != FILE_MANAGER_NULL && pFile->InProcess != 1) {
        if (fileManagerDriver->IsDetected()) {
                if (Queue_available(&pFile->CommandQueue) > 0 && pFile->CommandHeaderInProcess.Len == 0) {
                    pFile->FirstTimeRun = 1;
                    fatFsResult = fileManagerDriver->Mount(FileManager_ForceMount);
                    Queue_readItem (&pFile->CommandQueue, &pFile->CommandHeaderInProcess);
                    
                    switch (pFile->CommandHeaderInProcess.Mode) {
                        case FileManager_WriteMode :
                        /*******************************************************************************************************************************/
                          /*
                          if (pFile->Callbacks.onGetAddress != NULL && pFile->UseForLogger == 1) {
                              pFile->Callbacks.onGetAddress(pFile);
                          }
                        */
                          /*******************************************************************************************************************************/
                            if (pFile->CommandHeaderInProcess.DataType == FileManager_Const) {
                                Stream_readBytes(&pFile->WriteStream, (uint8_t*)&pFile->ConstVal, sizeof(uint32_t));
                            }
                            break;
                        case FileManager_ReadMode:
                            memcpy (&readCommand, &pFile->CommandHeaderInProcess, sizeof(FileManager_CommandHeader));
                            break;
#if FILE_MANAGER_USE_FOR_LOGGER                        
                        case FileManager_LoggerReadMode :
                            memcpy (&readCommand, &pFile->CommandHeaderInProcess, sizeof(FileManager_CommandHeader));
                            snprintf (pathBuffer, MAX_PATH_LENGTH - 1, FILE_MANAGER_PATH_FORMAT, ((FileManager_RecFrame*)pFile->Args1)->DeviceId,
                               ((FileManager_RecFrame*)pFile->Args1)->Indicator, pFile->CommandHeaderInProcess.DT.Year, 
                               pFile->CommandHeaderInProcess.DT.Month, pFile->CommandHeaderInProcess.DT.Day,
                               pFile->CommandHeaderInProcess.DT.Hour, pFile->CommandHeaderInProcess.DT.Minute);
                               fatFsResult = fileManagerDriver->Open(pFile, (uint8_t*)pathBuffer, FileManager_OpenAlways | FileManager_Write | FileManager_Read);
                            break;
#endif                        
                   } 
               }

               if (pFile->CommandHeaderInProcess.Len > 0) {
                 if (pFile->CommandHeaderInProcess.Mode != FileManager_LoggerReadMode) {
                    fatFsResult = fileManagerDriver->Open(pFile, pFile->Path, FileManager_OpenAlways | FileManager_Write | FileManager_Read); ////Open
                 }
                 else if (pFile->CommandHeaderInProcess.Mode == FileManager_LoggerReadMode) {
                    pFile->CommandHeaderInProcess.Mode = FileManager_ReadMode;
                 }
                 
                 if (fatFsResult == FileManager_OK) {
                   pFile->FileStatus = FileManager_FileIsOpen;
              /**/ if (fileManagerDriver->FileSize(pFile) == 0 && pFile->CommandHeaderInProcess.Mode == FileManager_WriteMode && pFile->UseForLogger) {
                      if (pFile->Callbacks.onCreateFile != 0) {
                          pFile->Callbacks.onCreateFile (pFile);
                      }
                   }
//                   else {
                      if (pFile->CommandHeaderInProcess.Addr != END_OF_FILE) {
                          fatFsResult = fileManagerDriver->Lseek (pFile, pFile->CommandHeaderInProcess.Addr);
                      } 
                      else {
                          fatFsResult = fileManagerDriver->Lseek (pFile, fileManagerDriver->FileSize(pFile));
                      }
//                   
                 }
                 
               if (pFile->Callbacks.onGetAddress != NULL && pFile->UseForLogger == 1 && pFile->CommandHeaderInProcess.Mode == FileManager_WriteMode && pFile->FirstTimeRun) {
                   pFile->Callbacks.onGetAddress(pFile);
                   pFile->FirstTimeRun = 0;
               }
                
               switch (pFile->CommandHeaderInProcess.Mode) {
                  case FileManager_WriteMode :
                      len = (pFile->CommandHeaderInProcess.Len > Stream_directAvailable(&pFile->WriteStream)) ? Stream_directAvailable(&pFile->WriteStream) : pFile->CommandHeaderInProcess.Len; 
                      pFile->Overflow = len > pFile->Config->MaxSS ? 1 : 0;
                      pFile->TempLen  = pFile->Overflow ? pFile->Config->MaxSS : len;
                
                      switch (pFile->CommandHeaderInProcess.DataType) {
                          case FileManager_Const :
                              fatFsResult = fileManagerDriver->Write (pFile, pFile->ConstVal, pFile->TempLen);
                              
                              if(pFile->PendingByte < pFile->TempLen - 1) {
                                fatFsResult = FileManager_INVALID_DRIVE;
                              }
                              if (fatFsResult == FileManager_OK) {
                                pFile->CommandHeaderInProcess.Len  -= pFile->TempLen;
                                pFile->CommandHeaderInProcess.Addr += pFile->TempLen;
                                pFile->ConstVal                    += pFile->TempLen;
                              }
                              break;

                          case FileManager_Var :
                              fatFsResult = fileManagerDriver->Write(pFile, Stream_getReadPtr(&pFile->WriteStream), pFile->TempLen);
                              if (pFile->PendingByte < pFile->TempLen - 1) {
                                  fatFsResult = FileManager_INVALID_DRIVE;
                              }
                              if (fatFsResult == FileManager_OK) {
                                  Stream_moveReadPos (&pFile->WriteStream, pFile->TempLen);
                                  pFile->CommandHeaderInProcess.Len -= pFile->TempLen;                             
                              }
                              break;
                      }        
                      break;
                
                   case FileManager_ReadMode :  
                      pFile->Overflow = pFile->CommandHeaderInProcess.Len > pFile->Config->MaxSS ? 1 : 0;
                      pFile->TempLen  = pFile->Overflow ? pFile->Config->MaxSS : pFile->CommandHeaderInProcess.Len;
                      fatFsResult     = fileManagerDriver->Read (pFile, Stream_getWritePtr(&pFile->ReadStream), pFile->TempLen);
                      Stream_moveWritePos (&pFile->ReadStream, pFile->TempLen);
                      if (pFile->PendingByte < pFile->TempLen - 1) {
                          fatFsResult = FileManager_INVALID_DRIVE;
                      }
                      
                      if (fatFsResult == FileManager_OK) {
                          pFile->CommandHeaderInProcess.Len -= pFile->TempLen;
                          if (pFile->Callbacks.onRead != NULL && pFile->CommandHeaderInProcess.Len < 1) {
                              Stream_lockRead (&pFile->ReadStream, &readTempStream, readCommand.Len);
                              pFile->Callbacks.onRead (pFile, &readTempStream, &readCommand);
                              Stream_unlockRead (&pFile->ReadStream, &readTempStream);
                          }
                          pFile->CommandHeaderInProcess.Addr += pFile->TempLen;
                      }
                      break;                    
               }
               fatFsResult = fileManagerDriver->Close(pFile);
               if (fatFsResult == FileManager_OK) {
                  pFile->FileStatus = FileManager_FileIsClose;
               }
             }
           }
           else {
              //fatFsResult = fileManagerDriver->UnMount();
              if (pFile->Callbacks.onNotDetect != NULL) {
                  pFile->Callbacks.onNotDetect();
              }
            fatFsResult = FileManager_DISK_ERR;    
          }
          pFile = pFile->Previous;    
      }
    return fatFsResult;
}

        





/**
 * @brief user set Argument (in case Logger Library  u must set this Arg (Logger))
 * @param file 
 * @param arg 
 */
void FileManager_setArgs (FileManager* file, void* arg) {
    file->Args = arg;
}




/**
 * @brief get User Argument
 * @param file 
 * @return void* 
 */
void* FileManager_getArgs (FileManager* file) {
    return file->Args;
}




/**
 * @brief this Function use to Write Header 
 * 
 * @param file Address of FileManager
 * @param data Address of date(Header)
 * @param len Length of Data
 */
void File_writeHeader (FileManager* file, uint8_t* data, uint16_t len) {
    fileManagerDriver->Lseek(file, 0);
    fileManagerDriver->Write(file, data, len);
}

/*****************************************************************************/
/**
 * @brief this Function use for Blocking uint8_t Value in File 
 * 
 * @param file Address of FileManager
 * @param val Value
 * @param addr Address in File user want write there
 * @return FileManager_Result 
 */
FileManager_Result File_writeUInt8Blocking (FileManager* file, uint8_t val, int32_t addr) {
    return File_writeBlocking (file, addr, (uint8_t*)&val, sizeof(val));
}

/**
 * @brief this Function use for Blocking Read uint8_t Value in File
 * 
 * @param file Address of FileManager
 * @param addr Address in File user want Read from that
 * @return uint8_t 
 */
uint8_t File_readUInt8Blocking (FileManager* file, int32_t addr) {
    uint8_t val = 0;
    File_readBlocking (file, addr, (uint8_t*)&val, sizeof(val));
    return val;
}



/**
 * @brief this Function use for Blocking write uint16_t Value in File
 * 
 * @param file Address of FileManager 
 * @param val Value
 * @param addr Address in File user want write there
 * @return FileManager_Result 
 */
FileManager_Result File_writeUInt16Blocking (FileManager* file, uint16_t val, int32_t addr) {
    return File_writeBlocking (file, addr, (uint8_t*)&val, sizeof(val));
}

/**
 * @brief this Function use for Blocking Read uint16_t Value from File
 * 
 * @param file Address of FileManager 
 * @param addr Address in File user want Read from that
 * @return uint16_t Value 
 */
uint16_t File_readUInt16Blocking (FileManager* file, int32_t addr) {
    uint16_t val = 0;
    File_readBlocking (file, addr, (uint8_t*)&val, sizeof(val));
    return val;
}



/**
 * @brief this Function use for Blocking write uint32_t Value in File
 * 
 * @param fileAddress of FileManager 
 * @param val Value
 * @param addr Address in File user want write there
 * @return FileManager_Result 
 */
FileManager_Result File_writeUInt32Blocking (FileManager* file, uint32_t val, int32_t addr) {
    return File_writeBlocking (file, addr, (uint8_t*)&val, sizeof(val));
}

/**
 * @brief this Function use for Blocking Read uint32_t Value From File
 * 
 * @param file Address of FileManager 
 * @param addr Address in File user want Read from that
 * @return uint32_t Value 
 */
uint32_t File_readUInt32Blocking (FileManager* file, int32_t addr) {
    uint32_t val = 0;
    File_readBlocking(file, addr, (uint8_t*)&val, sizeof(val));
    return val;
}


/**
 * @brief this Function use for Blocking write uint64_t Value in File
 * 
 * @param file Address of FileManager 
 * @param val Value
 * @param addr Address in File user want write there
 * @return FileManager_Result 
 */
FileManager_Result File_writeUInt64Blocking (FileManager* file, uint64_t val, int32_t addr) {
    return File_writeBlocking(file, addr, (uint8_t*)&val, sizeof(val));
}


/**
 * @brief this Function use for Blocking Read uint64_t Value From File
 * 
 * @param file Address of FileManager 
 * @param addr Address in File user want Read from that
 * @return uint64_t Value 
 */
uint64_t File_readUInt64Blocking (FileManager* file, int32_t addr) {
    uint64_t val = 0;
    File_readBlocking (file, addr, (uint8_t*)&val, sizeof(val));
    return val;
}


/***************************** CallBack Function **************************/
void File_onRead       (FileManager* file, FileManager_ReadCallbackFn cb) {
    file->Callbacks.onRead = cb;
}

void File_onNotDetect  (FileManager* file, FileManager_noDetectSDCallbackFn cb) {
    file->Callbacks.onNotDetect = cb;
}

void File_onCreateFile (FileManager* file, FileManager_createFileCallbackFn cb) {
    file->Callbacks.onCreateFile = cb;
}
/*
void File_onChangePath (FileManager* file, FileManager_changePathCallbackFn cb) {
    file->Callbacks.onChangePath = cb;
}
*/
void File_onGetAddress (FileManager* file, FileManager_getAddressFn cb) {
    file->Callbacks.onGetAddress = cb;
}



/*********************************************************************************/

int8_t FileManager_assertMemory (uint8_t* arr1, uint8_t* arr2, uint16_t len) {
  return memcmp(arr1, arr2, len);
}

