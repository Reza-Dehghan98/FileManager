#include "FileManagerPort.h"

const FileManager_Driver myFileManagerDriver = {
    FileManager_userOpen,
    FileManager_userWrite,
    FileManager_userRead,
    FileManager_userMount,
    FileManager_userUnMount,
    FileManager_userLseek,
    FileManager_userClose,
    FileManager_userIsOpen,
    FileManager_userGetSize,
    FileManager_userBSP_SdDetect,
    FileManager_userUnLink,
    FileManager_userGetTimestamp,
};

 const FileManager_Config myFileConfig = {
    _MAX_SS,
};

FileManager_Result FileManager_userErase (FileManager* file) {
    return (FileManager_Result) f_truncate (file->Context);
}

FileManager_Result FileManager_userOpen (FileManager* file, uint8_t* path, FileManager_OpenMethod openMethod) {
    return (FileManager_Result) f_open (file->Context, (const TCHAR*)file->Path, openMethod);
}

FileManager_Result FileManager_userWrite (FileManager* file , void* data, int32_t len) {
    return (FileManager_Result) f_write (file->Context, data, len, &file->PendingByte);
}

FileManager_Result FileManager_userRead (FileManager* file, void* data, int32_t len) {
    return (FileManager_Result) f_read (file->Context, data, len, &file->PendingByte);
}


FileManager_Result FileManager_userMount (FileManager_MountMethod mountStatus) {
    return (FileManager_Result) f_mount (&SDFatFS, SDPath, mountStatus);
}


FileManager_Result FileManager_userUnMount (void) {
    return (FileManager_Result) f_mount (0, SDPath, 0);
}

FileManager_Result FileManager_userLseek (FileManager* file, int32_t addr) {
    return (FileManager_Result) f_lseek (file->Context, addr);
}

FileManager_Result FileManager_userClose (FileManager* file){
    return (FileManager_Result) f_close (file->Context);
}

uint8_t FileManager_userBSP_SdDetect (void) {
    return (uint8_t) BSP_SD_IsDetected();
}

uint8_t FileManager_userIsOpen (FileManager* file) {
    uint8_t result = 0;
    result = (uint8_t) (((FIL*) file->Context)->obj.fs) == NULL ? 0 : 1;
    return result;
}

uint32_t FileManager_userGetSize (FileManager* file) {
    return f_size((FIL*)file->Context);    
}


FileManager_Result    FileManager_userUnLink (uint8_t* path) {
  return   (FileManager_Result) f_unlink((const TCHAR*)path);
}
FileManager_Timestamp FileManager_userGetTimestamp (void) {
    return (FileManager_Timestamp) HAL_GetTick();
}

