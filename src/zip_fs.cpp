#include <zip_fs.hpp>
#include <stdexcept>

ZipArchive::ZipArchive(const std::string& path) {
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

ZipArchive::~ZipArchive() { zip_close(z); }
