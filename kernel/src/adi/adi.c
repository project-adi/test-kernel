#include "adi.h"
#include "adi_ff.h"
#include <fs/vfs.h>

#include <lib/printf.h>

const char* get_string(const char* data, uint16_t offset){
    adi_ff_header_t* header = (adi_ff_header_t*)data;
    return data + header->string_table_offset + offset;
}

void load_adi(const char* data) {
    adi_ff_header_t* header = (adi_ff_header_t*)data;

    if (header->magic_num != ADI_FF_MAGIC) {
        printf("ADI: Invalid magic number\n");
        return;
    }

    if (header->spec_version != 0x0001) {
        printf("ADI: Invalid spec version\n");
        return;
    }

    printf("Driver Name: %s\n", get_string(data, header->name_offset));
    printf("Driver Author: %s\n", get_string(data, header->author_offset));
    printf("Driver Version: v%d.%d.%d\n", 
        header->ver_major, header->ver_minor, header->ver_patch);


}