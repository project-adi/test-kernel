#include "adi.h"
#include "adi_ff.h"
#include "libs.h"
#include <mm/vmm.h>
#include <mm/pmm.h>

#include <lib/printf.h>

const char* get_string(const char* data, uint16_t offset){
    adi_ff_header_t* header = (adi_ff_header_t*)data;
    return data + header->string_table_offset + offset;
}

void poke(pagemap* pm, uptr addr, u64 val){
    void* newaddr = HIGHER_HALF(vmm_get_region_paddr(pm, addr) + (addr & 0x0FFF));
    *(u64*)newaddr = val;
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

    poke(pm, header->cfr_addr, (u64)&cfr);

    for(int i = 0; i*sizeof(adi_ff_segment_t) < header->segment_table_size; i++){
        adi_ff_segment_t* segment = (adi_ff_segment_t*)(data + header->segment_table_offset + i*sizeof(adi_ff_segment_t));

        u64 flags = (segment->flags & ADI_FF_SEGMENT_FLAG_EXEC) ? 0 : PTE_NX;
        if(segment->flags & ADI_FF_SEGMENT_FLAG_READ) flags |= PTE_PRESENT;
        if(segment->flags & ADI_FF_SEGMENT_FLAG_WRITE) flags |= PTE_WRITABLE;


        vmm_map_range(pm, segment->virt_addr, (u64)PHYSICAL(data + header->content_region_offset + segment->segment_offset), DIV_ROUND_UP(segment->segment_size,PAGE_SIZE), flags);

        printf("Segment \"%s\" mapped at 0x%lx spaning 0x%x pages with falgs 0x%lx!\n", get_string(data, segment->name_offset), segment->virt_addr, DIV_ROUND_UP(segment->segment_size,PAGE_SIZE), flags);
    }

    for(int i = 0; i*sizeof(adi_ff_metalang_t) < header->metalang_table_size; i++){
        adi_ff_metalang_t* metalang = (adi_ff_metalang_t*)(data + header->metalang_table_offset + i*sizeof(adi_ff_metalang_t));
        switch(metalang->id){
            case 0x00000001:
                printf("x86_64 metalang found!\n");
                poke(pm, metalang->pointer_addr, 0);//TODO: inject a pointer to actual code here
                break;
            case 0x00010000:
                printf("PCI metalang found!\n");
                poke(pm, metalang->pointer_addr, 0);//TODO: inject a pointer to actual code here
                break;
            case 0x400F0000:
                printf("Storage metalang found!\n");
                poke(pm, metalang->pointer_addr, (u64)&storage);
                break;
            default:
                printf("Unsupported metalang(only storage works rn) id: %d\n", metalang->id);
        }
    }


    printf("ADI loaded!\n");

    void(*init)(void) = (void(*)())header->entry_point;

    __asm__ volatile("mov %%rsp, %0" : "=r"(stackptr));

    uptr stackptr = 0;
    void* new_stack = pmm_alloc(4);
    vmm_map_range(pm, (uptr)stackptr, (uptr)new_stack, 4, PTE_PRESENT | PTE_WRITABLE);

    vmm_switch_pm(pm);
    init();

    printf("Error: ADI driver exited without calling \"core->exit()\"!!!\n");
}