#include "adi.h"
#include "adi_ff.h"
#include <mm/vmm.h>
#include <mm/pmm.h>

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

    pagemap* pm = vmm_new_pm();

    for(int i = 0; i*sizeof(adi_ff_segment_t) < header->segment_table_size; i++){
        adi_ff_segment_t* segment = (adi_ff_segment_t*)(data + header->segment_table_offset + i*sizeof(adi_ff_segment_t));

        u64 flags = PTE_NX;
        if(segment->flags & ADI_FF_SEGMENT_FLAG_READ) flags |= PTE_PRESENT;
        if(segment->flags & ADI_FF_SEGMENT_FLAG_WRITE) flags |= PTE_WRITABLE;
        if(segment->flags & ADI_FF_SEGMENT_FLAG_EXEC) flags &= ~PTE_NX;

        vmm_map_range(pm, segment->virt_addr, (u64)PHYSICAL(data + header->content_region_offset + segment->segment_offset), DIV_ROUND_UP(segment->segment_size,PAGE_SIZE), flags);

        printf("Segment \"%s\" mapped!\n", get_string(data, segment->name_offset));
    }

}