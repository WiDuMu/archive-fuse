#ifndef ZIPFS_H
#define ZIPFS_H
#include <zipconf.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "logging.hpp"
#pragma once

#include <zip.h>

#include <fs.hpp>

class ZipArchive {
   public:
	zip_t* z;
	ZipArchive(const std::string& path);

	~ZipArchive();
};

class ZipFS : public FileSystem {
   private:
	ZipArchive arch;

   public:
	ZipFS(const std::string& archive_path) : FileSystem(), arch(archive_path) {}

	int getattr(const std::string& path, struct stat* stbuf) {
		zip_stat_t sb;

		log(VERBOSE, "Stating entry {}", path);

		memset(stbuf, 0, sizeof(*stbuf));

		if (path == "/") {
			stbuf->st_mode = S_IFDIR | 0755;
			stbuf->st_nlink = 2;
		} else if (!zip_stat(arch.z, path.c_str() + 1, ZIP_FL_ENC_GUESS, &sb)) {
            if (path.ends_with('/')) {
                stbuf->st_mode = S_IFDIR | 0444;
                stbuf->st_nlink = 3;
                return 0;
            }
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
			stbuf->st_size = (size_t)sb.size;
			stbuf->st_uid = 0000;
		} else {
			return -ENOENT;
		}

		return 0;
	}

	int readdir(const std::string& path, void* buf, fuse_fill_dir_t filler, off_t offset,
	            struct fuse_file_info* fi) {
		log(VERBOSE, "Reading directory {}", path);

		if (!path.ends_with('/')) {
			return -ENOENT;
		}

		filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
		filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);

		long entries = zip_get_num_entries(arch.z, 0);

		for (long i = 0; i < entries; i++) {
			// This is probably less efficient than doing it manually, too bad.
			std::string file_name = zip_get_name(arch.z, i, ZIP_FL_ENC_GUESS);
			log(VERBOSE, "Adding file {}", file_name);

			if (file_name.find('/') == std::string::npos) {
				filler(buf, file_name.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
			}
		}

		return 0;
	}

	int open(const std::string& path, struct fuse_file_info* fi) { return 0; }

	int read(const std::string& path, char* buf, size_t size, off_t offset,
	         struct fuse_file_info* fi) {
		zip_file_t* file = NULL;
		log(VERBOSE, "Reading {} bytes from offset {} from {}", size, offset, path);

		file = zip_fopen(arch.z, path.c_str() + 1, ZIP_FL_ENC_GUESS);

		if (file) {
			if (offset != 0 && zip_file_is_seekable(file) && zip_fseek(file, offset, SEEK_SET)) {
				log(ERROR, "Failed to seek to offset {} in file {}", offset, path);
				return EOF;
			} else if (offset != 0 && !zip_file_is_seekable(file)) {
				log(VERBOSE, "Attempting a read seek");
				char dontcare[4096];
				zip_int64_t pos = zip_ftell(file);

				while (pos != -1 && (pos + 4096) < offset) {
					if (zip_fread(file, dontcare, 4096) != 4096) {
						log(ERROR, "Failed read seek to offset {} in file {}", offset, path);
						return EOF;
					}

					pos = zip_ftell(file);
					log(VERBOSE, "Read seek at pos {}", pos);
				}

				zip_int64_t final_read = offset - pos;

				if (zip_fread(file, dontcare, final_read) != final_read) {
					log(ERROR, "Failed final read seek to offset {} in file {}", offset, path);
					return EOF;
				}
			}

			zip_int64_t nread = zip_fread(file, buf, size);

			zip_fclose(file);
			return nread;
		}
		return -ENOENT;
	}
};

#endif
