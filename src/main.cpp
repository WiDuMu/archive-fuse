#include <logging.hpp>
#include <iostream>
#include <string>
#include <tempfile.hpp>

#include "zip_fs.hpp"

#include "clipp.h"

int main(int argc, char** argv) {
    std::string archive_path;
    bool verbose;
    bool quiet;

    auto cli = (
        clipp::value("archive", archive_path).doc("archive to mount"),
        clipp::option("-v", "--verbose").set(verbose).doc("verbose output"),
        clipp::option("-q", "--quiet").set(quiet).doc("quiet output")
    );

    if (!clipp::parse(argc, argv, cli)) {
        std::cout << clipp::make_man_page(cli);
        return 1;
    }

    if (verbose) {
        logging_level = VERBOSE;
    } else if (quiet) {
        logging_level = ERROR;
    }

	TempDir temp = TempDir::tempdir_here();


	ZipFS zfs(archive_path);

	zfs.run(temp);

	return 0;
}
