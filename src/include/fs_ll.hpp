#pragma once
#ifndef FS_LL_H_
#define FS_LL_H_

#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 12)

#include <fuse_lowlevel.h>

class FileSystem_LowLevel {
   private:
    // Whether you are multithreaded
    bool multithreaded = false;
    // Whether to go into the background
	bool background = false;

	static void ll_init(void *userdata, struct fuse_conn_info* info);
	static void ll_destroy(void *userdata);
	static void ll_lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
	static void ll_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
	static void ll_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
	                       struct fuse_file_info *fi);
	static void ll_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
	static void ll_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
	                   struct fuse_file_info *fi);
	static FileSystem_LowLevel *get_instance(fuse_req_t req);
   protected:

	FileSystem_LowLevel();
	virtual ~FileSystem_LowLevel() = default;
	virtual void init(void *userdata, struct fuse_conn_info *conn);
	virtual void destroy(void *userdata);
	virtual void getattr(fuse_req_t req, fuse_ino_t parent, struct fuse_file_info *fi);
	virtual void lookup(fuse_req_t req, fuse_ino_t parent, const char *name);
	virtual void readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
	                     struct fuse_file_info *fi);
	virtual void open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
	virtual void read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
	                  struct fuse_file_info *fi);

   public:
	int mount(const char *mount_point);
};

#endif  // FS_LL_H_
