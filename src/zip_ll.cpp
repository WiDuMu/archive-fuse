#include "zip_ll.hpp"

#include <fuse_lowlevel.h>
#include <sys/stat.h>

#include <vector>

ZipFSLL::ZipFSLL(const char *path) : z(nullptr, zip_close) {
	int err;

	zip_t *za = zip_open(path, ZIP_RDONLY, &err);

	if (!za) {
		zip_error_t error;
		zip_error_init_with_code(&error, err);

		std::runtime_error except(zip_error_strerror(&error));
		zip_error_fini(&error);

		throw except;
	}

	z.reset(za);
}

struct Entry {
	const char *name;
	fuse_ino_t ino;
	mode_t mode;
};

void ZipFSLL::readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                      struct fuse_file_info *fi) {
	if (ino != FUSE_ROOT_ID) {
		fuse_reply_err(req, ENOTDIR);
		return;
	}

	std::vector<Entry> entries = {{".", FUSE_ROOT_ID, S_IFDIR | 0755},
	                              {"..", FUSE_ROOT_ID, S_IFDIR | 0755},
	                              {"hello", 2, S_IFREG | 0444}};

	std::vector<char> buf(size);
	size_t pos = 0;

	for (size_t i = off; i < entries.size(); ++i) {
		struct stat st = {};
		st.st_ino = entries[i].ino;
		st.st_mode = entries[i].mode;

		// fuse_add_direntry returns the size needed for this entry.
		// The last parameter is the *next* offset to read from (i + 1).
		size_t len =
		    fuse_add_direntry(req, buf.data() + pos, size - pos, entries[i].name, &st, i + 1);

		// If the buffer is full, stop adding entries.
		if (len > size - pos) {
			break;
		}
		pos += len;
	}

	fuse_reply_buf(req, buf.data(), pos);
}
