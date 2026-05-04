#include <logging.hpp>
#include <print>
#include <tempfile.hpp>

#include "zip_fs.hpp"

int main(int argc, char** argv) {
    TempDir temp;

    logging_level = VERBOSE;

    if (argc < 2) {
	std::println("Please specify a archive.");
	return 1;
    }

    ZipFS zfs(argv[1]);

    log(INFO, "Hello there, my temporary directory is {}!", temp)

    std::string mp = temp;

    zfs.run(mp);

    return 0;
}
