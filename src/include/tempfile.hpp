#ifndef TEMPFILE_H
#define TEMPFILE_H
#pragma once

#include <cerrno>
#include <filesystem>
#include <string>
#include <system_error>

/// A temporary directory that is cleaned up when it goes out of scope.
class TempDir {
   private:
	bool cleanup = true;
	std::string loc;

   public:
	TempDir() : loc(std::filesystem::temp_directory_path() / ".tmp-XXXXXX") {
		if (mkdtemp(loc.data()) == nullptr) {
			throw std::system_error(errno, std::generic_category(),
			                        "Failed to open temporary directory");
		}
	}

	TempDir(bool local) : loc(std::filesystem::current_path() / ".tmp-XXXXXX") {
		if (mkdtemp(loc.data()) == nullptr) {
			throw std::system_error(errno, std::generic_category(),
			                        "Failed to open temporary directory");
		}
	}

	TempDir(std::string& parent) : loc(parent + "/.tmp-XXXXXX") {
		if (mkdtemp(loc.data()) == nullptr) {
			throw std::system_error(errno, std::generic_category(),
			                        "Failed to open temporary directory");
		}
	}

	TempDir(std::string& parent, std::string& templ) : loc(parent + "/" + templ) {
		if (mkdtemp(loc.data()) == nullptr) {
			throw std::system_error(errno, std::generic_category(),
			                        "Failed to open temporary directory");
		}
	}

	// Because TempDir deletes it's contents, it currently doesn't make sense to allow it to be
	// cloned.
	TempDir(TempDir& other) = delete;

	TempDir(TempDir&& other) = delete;

	static TempDir tempdir_here(std::string& templ) {
		std::string current_dir = std::filesystem::current_path();
		return TempDir(current_dir, templ);
	}

	static TempDir tempdir_here() {
		std::string current_dir = std::filesystem::current_path();
		return TempDir(current_dir);
	}

	const std::string& get_loc() { return loc; }

	operator const std::string&() const { return loc; }

	bool close() {
		if (cleanup) {
			return std::filesystem::remove_all(loc) > 0;
		}
		return 0;
	}

	~TempDir() { close(); }
};

template <>
struct std::formatter<TempDir> {
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	auto format(const TempDir& dir, std::format_context& ctx) const {
		return std::format_to(ctx.out(), "{}", (std::string)dir);
	}
};

#endif  // TEMPFILE_H
