#include "drivers/fs/fat/fat.h"
#include "kernel/io/log.h"
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "stdlib/assert.h"
#include "stdlib/structures/string.h"

static const char* fat_test_tag = "FAT Test";

// ---------------------------------------------------------------------------
// Helper: write a known pattern into a page, write it as a new cluster chain,
// clear the buffer, read the chain back and verify the pattern.
// ---------------------------------------------------------------------------
static void test_cluster_rw(filesystem::FAT_partition* part)
{
    log.v(fat_test_tag, "Test 3: Write / read cluster chain...");

    uint64_t* buf = (uint64_t*)kernel::allocate_pages(1);
    mmap(buf, buf);

    // Write two magic values at well-known offsets
    buf[42]  = 0xDEADBEEFCAFEBABE;
    buf[256] = 0x1234567890ABCDEF;

    int start_cluster = filesystem::write_cluster_chain(part, buf, 0x1000);
    assert(start_cluster > 0);

    // Clear the buffer so we know any match came from the disk
    kernel::memset(buf, 0x1000, 0);

    filesystem::read_cluster_chain(part, buf, (uint32_t)start_cluster);

    assert(buf[42]  == 0xDEADBEEFCAFEBABE);
    assert(buf[256] == 0x1234567890ABCDEF);

    log.v(fat_test_tag, "  PASS: cluster chain write/read verified");

    // Overwrite the same chain (in-place write)
    buf[42] = 0xBEEFDEAD00112233;
    filesystem::write_cluster_chain(part, buf, 0x1000, (uint64_t)start_cluster);
    kernel::memset(buf, 0x1000, 0);

    filesystem::read_cluster_chain(part, buf, (uint32_t)start_cluster);
    assert(buf[42] == 0xBEEFDEAD00112233);

    log.v(fat_test_tag, "  PASS: in-place cluster chain overwrite verified");

    kernel::free_pages(buf);
}

// ---------------------------------------------------------------------------
// Helper: look up /test.txt, read it and verify the content starts with "hi"
// ---------------------------------------------------------------------------
static void test_file_lookup(filesystem::FAT_partition* part)
{
    log.v(fat_test_tag, "Test 4: Look up and read /test.txt...");

    stdlib::string filepath("/test.txt");
    filesystem::fat_dir_entry* entry = filesystem::get_file_entry(part, &filepath);

    if (entry == NULL) {
        log.w(fat_test_tag, "  SKIP: /test.txt not found on disk");
        return;
    }

    filesystem::print_file_info(entry);

    // Make sure it is a plain file (not a directory)
    assert(!(entry->dir_attrib & filesystem::DIRECTORY));

    // Read the file data and verify the first bytes match what we put there
    void* data = filesystem::read_file_entry(part, entry);
    assert(data != NULL);

    const char* text = (const char*)data;
    assert(text[0] == 'h');
    assert(text[1] == 'i');

    log.v(fat_test_tag, "  PASS: /test.txt content verified (\"%c%c\")", text[0], text[1]);

    kernel::free_pages(data);
}

// ---------------------------------------------------------------------------
// Public entry point declared in tests/drivers.h
// ---------------------------------------------------------------------------
void testFATDriver(filesystem::FAT_partition* part)
{
    log.v(fat_test_tag, "=== FAT32 Driver Tests ===");

    // ------------------------------------------------------------------
    // Test 1: Partition metadata sanity checks
    // ------------------------------------------------------------------
    log.v(fat_test_tag, "Test 1: Partition metadata...");

    assert(part != NULL);
    assert(part->bpb != NULL);
    assert(part->bpb->magic_number == 0xAA55);
    assert(part->fat != NULL);
    assert(part->fsinfo != NULL);
    assert(part->fsinfo->signature_1 == FSINFO_SIGNATURE_1);
    assert(part->fsinfo->signature_2 == FSINFO_SIGNATURE_2);
    assert(part->fsinfo->signature_3 == FSINFO_SIGNATURE_3);
    assert(part->first_data_sector > part->first_fat_sector);
    assert(part->total_clusters > 0);

    log.v(fat_test_tag, "  PASS: BPB and FSINFO valid (vol: %.11s, clusters: 0x%x)",
          part->bpb->volume_label, part->total_clusters);

    // ------------------------------------------------------------------
    // Test 2: Root directory is readable and has a non-zero size
    // ------------------------------------------------------------------
    log.v(fat_test_tag, "Test 2: Root directory read...");

    uint32_t root_cluster = (uint32_t)part->bpb->root_dir_cluster;
    uint32_t root_size = filesystem::get_file_size(part, root_cluster);
    assert(root_size > 0);

    log.v(fat_test_tag, "  Root dir: cluster 0x%x, size 0x%x bytes", root_cluster, root_size);
    filesystem::print_directory_contents(part, &part->root_dir);

    log.v(fat_test_tag, "  PASS: root directory listed successfully");

    // ------------------------------------------------------------------
    // Test 3: Low-level cluster-chain write / read
    // ------------------------------------------------------------------
    test_cluster_rw(part);

    // ------------------------------------------------------------------
    // Test 4: File lookup and content read
    // ------------------------------------------------------------------
    test_file_lookup(part);

    // ------------------------------------------------------------------
    log.v(fat_test_tag, "=== FAT DRIVER TEST SUCCESS ===");
}
