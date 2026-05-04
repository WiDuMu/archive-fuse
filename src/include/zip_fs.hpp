#ifndef ZIPFS_H
#define ZIPFS_H
#include <zipconf.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#include "logging.hpp"
#pragma once

#include <zip.h>

#include <fs.hpp>

class ZipArchive {
   public:
    zip_t* z = NULL;
    ZipArchive(const std::string& path) {
	int err;
	z = zip_open(path.c_str(), ZIP_RDONLY, &err);

	if (z == NULL) {
	    zip_error_t error;
	    zip_error_init_with_code(&error, err);

	    std::runtime_error except(zip_error_strerror(&error));
	    zip_error_fini(&error);

	    throw except;
	}
    }

    ~ZipArchive() { zip_close(z); }
};

class ZipFS : public FileSystem {
   private:
    ZipArchive arch;

   public:
    ZipFS(const std::string& archive_path) : arch(archive_path), FileSystem() {}

    int getattr(const std::string& path, struct stat* stbuf) {
	zip_stat_t sb;

	log(VERBOSE, "Stating entry {}", path);

	memset(stbuf, 0, sizeof(*stbuf));

	if (path == "/") {
	    stbuf->st_mode = S_IFDIR | 0755;
	    stbuf->st_nlink = 2;
	} else if (!zip_stat(arch.z, path.c_str() + 1, ZIP_FL_ENC_GUESS, &sb)) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = (size_t) sb.size;
        stbuf->st_uid = 0;
    } else {
	    return -ENOENT;
	}

	return 0;
    }

    int readdir(const std::string& path, void* buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info* fi) {
    log(VERBOSE, "Reading directory {}", path);

	if (path != "/") {
	    return -ENOENT;
	}

	filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);

	long entries = zip_get_num_entries(arch.z, 0);

	for (long i = 0; i < entries; i++) {
	    // This is probably less efficient than doing it manually, too bad.
	    std::string file_name = zip_get_name(arch.z, i, ZIP_FL_ENC_GUESS);

	    if (file_name.find('/')) {
		filler(buf, file_name.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
	    }
	}

	return 0;
    }

    int open(const std::string& path, struct fuse_file_info* fi) { return 0; }

    int read(const std::string& path, char* buf, size_t size, off_t offset,
             struct fuse_file_info* fi) {
	zip_file_t* file = nullptr;

	log(VERBOSE, "Reading offset {} from {}", offset, path);

	file = zip_fopen(arch.z, path.c_str(), ZIP_FL_ENC_GUESS);

	if (!file) {
	    return -ENOENT;
	}

	if (offset != 0 && !zip_file_is_seekable(file) || zip_fseek(file, offset, SEEK_SET)) {
        return EOF;
    }

	// if (offset != 0) {
	//     if (zip_file_is_seekable(file)) {
	// 	    if (zip_fseek(file, offset, SEEK_SET)) {
	// 	        log(ERROR, "Unable to seek file {}", path);
	// 	        return -ESPIPE;
	// 	    }
	//     } else {
 //    		// Bad seek simulation: performance: don't ask
 //    		char dont_care_buf[4096];
 //    		zip_int64_t pos = zip_ftell(file);
 //    		while (pos != -1 && (pos + 4096) < offset) {
 //    		    if (zip_fread(file, dont_care_buf, 4096) != 4096) {
 //    			return -ESPIPE;
 //    		    }
 //    		    pos = zip_ftell(file);
 //    		}

 //    		zip_int64_t final_read_size = offset - pos;

 //    		if (zip_fread(file, dont_care_buf, final_read_size) !=
 //    		    final_read_size) {
 //    		    return -ESPIPE;
 //    		}
	//     }
	// }

	zip_int64_t nread = zip_fread(file, buf, size);

	zip_fclose(file);
	return nread;
    }
};

#endif
