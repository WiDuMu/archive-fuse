#include <zip.h>

#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <string>
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

	for (zip_int64_t i = 0; i < nentries; i++) {
		const char* fname = zip_get_name(z, i, ZIP_FL_ENC_GUESS);
		if (fname) {
			std::string potential_dir_name = fname;
			size_t separator_loc = potential_dir_name.find('/');

			while (separator_loc != std::string::npos) {
				potential_dir_name = potential_dir_name.substr(0, separator_loc);
				log(VERBOSE, "Adding dir {} to dirs", potential_dir_name);
				dirs.insert(potential_dir_name);
				separator_loc = potential_dir_name.find('/');
			}
		}
	}
}

ZipFS::~ZipFS() { zip_close(z); }

int ZipFS::getattr(const std::string& path, struct stat* stbuf) {
	zip_stat_t sb{};

	log(VERBOSE, "Stating entry {}", path);

	if (path == "/") {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (dirs.contains(path.c_str() + 1)) {
		stbuf->st_mode = S_IFDIR | 0444;
		stbuf->st_nlink = 3;
	} else if (!zip_stat(z, path.c_str() + 1, ZIP_FL_ENC_GUESS, &sb)) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = (size_t)sb.size;
		stbuf->st_uid = 0000;
	} else {
		return -ENOENT;
	}

	return 0;
}

// static bool zip_directory_exists(zip_t* archive, const std::string dir_path) {
// 	std::string dir = dir_path;
// 	if (!archive || dir_path.empty()) return false;

// 	if (dir_path.back() != '/') dir += '/';

// 	if (zip_name_locate(archive, dir.c_str(), 0) != -1) return true;

// 	zip_int64_t entries = zip_get_num_entries(archive, 0);

// 	for (zip_int64_t i = 0; i < entries; i++) {
// 		const char* file_name = zip_get_name(archive, i, 0);
// 		if (file_name == nullptr) continue;

// 		if (strncmp(file_name, dir_path.c_str(), dir_path.length()) == 0) return true;
// 	}
// 	return false;
// }

const int default_perms = 0444;

int ZipFS::readdir(const std::string& path, void* buf, fuse_fill_dir_t filler, off_t offset,
                   struct fuse_file_info* fi) {
	bool any_added = false;
	std::set<std::string_view> dirs_added;
	std::string dir = path.substr(1);
	if (path != "/" && !path.ends_with('/')) {
		dir += '/';
	}

	log(VERBOSE, "Reading dir {}", dir);

	for (long i = 0; i < nentries; i++) {
		// This is probably less efficient than doing it manually, too bad.
		std::string_view file_name = zip_get_name(z, i, ZIP_FL_ENC_GUESS);

		if (file_name.starts_with(dir)) {
			std::string_view postfix = file_name.substr(dir.length());
			if (postfix.contains('/')) {
				std::string_view new_dir_name = postfix.substr(0, postfix.find('/'));

				if (!dirs_added.contains(new_dir_name)) {
					struct stat st{};
					st.st_nlink = 2;
					st.st_mode = S_IFDIR | default_perms;

					dirs_added.insert(new_dir_name);
					std::string f(new_dir_name);
					filler(buf, f.c_str(), &st, 0, FUSE_FILL_DIR_PLUS);
					any_added = true;
				}
			} else {
				if (!postfix.empty()) {
					std::string f(postfix);
					filler(buf, f.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
				}
				any_added = true;
			}
		}
	}

	if (any_added || path == "/") {
		struct stat st{};
		st.st_nlink = 3;
		st.st_nlink = S_IFDIR | default_perms;
		filler(buf, ".", &st, 0, FUSE_FILL_DIR_PLUS);
		filler(buf, "..", &st, 0, FUSE_FILL_DIR_PLUS);
		return 0;
	}

	return -ENOENT;
}

int ZipFS::open(const std::string& path, struct fuse_file_info* fi) {
	log(VERBOSE, "Opening entry {}", path);
	zip_file_t* file = zip_fopen(z, path.c_str() + 1, ZIP_FL_ENC_GUESS);
	if (file) {
		fi->fh = reinterpret_cast<size_t>(file);
		return 0;
	}
	return ENOENT;
}

int ZipFS::release(const std::string& path, struct fuse_file_info* fi) {
	log(VERBOSE, "Closing entry {}", path);
	if (fi->fh) {
		zip_fclose(reinterpret_cast<zip_file_t*>(fi->fh));
	}
	return 0;
}

const zip_int64_t PAGE_SIZE = 4096;

static inline int zseek(zip_file_t* file, off_t offset) {
	char dontcare[PAGE_SIZE];  // For some compressed files, we have to read to a point.
	bool seekable = zip_file_is_seekable(file);

	if (seekable) {
		return zip_fseek(file, offset, SEEK_SET);
	}

	zip_int64_t curr = zip_ftell(file);

	if (curr == offset) {
		return 0;
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
	bool fopened = false;
	log(VERBOSE, "Reading {} bytes from offset {} from {}", size, offset, path);

	if (fi->fh) {
		file = reinterpret_cast<zip_file_t*>(fi->fh);
	} else {
		file = zip_fopen(z, path.c_str() + 1, ZIP_FL_ENC_GUESS);
		fopened = true;
	}

	if (!file) {
		return -ENOENT;
	}

	if (zseek(file, offset)) {
		return EOF;
	}

	zip_int64_t nread = zip_fread(file, buf, size);
	if (fopened) {
		zip_fclose(file);
	}
	return nread;
}
