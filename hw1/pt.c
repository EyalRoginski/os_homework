#include "os.h"
#include <stdint.h>
#include <string.h>

#define NEW_VALID_PTE(ppn) ((ppn << 12) | 0x1)
/**
 * Gets the next node pointed to by the PTE at index, if it exists.
 * If it doesn't exist, if `create = 0`, return 0, if `create != 0`, allocate a
 * new page table node.
 * */
uint64_t *get_next_node(uint64_t *table_node, int index, int create) {
    uint64_t pte = table_node[index];
    if (pte & 0x1) {
        // PTE is valid
        uint64_t frame_address = pte & 0xfffffe00; // Zero last 12 bits
        return phys_to_virt(frame_address);
    }
    if (!create) {
        return 0;
    }
    uint64_t new_page = alloc_page_frame();
    uint64_t new_pte = NEW_VALID_PTE(new_page);
    table_node[index] = new_pte;
    uint64_t *new_node = phys_to_virt(new_page << 12);
    memset((void *)new_node, 0, 512);
    return new_node;
}

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn) {
    if (ppn != NO_MAPPING) {
        uint64_t *table_node = phys_to_virt(pt << 12);
        uint64_t bitmask;
        int index;
        for (int i = 0; i < 4; i++) {
            bitmask = 0x1ff << (12 + (i - 4) * 9);
            index = vpn & bitmask;
            table_node = get_next_node(table_node, index, 1);
        }
        bitmask = 0x1ff << 12;
        index = vpn & bitmask;
        table_node[index] = NEW_VALID_PTE(ppn);
    } else {
        uint64_t *table_node = phys_to_virt(pt << 12);
        uint64_t bitmask;
        int index;
        for (int i = 0; i < 4 && table_node; i++) {
            bitmask = 0x1ff << (12 + (i - 4) * 9);
            index = vpn & bitmask;
            table_node = get_next_node(table_node, index, 0);
        }
        bitmask = 0x1ff << 12;
        index = vpn & bitmask;
        table_node[index] = 0;
    }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn) { return NO_MAPPING; }
