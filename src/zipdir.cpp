#include <string>
#include <print>

#include <zip.h>

int main(int argc, char** argv)
{
    char* ar;
    char* search_dir = nullptr;
    int err;

    if (argc < 2) {
        return 1;
    }

    ar = argv[1];

    if (argc > 2) {
        search_dir = argv[2];
        std::println("Search directory: {}", search_dir);
    }

	zip_t* z = zip_open(ar, ZIP_RDONLY, &err);

	long entries = zip_get_num_entries(z, 0);

	for (long i = 0; i < entries; i++) {
		// This is probably less efficient than doing it manually, too bad.
		std::string file_name = zip_get_name(z, i, ZIP_FL_ENC_GUESS);

		if (!search_dir || (file_name.starts_with(search_dir) && !(file_name.ends_with('/') && file_name == search_dir))) {
		    std::println("file: {}", file_name);
		}
	}
}
