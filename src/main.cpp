#include <logging.hpp>
#include <print>
#include <string>
#include <tempfile.hpp>

#include "zip_fs.hpp"

#include "clipp.h"

int main(int argc, char** argv) {
    std::string archive_path;
    auto cli = (
        clipp::value("archive", archive_path),
        option("-v", "--verbose").set()
    )

	TempDir temp = TempDir::tempdir_here();


	ZipFS zfs(archive_path);

	zfs.run(temp);

	return 0;
}
