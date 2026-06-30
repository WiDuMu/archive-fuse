// SPDX-License-Identifier: MPL-2.0

#ifndef ZIPFS_H
#define ZIPFS_H
#include <optional>
#pragma once

#include <zip.h>

#include <cstring>
#include <set>

#include <fs.hpp>

/// Zip FUSE filesystem
class ZipFS : public FileSystem {
   private:
	zip_t* z;
	zip_int64_t nentries;
	std::set<std::string> dirs;
	std::optional<std::string> password;
	// std::unordered_map<std::string, struct stat> entries;

   public:
    /// Load archive_path to get it ready to mount
    ZipFS(const char* archive_path);
	ZipFS(const std::string& archive_path);
	~ZipFS();

	int getattr(const std::string& path, struct stat* stbuf);

	int readdir(const std::string& path, void* buf, fuse_fill_dir_t filler, off_t offset,
	            struct fuse_file_info* fi);

	int open(const std::string& path, struct fuse_file_info* fi);

	int release(const std::string& path, struct fuse_file_info* fi);

	int read(const std::string& path, char* buf, size_t size, off_t offset,
	         struct fuse_file_info* fi);
};

#endif   // ZIP_FS_H
