#ifndef __IST8310_H
#define __IST8310_H

#include <stdint.h>

typedef struct ist8310_raw_data_t {
    float x;
    float y;
    float z;
} ist8310_raw_data_t;

typedef enum ist8310_error_e {
    IST8310_NO_ERROR = 0x00,
    MEG_ID_ERROR = 0x01,
    ist8310_no_init =0x02,
} ist8310_error_e;

typedef struct ist8310_data_t {
    uint8_t chip_id;
    uint8_t status;
    ist8310_error_e ist8310_error;
    ist8310_raw_data_t raw_data;
} ist8310_data_t;

extern ist8310_data_t ist8310_data;

// 功能函数
void ist8310_init(void);
void ReadIST8310Data();

// 校验函数
ist8310_error_e VerifyMegId(uint8_t* id);


#endif