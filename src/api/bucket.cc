/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "bucket.h"

#include <iostream>
#include <vector>

#include "utils.h"
#include "buffer_pool.h"
#include "metadata_management.h"

namespace hermes {

namespace api {

Bucket::Bucket(const std::string &initial_name,
               const std::shared_ptr<Hermes> &h, Context ctx)
    : name_(initial_name), hermes_(h) {
  (void)ctx;

  if (IsBucketNameTooLong(name_)) {
    id_.as_int = 0;
    throw std::length_error("Bucket name is too long: " +
                            std::to_string(kMaxBucketNameSize));
  } else {
    id_ = GetOrCreateBucketId(&hermes_->context_, &hermes_->rpc_, name_);
    if (!IsValid()) {
      throw std::runtime_error("Bucket id is invalid.");
    }
  }
}

bool Bucket::IsValid() const {
  return !IsNullBucketId(id_);
}

Status Bucket::Put(const std::string &name, const u8 *data, size_t size,
                   Context &ctx) {
  Status result;

  if (size > 0 && nullptr == data) {
    result = INVALID_BLOB;
    LOG(ERROR) << result.Msg();
  }

  if (result.Succeeded()) {
    std::vector<std::string> names{name};
    // TODO(chogan): Create a PreallocatedMemory allocator for std::vector so
    // that a single-blob-Put doesn't perform a copy
    std::vector<Blob> blobs{Blob{data, data + size}};
    result = Put(names, blobs, ctx);
  }

  return result;
}

size_t Bucket::GetBlobSize(Arena *arena, const std::string &name,
                           Context &ctx) {
  (void)ctx;
  size_t result = 0;

  if (IsValid()) {
    LOG(INFO) << "Getting Blob " << name << " size from bucket "
              << name_ << '\n';
    BlobID blob_id = GetBlobId(&hermes_->context_, &hermes_->rpc_, name,
                                     id_);
    if (!IsNullBlobId(blob_id)) {
      result = GetBlobSizeById(&hermes_->context_, &hermes_->rpc_, arena,
                               blob_id);
    }
  }

  return result;
}

size_t Bucket::Get(const std::string &name, Blob &user_blob, Context &ctx) {
  (void)ctx;

  size_t ret = 0;

  if (IsValid()) {
    // TODO(chogan): Assumes scratch is big enough to hold buffer_ids
    ScopedTemporaryMemory scratch(&hermes_->trans_arena_);

    if (user_blob.size() == 0) {
      ret = GetBlobSize(scratch, name, ctx);
    } else {
      LOG(INFO) << "Getting Blob " << name << " from bucket " << name_ << '\n';
      BlobID blob_id = GetBlobId(&hermes_->context_, &hermes_->rpc_,
                                       name, id_);
      ret = ReadBlobById(&hermes_->context_, &hermes_->rpc_,
                         &hermes_->trans_arena_, user_blob, blob_id);
    }
  }

  return ret;
}

template<class Predicate>
Status Bucket::GetV(void *user_blob, Predicate pred, Context &ctx) {
  (void)user_blob;
  (void)ctx;
  Status ret;

  LOG(INFO) << "Getting blobs by predicate from bucket " << name_ << '\n';

  return ret;
}

Status Bucket::DeleteBlob(const std::string &name, Context &ctx) {
  (void)ctx;
  Status ret;

  LOG(INFO) << "Deleting Blob " << name << " from bucket " << name_ << '\n';
  DestroyBlobByName(&hermes_->context_, &hermes_->rpc_, id_, name);

  return ret;
}

Status Bucket::RenameBlob(const std::string &old_name,
                          const std::string &new_name,
                          Context &ctx) {
  (void)ctx;
  Status ret;

  if (IsBlobNameTooLong(new_name)) {
    // TODO(chogan): @errorhandling
    ret = BLOB_NAME_TOO_LONG;
    LOG(ERROR) << ret.Msg();
    return ret;
  } else {
    LOG(INFO) << "Renaming Blob " << old_name << " to " << new_name << '\n';
    hermes::RenameBlob(&hermes_->context_, &hermes_->rpc_, old_name,
                       new_name, id_);
  }

  return ret;
}

bool Bucket::ContainsBlob(const std::string &name) {
  bool result = hermes::ContainsBlob(&hermes_->context_, &hermes_->rpc_, id_,
                                     name);

  return result;
}

bool Bucket::BlobIsInSwap(const std::string &name) {
  BlobID blob_id = GetBlobId(&hermes_->context_, &hermes_->rpc_, name,
                                   id_);
  bool result = hermes::BlobIsInSwap(blob_id);

  return result;
}

template<class Predicate>
std::vector<std::string> Bucket::GetBlobNames(Predicate pred,
                                              Context &ctx) {
  (void)ctx;

  LOG(INFO) << "Getting blob names by predicate from bucket " << name_ << '\n';

  return std::vector<std::string>();
}

struct bkt_info * Bucket::GetInfo(Context &ctx) {
  (void)ctx;
  struct bkt_info *ret = nullptr;

  LOG(INFO) << "Getting bucket information from bucket " << name_ << '\n';

  return ret;
}

Status Bucket::Rename(const std::string &new_name, Context &ctx) {
  (void)ctx;
  Status ret;

  if (IsBucketNameTooLong(new_name)) {
    // TODO(chogan): @errorhandling
    ret = BUCKET_NAME_TOO_LONG;
    LOG(ERROR) << ret.Msg();
    return ret;
  } else {
    LOG(INFO) << "Renaming a bucket to" << new_name << '\n';
    RenameBucket(&hermes_->context_, &hermes_->rpc_, id_, name_, new_name);
  }

  return ret;
}

Status Bucket::Persist(const std::string &file_name, Context &ctx) {
  (void)ctx;
  // TODO(chogan): Once we have Traits, we need to let users control the mode
  // when we're, for example, updating an existing file. For now we just assume
  // we're always creating a new file.
  std::string open_mode = "w";

  // TODO(chogan): Support other storage backends
  Status result = StdIoPersistBucket(&hermes_->context_, &hermes_->rpc_,
                                     &hermes_->trans_arena_, id_, file_name,
                                     open_mode);

  return result;
}

Status Bucket::Close(Context &ctx) {
  (void)ctx;
  Status ret;

  if (IsValid()) {
    LOG(INFO) << "Closing bucket '" << name_ << "'" << std::endl;
    DecrementRefcount(&hermes_->context_, &hermes_->rpc_, id_);
    id_.as_int = 0;
  }

  return ret;
}

Status Bucket::Destroy(Context &ctx) {
  (void)ctx;
  Status result;

  if (IsValid()) {
    LOG(INFO) << "Destroying bucket '" << name_ << "'" << std::endl;
    bool destroyed = DestroyBucket(&hermes_->context_, &hermes_->rpc_,
                                   name_.c_str(), id_);
    if (destroyed) {
      id_.as_int = 0;
    } else {
      // TODO(chogan): @errorhandling
      result = BUCKET_IN_USE;
      LOG(ERROR) << result.Msg();
    }
  }

  return result;
}

}  // namespace api
}  // namespace hermes
