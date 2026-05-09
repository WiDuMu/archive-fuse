#ifndef FS_FUSE_H
#define FS_FUSE_H
#pragma once

#define FUSE_USE_VERSION 31  // Use FUSE 3 API
#include <fuse.h>

#include <cstring>
#include <logging.hpp>
#include <string>
#include <vector>

/// Wrapper interface for a FUSE Filesystem
class FileSystem {
   private:
	bool background = false;

   public:
	FileSystem() {};
	virtual ~FileSystem() = default;

	// Filesystem
	virtual int getattr(const std::string& path, struct stat* stbuf) { return -ENOENT; }
	virtual int readdir(const std::string& path, void* buf, fuse_fill_dir_t filler, off_t offset,
	                    struct fuse_file_info* fi) {
		return -ENOENT;
	}
	virtual int open(const std::string& path, struct fuse_file_info* fi) { return 0; }
	virtual int read(const std::string& path, char* buf, size_t size, off_t offset,
	                 struct fuse_file_info* fi) {
		return 0;
	}

   private:
	// Static trampoline functions to bridge C and C++
	static int wrap_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi) {
		return get_instance()->getattr(path, stbuf);
	}

	static int wrap_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset,
	                        struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
		return get_instance()->readdir(path, buf, filler, offset, fi);
	}

	static int wrap_open(const char* path, struct fuse_file_info* fi) {
		return get_instance()->open(path, fi);
	}

	static int wrap_read(const char* path, char* buf, size_t size, off_t offset,
	                     struct fuse_file_info* fi) {
		return get_instance()->read(path, buf, size, offset, fi);
	}

	// Helper to retrieve the class instance from FUSE context
	static FileSystem* get_instance() {
		return static_cast<FileSystem*>(fuse_get_context()->private_data);
	}

   public:
	// Main loop to mount the filesystem
	int run(const char* mount_point) {
		log(VERBOSE, "Mounting fs on mount point {}", mount_point);

		struct fuse_operations ops = {};
		ops.getattr = wrap_getattr;
		ops.readdir = wrap_readdir;
		ops.open = wrap_open;
		ops.read = wrap_read;

		// Libfuse relies on a set of arguments to configure itself
		// However, we want to be able to use our own CLI, so these are faked statically.
		// const_cast is used
		std::vector<char*> fake_args = {
		    const_cast<char*>("fakeprogramname"),
		    const_cast<char*>(mount_point),
		    const_cast<char*>("-f"),
		    const_cast<char*>("-s"),
		};

		// if (!background) {
		//     fake_args.push_back(const_cast<char*>("-f"));
		// }

		return fuse_main(fake_args.size(), fake_args.data(), &ops, this);
	}

	int run(const std::string& mount_point) {
	    return run(mount_point.c_str());
	}
};

#endif  // FS_FUSE_H
