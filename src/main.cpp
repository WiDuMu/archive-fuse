// SPDX-License-Identifier: MPL-2.0

#include <cstdlib>
#include <iostream>
#include <logging.hpp>
#include <memory>
#include <string>
#include <tempfile.hpp>

#include "clipp.h"
#include "zip_fs.hpp"

int main(int argc, char** argv) {
	std::unique_ptr<TempDir> temp(nullptr);
	std::string command = "xdg-open";
	std::string archive;
	std::string mount_point;
	bool verbose = false;
	bool open = false;

	auto cli = (clipp::value("archive", archive).doc("Archive to open as a filesystem"),
	            clipp::option("-v", "--verbose").set(verbose).doc("Verbose log_levelging"),
	            clipp::option("-o", "--open").set(open).doc("Open folder"),
				(clipp::option("-c", "--command").set(open) & clipp::value("command").set(command)).doc("Command to run, if different to xdg-open"),
	            clipp::opt_value("mount_point")
	                .set(mount_point)
	                .doc("Optional mount point, otherwise a tempdir is used."));

	if (!parse(argc, argv, cli)) {
		std::cout << clipp::make_man_page(cli, argv[0]);
		return 1;
	}

	if (verbose) {
		logging_level = VERBOSE;
	}

	if (mount_point.empty()) {
		temp = std::make_unique<TempDir>(true);
		mount_point = temp.get()->get_loc();
	}

	log_level(INFO, "Mounting on {}", mount_point);

	ZipFS zfs(archive);

	if (open) {
		command = std::format("{} {} &", command, mount_point);

		log_level(INFO, "Running command '{}'", command);

		std::system(command.c_str());
	}

	zfs.run(mount_point);

	return 0;
}
