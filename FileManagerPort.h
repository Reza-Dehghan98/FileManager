


#ifndef _FILE_MANAGER_PORT_H_
#define _FILE_MANAGER_PORT_H_

#ifdef _cplusplus
extern "C" {
#endif

#include "fatfs.h"
#include "FileManager.h"

FileManager_Result    FileManager_userOpen             (FileManager* file, uint8_t* path, FileManager_OpenMethod openMethod);
FileManager_Result    FileManager_userWrite            (FileManager* file, void* data, int32_t len);
FileManager_Result    FileManager_userRead             (FileManager* file, void* data, int32_t len);
FileManager_Result    FileManager_userMount            (FileManager_MountMethod mountMethod);
FileManager_Result    FileManager_userUnMount          (void);
FileManager_Result    FileManager_userLseek            (FileManager* file, int32_t addr);
FileManager_Result    FileManager_userClose            (FileManager* file);
FileManager_Result    FileManager_userSync             (FileManager* file);
uint8_t               FileManager_userIsOpen           (FileManager* file);
uint32_t              FileManager_userGetSize          (FileManager* file);
FileManager_Result    FileManager_userDelete           (uint8_t* path);
uint8_t               FileManager_userBSP_SdDetect     (void);
FileManager_Result    FileManager_userUnLink           (uint8_t* path);
FileManager_Timestamp FileManager_userGetTimestamp     (void);




extern  const FileManager_Driver myFileManagerDriver;
extern  const FileManager_Config myFileConfig;






/*
void  FileManager_userOnCreateFile (FileManager* file);
void  FileManager_userOnGetAddress (FileManager* file);
*/





#ifdef __cplusplus
};
#endif

#endif /* _FILE_MANAGER_H_ */

