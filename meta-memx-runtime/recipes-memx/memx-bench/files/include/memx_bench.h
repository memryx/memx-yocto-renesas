
#ifndef MEMX_BENCH_H
#define MEMX_BENCH_H

#include <string>
#include "memx/memx.h"
#include "dfp.h"
#include "utils/mxpack.h"

using namespace std;
typedef struct _param{
    uint8_t     group;
    uint16_t    hours;
    uint32_t    frames;
    uint32_t    freq;
    uint32_t    voltage_mv;
    const char* dirpath;
    const char* dfp_path;
} Param;

uint32_t cal_format_size(uint8_t format, int total_size, uint16_t dim_h, uint16_t dim_w, uint16_t dim_z, uint16_t dim_c);
void convert_data(uint8_t format, int total_size, uint16_t dim_h, uint16_t dim_w, uint16_t dim_z, uint16_t dim_c, float *fmap_data, uint8_t *formatted_data);
void unconvert_data(uint8_t format, int total_size, uint16_t dim_h, uint16_t dim_w, uint16_t dim_z, uint16_t dim_c, float *fmap_data, uint8_t *formatted_data, int index);
memx_status init_device(uint8_t group, int num_chips, const char *dfp_path);
int memx_performance(Param *cmd_arg);

#endif //MEMX_BENCH_H
