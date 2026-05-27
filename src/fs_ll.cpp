#include "fs_ll.hpp"

#include <fuse_lowlevel.h>

#include <cerrno>

// Default implements
void FileSystem_LowLevel::init(void *userdata, struct fuse_conn_info *conn) {}
void FileSystem_LowLevel::destroy(void *userdata) {}
void FileSystem_LowLevel::lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
	fuse_reply_err(req, ENOSYS);
}
void FileSystem_LowLevel::getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
	fuse_reply_err(req, ENOSYS);
}
void FileSystem_LowLevel::readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                                  struct fuse_file_info *fi) {
	fuse_reply_err(req, ENOSYS);
}
void FileSystem_LowLevel::open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
	fuse_reply_err(req, ENOSYS);
}
void FileSystem_LowLevel::read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                               struct fuse_file_info *fi) {
	fuse_reply_err(req, ENOSYS);
}

// Retrieve the filesystem instance from the request passed.
FileSystem_LowLevel *FileSystem_LowLevel::get_instance(fuse_req_t req) {
	return static_cast<FileSystem_LowLevel *>(fuse_req_userdata(req));
}

// Functions that find an intance, then call the relevant function

void FileSystem_LowLevel::ll_init(void *userdata, struct fuse_conn_info *info) {
	static_cast<FileSystem_LowLevel *>(userdata)->init(userdata, info);
}

void FileSystem_LowLevel::ll_destroy(void *userdata) {
	static_cast<FileSystem_LowLevel *>(userdata)->destroy(userdata);
}

void FileSystem_LowLevel::ll_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
	get_instance(req)->lookup(req, parent, name);
}

void FileSystem_LowLevel::ll_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                                     struct fuse_file_info *fi) {
	get_instance(req)->readdir(req, ino, size, off, fi);
}

void FileSystem_LowLevel::ll_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
	get_instance(req)->open(req, ino, fi);
}

void FileSystem_LowLevel::ll_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                                  struct fuse_file_info *fi) {
	get_instance(req)->read(req, ino, size, off, fi);
}

int FileSystem_LowLevel::mount(const char *mount_point) {
	struct fuse_lowlevel_ops ops = {};
	ops.init = ll_init;
	ops.destroy = ll_destroy;
	ops.lookup = ll_lookup;
	ops.getattr = ll_getattr;
	ops.open = ll_open;
	ops.read = ll_read;
	ops.readdir = ll_readdir;

	struct fuse_args fargs = FUSE_ARGS_INIT(0, nullptr);
	// struct fuse_loop_config config;
	struct fuse_session *ses;
	int err;

	// We don't care about the exe name at all.
	if (fuse_opt_add_arg(&fargs, "fake_app")) goto arg_error;

	if (!background) {
		if (fuse_opt_add_arg(&fargs, "-f")) goto arg_error;
	}

	ses = fuse_session_new(&fargs, &ops, sizeof(ops), (void *)this);

	if (!ses) goto session_error;

	if (fuse_set_signal_handlers(ses) != 0) goto signal_error;

	if (fuse_session_mount(ses, mount_point)) goto mount_error;

	if (fuse_daemonize(!background)) goto mount_error;

	if (multithreaded) {
		err = fuse_session_loop(ses);
	} else {
		// config.clone_fd = 0;
		// err = fuse_session_loop(ses, &config);
	}

	fuse_session_unmount(ses);
	fuse_remove_signal_handlers(ses);
	fuse_session_destroy(ses);
	fuse_opt_free_args(&fargs);
	return err;
mount_error:
	fuse_remove_signal_handlers(ses);
signal_error:
	fuse_session_destroy(ses);
session_error:
	fuse_opt_free_args(&fargs);
	return ENOTCONN;
arg_error:
	fuse_opt_free_args(&fargs);
	return ENOMEM;
}
