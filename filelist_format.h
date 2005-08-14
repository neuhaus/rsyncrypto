#ifndef FILELIST_FORMAT_H
#define FILELIST_FORMAT_H

// Filelist special values

#define FILELIST_MAGIC_VER1 0x15c8fe60

struct block_std_header {
    uint16_t blk_size;
    uint16_t blk_type;
};

// The block types
static const uint16_t BLK_TYPE_PLATFORM=0, // Platform
	     BLK_TYPE_OFILENAME=0x0001, // Original file name
	     BLK_TYPE_EFILENAME=0x0002, // Encoded file name
	     BLK_TYPE_POSIX_PERM=0x0003, // Posix file permission
	     BLK_TYPE_NOP=0xfffe, // No Operation
	     BLK_TYPE_EOC=0xffff; // End of chunk

struct block_platform {
    struct block_std_header header;
    uint8_t platform;
    char dir_sep;
    char name[];
};

struct block_ofilename {
    struct block_std_header header;
    uint8_t file_type;
    uint8_t file_name_enc;
    char name[];
};

// file types
static const uint8_t FILETYPE_FIFO=0x01, // Named pipe
	     FILETYPE_CHRDEV=0x02, // Character device
	     FILETYPE_DIR=0x04, // Directory
	     FILETYPE_BLKDEV=0x06, // Block device
	     FILETYPE_REG=0x10, // Regular file
	     FILETYPE_LINK=0x12, // Symbolic link
	     FILETYPE_SOCK=0x14;

// OS and filesystem types. Taken from the gzip definition.
static const uint16_t OS_TYPE_FAT=0, // FAT filesystem
	     OS_TYPE_AMIGA=1,
	     OS_TYPE_VMS=2,
	     OS_TYPE_UNIX=3,
	     OS_TYPE_VM_CMS=4,
	     OS_TYPE_ATARI_TOS=5,
	     OS_TYPE_HPFS=6,
	     OS_TYPE_MAC=7,
	     OS_TYPE_Z_SYSTEM=8,
	     OS_TYPE_CPM=9,
	     OS_TYPE_TOPS_20=10,
	     OS_TYPE_NTFS=11,
	     OS_TYPE_QDOS=12,
	     OS_TYPE_ACORN_RISCOS=13,
	     OS_TYPE_UNKNOWN=255;

#endif // FILELIST_FORMAT_H
