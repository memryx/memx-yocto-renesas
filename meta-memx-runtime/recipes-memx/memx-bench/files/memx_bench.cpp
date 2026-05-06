/***************************************************************************//**
 * @file memx_bench.cpp
 * @brief inference benchmark
 *
 * Basic steps to benchmark single model DFP inference.
 *
 * @note
 * Copyright (C) 2019-2022 MemryX Limited. All rights reserved.
 *
 ******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <getopt.h>
#include <condition_variable>
#include <dirent.h>
#include <sys/stat.h>

#include "convert.h"
#include "memx_bench.h"

#if defined(__ANDROID__)
#include <jni.h>
#include <android/log.h>
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "NativeLib"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#else
#define ALOGE(...) fprintf(stderr, __VA_ARGS__)
#define ALOGD(...) fprintf(stdout, __VA_ARGS__)
#define ALOGW(...) fprintf(stdout, __VA_ARGS__)
#endif

using namespace std;

std::atomic_bool runflag;
std::atomic<uint64_t> g_frame_count(0);
std::mutex g_mtx;
std::condition_variable g_cv;
bool g_first_sent = false;

#define DEFAULT_QUEUE_SIZE (10)
#define DEFAULT_MODEL (0)
#define DEFAULT_GROUP (0)
#define DEFAULT_FRAME (100)
#define DEFAULT_HOURS (0)

#define FOLDER_STR_MAXBYTE (512)
#define DFP_FILE_NAME      "model.dfp"

vector<string> dfp_vector;

unsigned long getFileSize(const char* path) {
    unsigned long result = 0;
    FILE* fp = fopen(path, "rb");
    if (fp != NULL) {
        fseek(fp, 0, SEEK_END);
        result = ftell(fp);
        fclose(fp);
    }

    return result;
}

static void show_parameter(Param* param) {
    ALOGW("=== Arguments ===\r\n\n");
    if (param->dfp_path)
        ALOGW("DfpPath: %s\r\n",param->dfp_path);
    if (param->dirpath)
        ALOGW("DirPath: %s\r\n",param->dirpath);
    ALOGW("Group (device ID): %d\r\n", param->group);
    ALOGW("Frequency: %d MHz\r\n", param->freq);
    ALOGW("Voltage: %d mV\r\n\n", param->voltage_mv);
    
    if (param->hours > 0) {
        ALOGW("=== MODE: Continuous inference for %d hour(s) ===\r\n\n", param->hours);
    } else {
        ALOGW("=== MODE: Single run (per DFP) with %d frames ===\r\n\n", param->frames);
    }
}

static void print_usage(int argc, char **argv) {
    (void)argc;
    std::cout << "Usage: " << argv[0] << " [options] \r\n\n" <<
                "Options:\r\n" <<
                    "-h | --help              Print this message\r\n" <<
                    "-d | --dir               Directory to explore DFP file (Required)\r\n" <<
                    "-m | --model             DFP file path (Required)\r\n" <<
                    "-g | --group             Accelerator group ID, default = "<<DEFAULT_GROUP<<" \r\n" <<
                    "-f | --frames            Number of frames for single run, default = "<<DEFAULT_FRAME<<" \r\n" <<
                    "-t | --hours             Run continuous inference for specified hours\r\n" <<
                    "-q | --freq              Set frequency in MHz, default = 200\r\n" <<
                    "-v | --voltage           Set voltage in mV, default = 670\r\n" <<
                    "\r\n" <<
                    "Note: -f and -t are mutually exclusive.\r\n" <<
                    "  Use -f for single run with fixed frame count\r\n" <<
                    "  Use -t for continuous inference for specified duration\r\n" <<
                    " ";
}

static const char short_options[] = "hd:m:g:f:bt:q:v:";

static const struct option
    long_options[] = {
        {"help",        no_argument,        NULL,   'h'},
        {"dir",         required_argument,  NULL,   'd'},
        {"model",       required_argument,  NULL,   'm'},
        {"group",       required_argument,  NULL,   'g'},
        {"frames",      required_argument,  NULL,   'f'},
        {"hours",       required_argument,  NULL,   't'},
        {"freq",        required_argument,  NULL,   'q'},
        {"voltage",     required_argument,  NULL,   'v'},
        {0, 0, 0, 0 }};

static int parse_argument(int argc, char **argv, Param *param) {

    if (argc > 1) {
        param->group  = DEFAULT_GROUP;
        param->frames = DEFAULT_FRAME;
        param->hours   = DEFAULT_HOURS;
        param->dirpath = NULL;
        param->dfp_path = NULL;
        param->freq = 200;
        param->voltage_mv = 670;
        
        bool frames_specified = false;
        bool hours_specified = false;

        while (1) {
            int idx;
            int c;
            int temp;
            int freq;
            int voltage;

            c = getopt_long(argc, argv, short_options, long_options, &idx);

            if (c == -1 ) {
                break;
            }

            switch (c) {
            case 0:
                break;
            case 'd':
                param->dirpath = optarg;
                break;
            case 'm':
                param->dfp_path = optarg;
                break;
            case 'g':
                temp = strtol(optarg, NULL, 0);
                if ((temp >= 0) && (temp < 4)) {
                    param->group = temp;
                } else {
                    ALOGE("invalid group %d\r\n",temp);
                }
                break;
            case 'f':
                temp = strtol(optarg, NULL, 0);
                if (temp > 0) {
                    param->frames = temp;
                    frames_specified = true;
                } else {
                    ALOGE("invalid frames %d\r\n",temp);
                }
                break;
            case 't':
                temp = strtol(optarg, NULL, 0);
                if (temp > 0) {
                    param->hours = temp;
                    hours_specified = true;
                } else {
                    ALOGE("invalid hours %d\r\n",temp);
                }
                break;
            case 'q':
                freq = strtol(optarg, NULL, 0);
                if (freq > 0) {
                    param->freq = freq;
                } else {
                    ALOGE("invalid freq %d\r\n",freq);
                }
                break;
            case 'v':
                voltage = strtol(optarg, NULL, 0);
                if (voltage > 0) {
                    param->voltage_mv = voltage;
                } else {
                    ALOGE("invalid voltage %d\r\n",voltage);
                }
                break;

                case 'h':
                print_usage(argc, argv);
                return 1;
                break;
            case '?':
            default:
                ALOGE("Error: Missing argument\r\n");
                print_usage(argc, argv);
                return 1;
                break;
            }
        }
        
        // Check for mutually exclusive options
        if (frames_specified && hours_specified) {
            ALOGE("Error: -f (frames) and -t (hours) are mutually exclusive. Use -f for fixed frame count or -t for time-based continuous run.\r\n");
            print_usage(argc, argv);
            return 1;
        }
    } else {
        print_usage(argc, argv);
        return 1;
    }

    show_parameter(param);
    return 0;
}

int get_all_directories_path(const char* directory_path, vector<string>& dfp_path) {
    DIR* dir = opendir(directory_path);
    if (!dir) {
        perror("opendir");
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            string subdir = string(directory_path) + "/" + entry->d_name;
            string dfp_file = subdir + "/" + DFP_FILE_NAME;

            struct stat st;
            if (stat(dfp_file.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
                dfp_path.push_back(dfp_file);
                } else {
                ALOGE("%s is missing in folder %s...\r\n", DFP_FILE_NAME, subdir.c_str());
                }
            }
        }

    closedir(dir);
    ALOGW("Found %ld DFP path(s) in %s\r\n\n", dfp_path.size(), directory_path);
    return 0;
}

uint32_t cal_format_size(uint8_t format, int total_size, uint16_t dim_h, uint16_t dim_w, uint16_t dim_z, uint16_t dim_c) {
    uint32_t formatted_featuremap_size = 0;

    switch(format)
    {
        case 2: //MX_FMT_RGB888
            formatted_featuremap_size = total_size;
            break;
        case 5: //MX_FMT_FP32
            formatted_featuremap_size = total_size * 4;
            break;
        case 4: //MX_FMT_BF16
            formatted_featuremap_size = total_size * 2;
            if (formatted_featuremap_size % 2 )
                formatted_featuremap_size += 2;
            break;
        case 0: { //MX_FMT_GBF80
            size_t num_xyz_pixels = (total_size / dim_c);
            bool   any_remainder_chs = ((dim_c % 8) != 0);
            size_t num_gbf_words = (dim_c / 8) + (any_remainder_chs ? 1 : 0);
            formatted_featuremap_size = (uint32_t)(num_xyz_pixels * num_gbf_words * 10);
            break;
        }
        case 6: { //MX_FMT_GBF80_ROW
            bool   any_remainder_chs = ((dim_c % 8) != 0);
            size_t num_gbf_words = (dim_c / 8) + (any_remainder_chs ? 1 : 0);
            // padding row size to 4 bytes-alignment
            formatted_featuremap_size = dim_h * ((dim_w * dim_z * num_gbf_words * 10 + 3) & ~0x3);
            break;
        }
        default:
            throw std::invalid_argument("Invalid featureMap data format");
            break;
    }

    return formatted_featuremap_size;
}

void convert_data(uint8_t format, int total_size, uint16_t dim_h, uint16_t dim_w, uint16_t dim_z, uint16_t dim_c,
                float *fmap_data, uint8_t *formatted_data)
{
    switch(format)
    {
        case 2: //MX_FMT_RGB888
        {
            int num_pixels = total_size / 3;
            for (int i = 0; i < num_pixels; i++) {
                formatted_data[i * 3 + 0] = (uint8_t)(std::min(1.0f, std::max(0.0f, fmap_data[i * 3 + 0])) * 255.0f); // R
                formatted_data[i * 3 + 1] = (uint8_t)(std::min(1.0f, std::max(0.0f, fmap_data[i * 3 + 1])) * 255.0f); // G
                formatted_data[i * 3 + 2] = (uint8_t)(std::min(1.0f, std::max(0.0f, fmap_data[i * 3 + 2])) * 255.0f); // B
            }
            break;
        }
        case 5: //MX_FMT_FP32
        {
            for (int i = 0; i < total_size; ++i) {
                uint32_t word;
                memcpy(&word, &fmap_data[i], sizeof(float));
                formatted_data[i * 4 + 0] = (uint8_t)(word & 0xFF);
                formatted_data[i * 4 + 1] = (uint8_t)((word >> 8) & 0xFF);
                formatted_data[i * 4 + 2] = (uint8_t)((word >> 16) & 0xFF);
                formatted_data[i * 4 + 3] = (uint8_t)((word >> 24) & 0xFF);
            }
            break;
        }
        case 4: //MX_FMT_BF16
        {
            convert_bf16(reinterpret_cast<uint32_t*>(fmap_data), formatted_data, total_size);
            break;
        }
        case 0: //MX_FMT_GBF80
        {
            convert_gbf(reinterpret_cast<uint32_t*>(fmap_data), formatted_data, total_size, dim_c);
            break;
        }
        case 6: //MX_FMT_GBF80_ROW
        {
            convert_gbf_row_pad(reinterpret_cast<uint32_t*>(fmap_data), formatted_data, dim_h, dim_w, dim_z, dim_c);
            break;
        }
        default:
            throw std::invalid_argument("Invalid featureMap data format");
            break;
    }

    return;
}

void unconvert_data(uint8_t format, int total_size, uint16_t dim_h, uint16_t dim_w, uint16_t dim_z, uint16_t dim_c,
                    float *fmap_data, uint8_t *formatted_data, int index)
{
    memx_status status = MEMX_STATUS_OK;
    switch(format)
    {
        case 2: //MX_FMT_RGB888
        {
            // Assumes total_size = dim_h * dim_w * dim_z * 3
            int num_pixels = total_size / 3;
            for (int i = 0; i < num_pixels; i++) {
                fmap_data[i * 3 + 0] = formatted_data[i * 3 + 0] / 255.0f; // R
                fmap_data[i * 3 + 1] = formatted_data[i * 3 + 1] / 255.0f; // G
                fmap_data[i * 3 + 2] = formatted_data[i * 3 + 2] / 255.0f; // B
            }
            break;
        }
        case 5: //MX_FMT_FP32
        {
            for (int i = 0; i < total_size; ++i) {
                uint32_t word =
                    ((uint32_t)formatted_data[i * 4 + 0]) |
                    ((uint32_t)formatted_data[i * 4 + 1] << 8) |
                    ((uint32_t)formatted_data[i * 4 + 2] << 16) |
                    ((uint32_t)formatted_data[i * 4 + 3] << 24);

                float f;
                memcpy(&f, &word, sizeof(float));
                fmap_data[i] = f;
            }
            break;
        }
        case 4: //MX_FMT_BF16
        {
			unconvert_bf16(formatted_data, reinterpret_cast<uint32_t*>(fmap_data), total_size);
            break;
        }
        case 0: //MX_FMT_GBF80
        {
            int hpoc_size = 0;
            int* hpoc_indexes = NULL;
            int num_gbf_ch = dim_c;
            status = memx_get_ofmap_hpoc(DEFAULT_MODEL, index, &hpoc_size, &hpoc_indexes);
            if (memx_status_no_error(status) && hpoc_size != 0) {
                num_gbf_ch += hpoc_size;
            }
            if (hpoc_size != 0 && hpoc_indexes) {
                unconvert_gbf_hpoc(formatted_data, reinterpret_cast<uint32_t*>(fmap_data), dim_h, dim_w, dim_z, dim_c, hpoc_size, hpoc_indexes, 0);
            } else {
                unconvert_gbf(formatted_data, reinterpret_cast<uint32_t*>(fmap_data), total_size, dim_c);
            }
            break;
        }
        case 6: //MX_FMT_GBF80_ROW
        {
            int hpoc_size = 0;
            int* hpoc_indexes = NULL;
            int num_gbf_ch = dim_c;
            status = memx_get_ofmap_hpoc(DEFAULT_MODEL, index, &hpoc_size, &hpoc_indexes);
            if (memx_status_no_error(status) && hpoc_size != 0) {
                num_gbf_ch += hpoc_size;
            }
            if (hpoc_size != 0 && hpoc_indexes) {
                unconvert_gbf_hpoc(formatted_data, reinterpret_cast<uint32_t*>(fmap_data), dim_h, dim_w, dim_z, dim_c, hpoc_size, hpoc_indexes, 1);
            } else {
                unconvert_gbf_row_pad(formatted_data, reinterpret_cast<uint32_t*>(fmap_data), dim_h, dim_w, dim_z, dim_c);
            }
            break;
        }
        default:
            throw std::invalid_argument("Invalid featureMap data format");
            break;
    }

    return;
}

int send_function(uint32_t frame, void *context, vector<float *> float_data, vector<uint8_t *> buffer)
{
    uint32_t f = 0;
    memx_status status = MEMX_STATUS_OK;
    Dfp::DfpObject *dfp_context = (Dfp::DfpObject *)context;
    Dfp::DfpMeta dfp_meta = dfp_context->get_dfp_meta();

    // If frame is 0, run continuously (for time-based mode)
    bool continuous = (frame == 0);

    while (((continuous || f < frame)) && runflag.load()) {
        for (int i = 0; i < dfp_meta.num_used_inports; i++) {
            Dfp::PortInfo* input_port = dfp_context->input_port(i);
            convert_data(input_port->format, input_port->total_size,
                        input_port->dim_h, input_port->dim_w,
                        input_port->dim_z, input_port->dim_c,
                        float_data[i], buffer[i]);

            status = memx_stream_ifmap(DEFAULT_MODEL, i, buffer[i], 0);
            if (status) {
                throw std::invalid_argument("send_function error");
            }
        }
        if (f == 0) {
            {
                std::lock_guard<std::mutex> lk(g_mtx);
                g_first_sent = true;
            }
            g_cv.notify_one();
        }
        f += 1;
    }

    return 0;
}

int recv_function(uint32_t frame, void *context, vector<float *> float_data, vector<uint8_t *> buffer)
{
    uint32_t f = 0;
    memx_status status = MEMX_STATUS_OK;
    Dfp::DfpObject *dfp_context = (Dfp::DfpObject *)context;
    Dfp::DfpMeta dfp_meta = dfp_context->get_dfp_meta();

    {
        std::unique_lock<std::mutex> lk(g_mtx);
        g_cv.wait(lk, []{ return g_first_sent; });
    }

    // If frame is 0, run continuously (for time-based mode)
    bool continuous = (frame == 0);

    while (((continuous || f < frame)) && runflag.load()) {
        for (int i = 0; i < dfp_meta.num_used_outports; i++) {
            Dfp::PortInfo* output_port = dfp_context->output_port(i);
            status = memx_stream_ofmap(DEFAULT_MODEL, i, buffer[i], 0);
            if (status) {
                throw std::invalid_argument("recv_function error");
            }
            unconvert_data(output_port->format, output_port->total_size,
                            output_port->dim_h, output_port->dim_w,
                            output_port->dim_z, output_port->dim_c,
                            float_data[i], buffer[i], i);
        }
        f += 1;
        g_frame_count.fetch_add(1); // Track total frames processed
    }

    return 0;
}

int buffer_prepare(Dfp::DfpObject& dfp_context, vector<float *>& float_input_data, vector<float *>& float_output_data,
                    vector<uint8_t *>& input_port_buffer, vector<uint8_t *>& output_port_buffer) {
    int result = 0;
    Dfp::DfpMeta dfp_meta = dfp_context.get_dfp_meta();
    string dfp_path = dfp_context.path();
    size_t pos = dfp_path.find_last_of("/\\");
    string dfp_folder = (pos != string::npos) ? dfp_path.substr(0, pos) : "";
    int f = 0;

    for (int i = 0; i < dfp_meta.num_used_inports; i++) {
        Dfp::PortInfo* input_port = dfp_context.input_port(i);
        uint32_t data_size = input_port->total_size * 4;
        if (data_size) {
            float_input_data[i] = (float *)malloc(data_size);
            if (!float_input_data[i]) {
                result = 1;
                goto CLEANUP;
            }

            string fname = dfp_folder + "/fmaps/ifmap_truth_" + to_string(i) + "_" + to_string(f);
            ifstream file(fname);
            if (!file.is_open()) {
                memset(float_input_data[i], 0, data_size);
                for (int j = 0; j < input_port->total_size; j++) {
                    float_input_data[i][j] = 0.5f;
                }
            } else {
                vector<float> values;
                string line;
                while (getline(file, line)) {
                    try {
                        float value = stod(line);
                        values.push_back(value);
                    } catch (const invalid_argument& e) {
                        ALOGE("Invalid argument: %s is not a valid float.", line.c_str());
                        return 1;
                    } catch (const out_of_range& e) {
                        ALOGE("Out of range: %s is out of range for a float.", line.c_str());
                        return 1;
                    }
                }
                file.close();
                memcpy(float_input_data[i], values.data(), data_size);
            }
        }
    }

    for (int i = 0; i < dfp_meta.num_used_outports; i++) {
        Dfp::PortInfo* output_port = dfp_context.output_port(i);
        uint32_t data_size = output_port->total_size * 4;
        if (data_size) {
            float_output_data[i] = (float *)malloc(data_size);
            if (!float_output_data[i]) {
                result = 1;
                goto CLEANUP;
            }
        }
    }

    for (int i = 0; i < dfp_meta.num_used_inports; i++) {
        Dfp::PortInfo* input_port = dfp_context.input_port(i);
        uint32_t formatted_featuremap_size = cal_format_size(input_port->format, input_port->total_size,
                                                            input_port->dim_h, input_port->dim_w,
                                                            input_port->dim_z, input_port->dim_c);

        ALOGD("Input Port %d: %d values, format = %d, total_size = %d, dim_h = %d, dim_w = %d, dim_z = %d, dim_c = %d\n",
              i, formatted_featuremap_size, input_port->format, input_port->total_size,
              input_port->dim_h, input_port->dim_w, input_port->dim_z, input_port->dim_c);

        if (formatted_featuremap_size) {
            input_port_buffer[i] = (uint8_t *)malloc(formatted_featuremap_size);
            if (!input_port_buffer[i]) {
                result = 1;
                goto CLEANUP;
            }
        }

    }

    for (int i = 0; i < dfp_meta.num_used_outports; i++) {
        Dfp::PortInfo* output_port = dfp_context.output_port(i);
        uint32_t formatted_featuremap_size = cal_format_size(output_port->format, output_port->total_size,
                                                            output_port->dim_h, output_port->dim_w,
                                                            output_port->dim_z, output_port->dim_c);

        ALOGD("Output Port %d: %d values, format = %d, total_size = %d, dim_h = %d, dim_w = %d, dim_z = %d, dim_c = %d\n",
              i, formatted_featuremap_size, output_port->format, output_port->total_size,
              output_port->dim_h, output_port->dim_w, output_port->dim_z, output_port->dim_c);

        if (formatted_featuremap_size) {
            output_port_buffer[i] = (uint8_t *)malloc(formatted_featuremap_size);
            if (!output_port_buffer[i]) {
                result = 1;
                goto CLEANUP;
            }
        }
    }

    return result;

CLEANUP:
    for (size_t i = 0; i < float_input_data.size(); i++) {
        if (float_input_data[i]) {
            free(float_input_data[i]);
            float_input_data[i] = nullptr;
        }
    }
    for (size_t i = 0; i < float_output_data.size(); i++) {
        if (float_output_data[i]) {
            free(float_output_data[i]);
            float_output_data[i] = nullptr;
        }
    }
    for (size_t i = 0; i < input_port_buffer.size(); i++) {
        if (input_port_buffer[i]) {
            free(input_port_buffer[i]);
            input_port_buffer[i] = nullptr;
        }
    }
    for (size_t i = 0; i < output_port_buffer.size(); i++) {
        if (output_port_buffer[i]) {
            free(output_port_buffer[i]);
            output_port_buffer[i] = nullptr;
        }
    }
    return result;
}

memx_status init_device(uint8_t group, int num_chips, const char *dfp_path)
{
    memx_status status = MEMX_STATUS_OK;
    uint8_t total_chip_count = 0;

    status = memx_open(DEFAULT_MODEL, group, MEMX_DEVICE_CASCADE_PLUS);
    if (status) {
        ALOGE("memx_open fail %d\r\n", status);
    } else {
        ALOGD("memx_open done\r\n");
    }

    status = memx_set_ifmap_queue_size(DEFAULT_MODEL, DEFAULT_QUEUE_SIZE);
    if (status) {
        ALOGE("memx_set_ifmap_queue_size fail %d\r\n", status);
    } else {
        ALOGD("memx_set_ifmap_queue_size done\r\n");
    }

    status = memx_set_ofmap_queue_size(DEFAULT_MODEL, DEFAULT_QUEUE_SIZE);
    if (status) {
        ALOGE("memx_set_ofmap_queue_size fail %d\r\n", status);
    } else {
        ALOGD("memx_set_ofmap_queue_size done\r\n");
    }

    status = memx_get_total_chip_count(group, &total_chip_count);
    if (status) {
        ALOGE("memx_get_total_chip_count fail %d\r\n", status);
    } else {
        ALOGD("memx_get_total_chip_count done, total_chip_count %d\r\n", total_chip_count);
        if (!total_chip_count) {
            goto DONE;
        }
    }

    switch(num_chips ) {
        case 1:
            status = memx_config_mpu_group(group, MEMX_MPU_GROUP_CONFIG_ONE_GROUP_ONE_MPU);
            break;
        case 2:
            status = memx_config_mpu_group(group, MEMX_MPU_GROUP_CONFIG_ONE_GROUP_TWO_MPUS);
            break;
        case 3:
            status = memx_config_mpu_group(group, MEMX_MPU_GROUP_CONFIG_ONE_GROUP_THREE_MPUS);
            break;
        case 4:
            status = memx_config_mpu_group(group, MEMX_MPU_GROUP_CONFIG_ONE_GROUP_FOUR_MPUS);
            break;
        case 8:
            status = memx_config_mpu_group(group, MEMX_MPU_GROUP_CONFIG_ONE_GROUP_EIGHT_MPUS);
            break;
        case 12:
            status = memx_config_mpu_group(group, MEMX_MPU_GROUP_CONFIG_ONE_GROUP_TWELVE_MPUS);
            break;
        case 16:
            status = memx_config_mpu_group(group, MEMX_MPU_GROUP_CONFIG_ONE_GROUP_SIXTEEN_MPUS);
            break;
        default:
            ALOGE("dfp compile chip count %d not support\r\n", num_chips);
            status = MEMX_STATUS_OTHERS;
            break;
    }

    if (status) {
        ALOGE("memx_config_mpu_group fail %d\r\n", status);
    } else {
        ALOGD("memx_config_mpu_group done for chip count %d\r\n", num_chips);
    }

    status = memx_download_model(DEFAULT_MODEL, dfp_path, 0, MEMX_DOWNLOAD_TYPE_WTMEM_AND_MODEL);
    if (status) {
        ALOGE("memx_download_model %s fail %d\r\n", dfp_path, status);
    } else {
        ALOGD("memx_download_model %s done\r\n", dfp_path);
    }

    status = memx_set_stream_enable(DEFAULT_MODEL, 0);
    if (status) {
        ALOGE("memx_set_stream_enable fail %d\r\n", status);
    } else {
        ALOGD("memx_set_stream_enable done\r\n");
    }

DONE:
    return status;
}

int memx_performance(Param *cmd_arg) {
    int result = 0;
    memx_status status = MEMX_STATUS_OK;

    runflag.store(true);
    if (cmd_arg->dfp_path) {
        dfp_vector.push_back(cmd_arg->dfp_path);
    } else if (cmd_arg->dirpath) {
        get_all_directories_path(cmd_arg->dirpath, dfp_vector);
    } else {
        ALOGE("Error: No dfp file or directory specified\n");
        return 1;
    }

    auto overall_start_time = std::chrono::high_resolution_clock::now();
    
    // If hours is specified, run continuously (frame=0 means infinite). Otherwise, run once with specified frames.
    bool continuous_mode = (cmd_arg->hours > 0);
    uint32_t frames_to_run = continuous_mode ? 0 : cmd_arg->frames;

    for (auto dfp : dfp_vector) {
        ALOGW("DFP: %s, starting inference...\r\n", dfp.c_str());
        Dfp::DfpObject dfp_context(dfp.c_str());
        Dfp::DfpMeta dfp_meta = dfp_context.get_dfp_meta();
        vector<float *> float_input_data(dfp_meta.num_used_inports, NULL);
        vector<float *> float_output_data(dfp_meta.num_used_outports, NULL);
        vector<uint8_t *> input_port_buffer(dfp_meta.num_used_inports, NULL);
        vector<uint8_t *> output_port_buffer(dfp_meta.num_used_outports, NULL);

        ALOGD("DFP Info: Inputs=%d, Outputs=%d, Chips=%d\r\n", dfp_meta.num_used_inports, dfp_meta.num_used_outports, dfp_meta.num_chips);

        result = buffer_prepare(dfp_context, float_input_data, float_output_data, input_port_buffer, output_port_buffer);
        if (result) {
            ALOGE("buffer_prepare fail %d\r\n", status);
            goto FAIL;
        } else {
            ALOGD("buffer_prepare done\n");
        }

        status = init_device(cmd_arg->group, dfp_meta.num_chips, dfp.c_str());
        if (status) {
            ALOGE("init_device fail %d\r\n", status);
            goto FAIL;
        } else {
            auto start = std::chrono::high_resolution_clock::now();

            std::thread send(send_function, frames_to_run, (void *)&dfp_context, float_input_data, input_port_buffer);
            std::thread recv(recv_function, frames_to_run, (void *)&dfp_context, float_output_data, output_port_buffer);
            
            // For continuous mode, monitor time and stop when limit is reached
            if (continuous_mode) {
                ALOGW("\n");
                ALOGW("╔════════════════════════════════════════════════════╗\n");
                ALOGW("║           CONTINUOUS INFERENCE MODE                ║\n");
                ALOGW("╠════════════════════════════════════════════════════╣\n");
                
                char target_str[50];
                snprintf(target_str, sizeof(target_str), "Target Duration: %d hour(s)", cmd_arg->hours);
                ALOGW("║  %-48s  ║\n", target_str);
                
                ALOGW("╚════════════════════════════════════════════════════╝\n");
                ALOGW("\n");
                
                g_frame_count.store(0); // Reset frame counter
                uint64_t last_frame_count = 0;
                auto last_update = std::chrono::high_resolution_clock::now();
                
                while (runflag.load()) {
                    std::this_thread::sleep_for(std::chrono::seconds(30));
                    auto current_time = std::chrono::high_resolution_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::hours>(current_time - overall_start_time);
                    
                    if (elapsed.count() >= cmd_arg->hours) {
                        ALOGW("\n");
                        ALOGW("╔════════════════════════════════════════════════════╗\n");
                        
                        char limit_str[50];
                        snprintf(limit_str, sizeof(limit_str), "TIME LIMIT REACHED (%d hours)", cmd_arg->hours);
                        ALOGW("║  %-48s  ║\n", limit_str);
                        
                        ALOGW("╚════════════════════════════════════════════════════╝\n");
                        runflag.store(false);
                        break;
                    }
                    
                    // Calculate elapsed time components
                    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - overall_start_time).count();
                    int hours = total_seconds / 3600;
                    int minutes = (total_seconds % 3600) / 60;
                    int seconds = total_seconds % 60;
                    
                    // Calculate FPS every 30 seconds
                    uint64_t current_frame_count = g_frame_count.load();
                    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_update).count();
                    double fps = 0.0;
                    
                    if (time_diff > 0) {
                        uint64_t frames_processed = current_frame_count - last_frame_count;
                        fps = (double)frames_processed * 1000.0 / time_diff;
                    }
                    
                    ALOGW("  [%02d:%02d:%02d] Frames: %lu | FPS: %.2f\n", 
                          hours, minutes, seconds, current_frame_count, fps);
                    
                    last_frame_count = current_frame_count;
                    last_update = current_time;
                }
            }
            
            send.join();
            recv.join();
            
            if (!runflag.load() && !continuous_mode) {
                ALOGE("Inference interrupted\n");
                goto FAIL;
            } else {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                auto duration_ms = duration.count() ? duration.count() : 1;

                size_t pos = dfp.find_last_of('/');
                string parent_path = (pos != string::npos) ? dfp.substr(0, pos) : dfp;
                size_t pos2 = parent_path.find_last_of('/');
                string folder_name = (pos2 != string::npos) ? parent_path.substr(pos2+1) : parent_path;
                
                if (continuous_mode) {
                    auto total_seconds = duration_ms / 1000;
                    int hours = total_seconds / 3600;
                    int minutes = (total_seconds % 3600) / 60;
                    int seconds = total_seconds % 60;
                    
                    // Calculate average FPS for continuous mode
                    uint64_t total_frames = g_frame_count.load();
                    double avg_fps = (double)total_frames * 1000.0 / duration_ms;
                    
                    ALOGW("\n");
                    ALOGW("╔════════════════════════════════════════════════════╗\n");
                    ALOGW("║                 BENCHMARK COMPLETE                 ║\n");
                    ALOGW("╠════════════════════════════════════════════════════╣\n");
                    ALOGW("║  %-48s  ║\n", (string("Model: ") + folder_name).c_str());
                    ALOGW("║  %-48s  ║\n", "Status: PASS");
                    
                    char duration_str[50];
                    snprintf(duration_str, sizeof(duration_str), "Duration: %02d:%02d:%02d", hours, minutes, seconds);
                    ALOGW("║  %-48s  ║\n", duration_str);
                    
                    char frames_str[50];
                    snprintf(frames_str, sizeof(frames_str), "Total Frames: %lu", total_frames);
                    ALOGW("║  %-48s  ║\n", frames_str);
                    
                    char fps_str[50];
                    snprintf(fps_str, sizeof(fps_str), "Average FPS: %.2f", avg_fps);
                    ALOGW("║  %-48s  ║\n", fps_str);
                    
                    char freq_str[50];
                    snprintf(freq_str, sizeof(freq_str), "Frequency: %d MHz", cmd_arg->freq);
                    ALOGW("║  %-48s  ║\n", freq_str);
                    
                    char voltage_str[50];
                    snprintf(voltage_str, sizeof(voltage_str), "Voltage: %d mV", cmd_arg->voltage_mv);
                    ALOGW("║  %-48s  ║\n", voltage_str);
                    
                    ALOGW("╚════════════════════════════════════════════════════╝\n");
                    ALOGW("\n");
                } else {
                    double fps = (double)(cmd_arg->frames * 1000) / duration_ms;
                    ALOGW("\n");
                    ALOGW("╔════════════════════════════════════════════════════╗\n");
                    ALOGW("║                 BENCHMARK COMPLETE                 ║\n");
                    ALOGW("╠════════════════════════════════════════════════════╣\n");
                    ALOGW("║  %-48s  ║\n", (string("Model: ") + folder_name).c_str());
                    ALOGW("║  %-48s  ║\n", "Status: PASS");
                    
                    char frames_str[50];
                    snprintf(frames_str, sizeof(frames_str), "Frames: %d", cmd_arg->frames);
                    ALOGW("║  %-48s  ║\n", frames_str);
                    
                    char fps_str[50];
                    snprintf(fps_str, sizeof(fps_str), "FPS: %.2f", fps);
                    ALOGW("║  %-48s  ║\n", fps_str);
                    
                    char freq_str[50];
                    snprintf(freq_str, sizeof(freq_str), "Frequency: %d MHz", cmd_arg->freq);
                    ALOGW("║  %-48s  ║\n", freq_str);
                    
                    char voltage_str[50];
                    snprintf(voltage_str, sizeof(voltage_str), "Voltage: %d mV", cmd_arg->voltage_mv);
                    ALOGW("║  %-48s  ║\n", voltage_str);
                    
                    ALOGW("╚════════════════════════════════════════════════════╝\n");
                    ALOGW("\n");
                }
            }
        }

FAIL:
        memx_close(cmd_arg->group);

        for (size_t i = 0; i < float_input_data.size(); i++) {
            if (float_input_data[i]) {
                free(float_input_data[i]);
                float_input_data[i] = nullptr;
            }
        }
        for (size_t i = 0; i < float_output_data.size(); i++) {
            if (float_output_data[i]) {
                free(float_output_data[i]);
                float_output_data[i] = nullptr;
            }
        }
        for (size_t i = 0; i < input_port_buffer.size(); i++) {
            if (input_port_buffer[i]) {
                free(input_port_buffer[i]);
                input_port_buffer[i] = nullptr;
            }
        }
        for (size_t i = 0; i < output_port_buffer.size(); i++) {
            if (output_port_buffer[i]) {
                free(output_port_buffer[i]);
                output_port_buffer[i] = nullptr;
            }
        }

        if (status) {
            break;
        }
    }

    return result;
}

void signal_handler(int p_signal) {
    (void)p_signal;
    runflag.store(false);
    std::this_thread::sleep_for (std::chrono::seconds(1));
}

int set_freq_voltage(int freq, int voltage)
{
    memx_status status = MEMX_STATUS_OK;
    int group_id = 0;
    int chip_id = 0;

    if (freq > 1000) { freq = 1000; }
    if (freq < 200) { freq = 200; }
    if (voltage > 950) { voltage = 950; }
    if (voltage < 700) { voltage = 700; }

    status = memx_set_feature(group_id, 0xff, OPCODE_SET_FREQUENCY, freq);
    if (status) {
        ALOGE("set_freq_voltage fail %d\r\n", status);
        return 1;
    }
    status = memx_set_feature(group_id, chip_id, OPCODE_SET_VOLTAGE, voltage);
    if (status) {
        ALOGE("set_freq_voltage fail %d\r\n", status);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    Param cmd_arg = {0, 0, 0, 0, 0, NULL, NULL};
    signal(SIGINT, signal_handler);

    if (parse_argument(argc, argv, &cmd_arg) != 0)
        return EXIT_FAILURE;

    if (set_freq_voltage(cmd_arg.freq, cmd_arg.voltage_mv) != 0)
        return EXIT_FAILURE;

    if (memx_performance(&cmd_arg) != 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
