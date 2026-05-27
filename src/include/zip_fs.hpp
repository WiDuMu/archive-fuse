#pragma once
#ifndef ZIPFS_H
#define ZIPFS_H

#include <zip.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fs.hpp>
#include <unordered_map>

#include "logging.hpp"

const int default_perms = 0777;

class ZipArchive {
   public:
	zip_t* z;
	ZipArchive(const std::string& path);

	~ZipArchive();
};

class ZipFS : public FileSystem {
   private:
	ZipArchive arch;
	std::vector<std::string> paths;

   public:
	ZipFS(const std::string& archive_path) : FileSystem(), arch(archive_path) {
		long entries = zip_get_num_entries(arch.z, 0);

		if (entries == -1) {
			// This shouldn't happen in practice
			throw std::runtime_error(
			    "Null archive passed to ZipFS constructor, this should be impossible, good job.");
		}

		for (long i = 0; i < entries; i++) {
			// This is probably less efficient than doing it manually, too bad.
			const char* file_name = zip_get_name(arch.z, i, ZIP_FL_ENC_GUESS);

			if (!file_name) continue;

			log(VERBOSE, "Adding file {} to indexes", file_name);

			paths.push_back(file_name);
		}
	}

	int getattr(const std::string& path, struct stat* stbuf) {
		zip_stat_t sb;

		log(VERBOSE, "Stating entry {}", path);

		memset(stbuf, 0, sizeof(*stbuf));

		if (path == "/") {
			stbuf->st_mode = S_IFDIR | default_perms;
			stbuf->st_nlink = 2;
		} else if (path.ends_with('/')) {
			stbuf->st_mode = S_IFDIR | default_perms;
			stbuf->st_nlink = 2;
		} else if (!zip_stat(arch.z, path.c_str() + 1, ZIP_FL_ENC_GUESS, &sb)) {
			stbuf->st_mode = S_IFREG | default_perms;
			stbuf->st_nlink = 1;
			stbuf->st_size = (size_t)sb.size;
			stbuf->st_uid = 0000;
		} else {
			if (zip_directory_exists(arch.z, path.c_str() + 1)) {
				stbuf->st_mode = S_IFDIR | default_perms;
				stbuf->st_nlink = 2;
				return 0;
			}
			log(VERBOSE, "Entry {} not found", path);
			return -ENOENT;
		}

		return 0;
	}

	static bool zip_directory_exists(zip_t* archive, const std::string dir_path) {
		std::string dir = dir_path;
		if (!archive || dir_path.empty()) return false;

		if (dir_path.back() != '/') dir += '/';

		if (zip_name_locate(archive, dir.c_str(), 0) != -1) return true;

		zip_int64_t entries = zip_get_num_entries(archive, 0);

		for (zip_int64_t i = 0; i < entries; i++) {
			const char* file_name = zip_get_name(archive, i, 0);
			if (file_name == nullptr) continue;

			if (strncmp(file_name, dir_path.c_str(), dir_path.length()) == 0) return true;
		}
		return false;
	}

	int readdir(const std::string& path, void* buf, fuse_fill_dir_t filler, off_t offset,
	            struct fuse_file_info* fi) {
		bool any_added = false;
		std::unordered_map<std::string, bool> dirs_added;
		std::string dir = path.substr(1);
		if (path != "/") {
			dir += '/';
		}

		log(VERBOSE, "Reading dir {}", dir);

		long entries = zip_get_num_entries(arch.z, 0);

		for (long i = 0; i < entries; i++) {
			// This is probably less efficient than doing it manually, too bad.
			std::string file_name = zip_get_name(arch.z, i, ZIP_FL_ENC_GUESS);

			if (file_name.starts_with(dir)) {
				std::string postfix = file_name.substr(dir.length());
				if (postfix.contains('/')) {
					std::string new_dir_name = postfix.substr(0, postfix.find('/'));
					log(VERBOSE, "File {} at postfix {} belongs in a subdirectory {}", file_name,
					    postfix, new_dir_name);

					if (!dirs_added.contains(new_dir_name)) {
						log(VERBOSE, "Adding subdirectory {}", new_dir_name);

						struct stat st;
						memset(&st, 0, sizeof(st));
						st.st_nlink = 2;
						st.st_mode = S_IFDIR | default_perms;

						dirs_added[new_dir_name] = true;
						filler(buf, new_dir_name.c_str(), &st, 0, FUSE_FILL_DIR_PLUS);
						any_added = true;
					}
				} else {
					log(VERBOSE, "Adding file {} at postfix {} to directory {}", file_name, postfix,
					    dir);
					if (!postfix.empty())
					    filler(buf, postfix.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
					any_added = true;
				}
			}
		}

		if (any_added) {
			filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
			filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);
			return 0;
		}

		return -ENOENT;
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
