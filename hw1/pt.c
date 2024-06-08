#include "os.h"
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#define NEW_VALID_PTE(ppn) ((ppn << 12) | 0x1)
#define VPN_TO_INDEX(vpn, level) (vpn >> (9 * (4 - level))) & 0x1ffULL
/**
 * Gets the next node pointed to by the PTE at index, if it exists.
 * If it doesn't exist, if `create = 0`, return 0, if `create != 0`, allocate a
 * new page table node.
 * */
uint64_t *get_next_node(uint64_t *table_node, uint16_t index, int create) {
    uint64_t pte = table_node[index];
    if (pte & 0x1) {
        // PTE is valid
        uint64_t frame_address = pte & 0xfffffffffffffe00; // Zero last 12 bits
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

uint64_t *get_terminal_pte(uint64_t *table, uint64_t vpn, int create) {
    uint16_t index;
    for (int i = 0; i < 4; i++) {
        index = VPN_TO_INDEX(vpn, i);
        table = get_next_node(table, index, create);
        if (!table)
            return 0;
    }
    index = VPN_TO_INDEX(vpn, 4);
    return &table[index];
}

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn) {
    uint64_t *table = phys_to_virt(pt << 12);
    uint64_t *terminal_pte = get_terminal_pte(table, vpn, ppn != NO_MAPPING);
    if (!terminal_pte) // VPN is unmapped and we didn't create a new PTE for it.
        return;
    if (ppn == NO_MAPPING) {
        *terminal_pte = 0;
    } else {
        *terminal_pte = NEW_VALID_PTE(ppn);
    }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn) {
    uint64_t *table = phys_to_virt(pt << 12);
    uint64_t *terminal_pte = get_terminal_pte(table, vpn, 0);
    if (!terminal_pte || !(*terminal_pte & 0x1))
        return NO_MAPPING;
    return (*terminal_pte) >> 12;
}
