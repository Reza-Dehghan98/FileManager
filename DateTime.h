#ifndef _DATE_TIME_H_
#define _DATE_TIME_H_

#include <stdint.h>

typedef struct {
    uint8_t                Year;
    uint8_t                Month;
    uint8_t                Day;
} Date;

typedef struct {
    uint8_t                Hour;
    uint8_t                Minute;
    uint8_t                Second;
} Time;


typedef struct {
    union {
       struct {
           uint8_t         Year; 
           uint8_t         Month;  
           uint8_t         Day;
                   
       }; 
       Date Date;
    };    
    union {
        struct {
           uint8_t         Hour;
           uint8_t         Minute;
           uint8_t         Second;
        };
        Time Time;
    };
} DateTime_X;


#endif // _DATE_TIME_H_
