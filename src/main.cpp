#include <logging.hpp>
#include <print>
#include <string>
#include <tempfile.hpp>

#include "zip_fs.hpp"

int main(int argc, char** argv) {
	TempDir temp = TempDir::tempdir_here();

	if (argc < 2 || (std::string)argv[1] == "-v") {
		std::println("Please specify a archive.");
		return 1;
	}

	for (int i = 2; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-v") {
			logging_level = VERBOSE;
		}
	}

	ZipFS zfs(argv[1]);

	zfs.run(temp);

	return 0;
}
