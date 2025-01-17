#ifndef vivenas_internal_h__
#define vivenas_internal_h__
#include "rocksdb/db.h"
#include <rocksdb/utilities/transaction_db.h>
#include "rocksdb/options.h"

#include <memory>
#include <string>
#include <stdint.h>
#include <linux/types.h>
#include "vn_vivenas.h"

struct ViveSuperBlock;

#define VIVEFS_MAGIC_STR "vivefs_0"
#define VIVEFS_VER 0x00010000

#define LBA_SIZE 4096
#define LBA_SIZE_ORDER 12
#define VIVEFS_EXTENT_SIZE (64<<10)

struct ViveSuperBlock
{
	std::string magic;
	int32_t version;
};




struct vn_inode_no_t {
	int64_t i_no;
	explicit vn_inode_no_t(int64_t i) :i_no(i) {}
	__always_inline int64_t  to_int() {
		return i_no;
	}
	struct vn_inode_no_t from_int(int64_t i_no) {
		return vn_inode_no_t{ i_no };
	}
};

struct ViveInode;
struct ViveFile {
	ViveInode* inode;
	__le64  i_no;
	std::string file_name;
	__le64 parent_inode_no;
	int noatime; //1: if O_NOATIME specified on open. 
	int nomtime; //1: if O_NOMTIME specified on open
	int dirty;
	ViveFile();
	~ViveFile();

};

struct ViveFsContext {
	ViveFsContext();
	~ViveFsContext();
	std::string db_path;
	ROCKSDB_NAMESPACE::TransactionDB* db;
	ROCKSDB_NAMESPACE::ColumnFamilyHandle* default_cf;
	ROCKSDB_NAMESPACE::ColumnFamilyHandle* meta_cf;
	ROCKSDB_NAMESPACE::ColumnFamilyHandle* data_cf;

	ROCKSDB_NAMESPACE::WriteOptions meta_opt;
	ROCKSDB_NAMESPACE::WriteOptions data_opt;
	ROCKSDB_NAMESPACE::ReadOptions read_opt;

	struct ViveInode root_inode;
	std::unique_ptr<ViveFile> root_file;

	int64_t inode_seed;
	int lazy_time; //1: update time lazy, i.e. option 'lazytime' is specified on mount
	               //1 by default in ViveNAS
	int relatime;  //1 if 'relatime' is specified on mount
	int64_t generate_inode_no();
};
struct vn_extent_key {
	union {
		struct {
			__le64 extent_index;
			__le64 inode_no;
		};
		char keybuf[16];
	};
	const char* to_string() const;
};

#define PFS_FULL_EXTENT_BMP  (uint16_t)0xffff
struct vn_extent_head {
	int8_t flags;
	int8_t pad0;
	union {
		uint16_t data_bmp; //extent当中有效的数据部分。bmp为0的部分在extent数据中并没有被存储
		uint16_t merge_off;  //一次写入操作在extent内部的offset
	};
	char pad1[12];
}; //total 16 Byte
static_assert(sizeof(struct vn_extent_head)==16, "sizeof struct vn_extent_head not expected 16");
#define PFS_EXTENT_HEAD_SIZE sizeof(struct vn_extent_head)

/**
 * special inode no, as defined in ext:
 * 0	No such inode, numberings starts at 1
 * 1	Defective block list
 * 2	Root directory
 * 3	User quotas
 * 4	Group quotas
 * 5	Boot loader
 * 6	Undelete directory
 * 7	Reserved group descriptors (for resizing filesystem)
 * 8	Journal
 * 9	Exclude inode (for snapshots)
 * 10	Replica inode
 * 11	First non-reserved inode (often lost + found)
 */ 
#define VN_ROOT_INO 2
#define VN_FIRST_USER_INO 12
#define CHECKED_CALL(exp) do{ \
	Status s = exp;         \
	if (!s.ok()) {          \
	S5LOG_FATAL("Failed: `%s` status:%s", #exp, s.ToString().c_str()); \
	}                       \
}while(0)
static __always_inline int64_t deserialize_int64(const char* s) {
	return *(int64_t*)s;
}
void deserialize_superblock(const char* buf, ViveSuperBlock& sb);
std::string serialize_superblock(const ViveSuperBlock& sb);
void setup_db_options(rocksdb::Options& options);
void setup_data_cf_options(rocksdb::ColumnFamilyOptions& options);
void setup_meta_cf_options(rocksdb::ColumnFamilyOptions& options);

#endif // vivenas_internal_h__