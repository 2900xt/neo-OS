#include <types.h>


//Misc memory functionss

void memset_64(void* _addr, uint64_t num, uint64_t value){

    //Round up!

    if(num % 8 != 0){
        uint8_t remainder = num % 8;
        num -= remainder;
        num += 8;
    }

    num /= 8;

    uint64_t* addr = (uint64_t*)_addr;

    for(int i = 0; i < num; i++){
        addr[i] = value;
    }
}

void memset_8(void* _addr, uint64_t num, uint8_t value){
    uint8_t* addr = (uint8_t*)_addr;

    for(int i = 0; i < num; i++){
        addr[i] = value;
    }
}

void memcpy(void* _destination, void* _src, uint64_t num){

    if(num % 8 != 0){
        uint8_t remainder = num % 8;
        num -= remainder;
        num += 8;
    }

    uint8_t* destPtr = (uint8_t*)_destination;
    uint8_t* srcPtr = (uint8_t*)_src;

    for(int i = 0; i < num; i++){
        destPtr[i] = srcPtr[i];
    }

}
