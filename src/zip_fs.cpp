#include <zip.h>
#include <zipconf.h>

#include <cstdio>
#include <stdexcept>
#include <zip_fs.hpp>

#include "logging.hpp"

ZipFS::ZipFS(const std::string& archive_path) : FileSystem(), z(nullptr) {
	int err;
	z = zip_open(archive_path.c_str(), ZIP_RDONLY, &err);

	if (!z) {
		zip_error_t error;
		zip_error_init_with_code(&error, err);

		std::runtime_error except(zip_error_strerror(&error));
		zip_error_fini(&error);

		throw except;
	}

	nentries = zip_get_num_entries(z, 0);

	if (nentries == -1) {
		throw std::runtime_error("Unable to get the number of entries for archive");
	}
}

ZipFS::~ZipFS() { zip_close(z); }

int ZipFS::getattr(const std::string& path, struct stat* stbuf) {
	zip_stat_t sb{};

	log(VERBOSE, "Stating entry {}", path);

	if (path == "/") {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (!zip_stat(z, path.c_str() + 1, ZIP_FL_ENC_GUESS, &sb)) {
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

int ZipFS::readdir(const std::string& path, void* buf, fuse_fill_dir_t filler, off_t offset,
                   struct fuse_file_info* fi) {
	log(VERBOSE, "Reading directory {}", path);

	if (!path.ends_with('/')) {
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);

	for (long i = 0; i < nentries; i++) {
		// This is probably less efficient than doing it manually, too bad.
		std::string file_name = zip_get_name(z, i, ZIP_FL_ENC_GUESS);
		log(VERBOSE, "Adding file {}", file_name);

		if (file_name.find('/') == std::string::npos) {
			filler(buf, file_name.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
		}
	}

	return 0;
}

int ZipFS::open(const std::string& path, struct fuse_file_info* fi) {
	log(VERBOSE, "Opening entry {}", path);
	return 0;
}

int ZipFS::release(const std::string& path, struct fuse_file_info* fi) {
	log(VERBOSE, "Closing entry {}", path);
	return 0;
}

const zip_int64_t PAGE_SIZE = 4096;

static inline int zseek(zip_file_t* file, off_t offset) {
	char dontcare[PAGE_SIZE];  // For some compressed files, we have to read to a point.
	zip_int64_t curr = zip_ftell(file);

	if (curr == -1) {
		throw std::runtime_error("Failed to seek in file: could not get file offset");
	}

	if (curr == offset) {
		return 0;
	}

	bool seekable = zip_file_is_seekable(file);

	if (seekable) {
		return zip_fseek(file, offset, SEEK_SET);
	}

	while (curr != -1 && (curr + PAGE_SIZE) < offset) {
		if (zip_fread(file, dontcare, PAGE_SIZE) != PAGE_SIZE) {
			log(ERROR, "Failed read seek from offset {} to offset {}", curr, offset);
			return EOF;
		}

		curr = zip_ftell(file);
	}

	if (curr == -1) {
		log(ERROR, "Failed read seek, ftell failed");
		return EOF;
	}

	zip_int64_t read_size = offset - curr;

	if (zip_fread(file, dontcare, read_size) != read_size) {
		log(ERROR, "Failed final read seek from offset {} to {}", curr, offset);
		return EOF;
	}

	return 0;
}

int ZipFS::read(const std::string& path, char* buf, size_t size, off_t offset,
                struct fuse_file_info* fi) {
	zip_file_t* file = nullptr;
	log(VERBOSE, "Reading {} bytes from offset {} from {}", size, offset, path);

	file = zip_fopen(z, path.c_str() + 1, ZIP_FL_ENC_GUESS);

	if (file) {
		if (zseek(file, offset)) {
			return EOF;
		}

		zip_int64_t nread = zip_fread(file, buf, size);

		zip_fclose(file);
		return nread;
	}
	return -ENOENT;
}
