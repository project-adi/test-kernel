#include "libs.h"
#include "libadi/core.h"
#include "libadi/metalanguages/misc/storage.h"
#include <lib/printf.h>
#include <lib/libc.h>
#include <mm/kmalloc.h>
#include <stdbool.h>

event_identify metalang_storage_identify_func = NULL;
event_transact metalang_storage_transact_func = NULL;

void log(char* format,...) {
    printf(format);
}

void exit(bool success) {
    printf("it exited yaey");
    if(!success) printf(" ... but at what cost?");
    while(1);
}

bool _memcpy(void* dest, void* src, usize n) {
    memcpy(dest, src, n);
    return true;
}

bool _memset(void* s, uint8_t c, usize n) {
    memset(s, c, n);
    return true;
}

bool _kfree(void* ptr) {
    kfree(ptr);
    return true;
}

uint32_t _register_device(metalanguage_t* metalanguages_implemented,uint32_t metalanguage_count){
    (void)metalanguages_implemented;
    (void)metalanguage_count;
    return 0;// this is a quick-and-dirty test for a single driver, we dont need to keep track of devices
}
void _unregister_device(uint32_t device_id){
    (void)device_id;
    //same here
}

metalanguage_t metalang_storage_create(event_identify identify,event_transact transact){
    metalang_storage_identify_func = identify;
    metalang_storage_transact_func = transact;
    return (metalanguage_t){
        .id = 0x400F0000,
        .lang_impl = &storage,
        .name = "Never Gonna Give You Up",
        .params = NULL
    };
}
bool metalang_storage_signal_transaction_done(adi_device_t* dev, uint32_t id){
    (void)dev;
    (void)id;
    return true;
}

adi_core_t cfr = {
    .log_info = log,
    .log_warning = log,
    .log_error = log,
    .panic = log,
    .exit = exit,

    .memcpy = _memcpy,
    .memset = _memset,

    .alloc = kmalloc,
    .free = _kfree,
    .realloc = krealloc,

    .register_device = _register_device,
    .unregister_device = _unregister_device
};

metalang_storage_t storage = {
    .create = metalang_storage_create,
    .signal_transaction_done = metalang_storage_signal_transaction_done
};