#include "utils.h"

#include "wifi_debug.h"

void hexdump(const void* data, size_t size)
{
    u32 offset = 0;
    const size_t line_max = 12;

    while(size)
    {
        size_t current_size = min(line_max, size);

        for(int i = 0; i < current_size; i++)
        {
            wifi_printf("%02X ", ((u8*)data)[offset + i]);
        }

        wifi_printf("\n");

        size -= current_size;
        offset += current_size;
    }
}
