#pragma once
#ifndef ZIPFS_LL_H
#define ZIPFS_LL_H
#include <memory>

#include "fs_ll.hpp"
#include <zip.h>

class ZipFSLL : public FileSystem_LowLevel {
  private:
    std::unique_ptr<zip_t, decltype(&zip_close)> z;
  protected:
      void init(void *userdata, struct fuse_conn_info *conn) override;
      void destroy(void *userdata) override;
      void getattr(fuse_req_t req, fuse_ino_t parent, struct fuse_file_info *fi) override;
      void lookup(fuse_req_t req, fuse_ino_t parent, const char *name) override;
      void readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) override;
      void open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) override;
      void read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) override;
  public:
      ZipFSLL(const char *path);
      ~ZipFSLL() = default;
};


#endif // ZIPFS_LL_H
