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

#ifndef VBUCKET_H_
#define VBUCKET_H_

#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "glog/logging.h"
#include "hermes.h"
#include "traits.h"

namespace hermes {

namespace api {

class VBucket {
 private:
  std::string name_;
  VBucketID id_;
  std::list<std::pair<std::string, std::string>> linked_blobs_;
  std::list<Trait *> attached_traits_;
  Blob local_blob;
  bool persist;
  /** internal Hermes object owned by vbucket */
  std::shared_ptr<Hermes> hermes_;

 public:
  VBucket(std::string initial_name, std::shared_ptr<Hermes> const &h,
          bool persist, Context ctx)
      : name_(initial_name),
        id_({0, 0}),
        linked_blobs_(),
        attached_traits_(),
        local_blob(),
        persist(persist),
        hermes_(h) {
    LOG(INFO) << "Create VBucket " << initial_name << std::endl;
    (void)ctx;
    if (IsVBucketNameTooLong(name_)) {
      id_.as_int = 0;
      throw std::length_error("VBucket name exceeds maximum size of " +
                              std::to_string(kMaxVBucketNameSize));
    } else {
      id_ = GetOrCreateVBucketId(&hermes_->context_, &hermes_->rpc_, name_);
      if (!IsValid()) {
        throw std::runtime_error("Could not open or create VBucket");
      }
    }
  }

  ~VBucket() {
    name_.clear();
    linked_blobs_.clear();
  }

  bool IsValid() const;

  /** get the name of vbucket */
  std::string GetName() const { return this->name_; }

  /** link a blob to this vbucket */
  Status Link(std::string blob_name, std::string bucket_name, Context &ctx);

  /** unlink a blob from this vbucket */
  Status Unlink(std::string blob_name, std::string bucket_name, Context &ctx);

  /** check if blob is in this vbucket */
  bool Contain_blob(std::string blob_name, std::string bucket_name);

  /** get a blob linked to this vbucket */
  Blob &GetBlob(std::string blob_name, std::string bucket_name);

  /** retrieves the subset of links satisfying pred */
  /** could return iterator */
  template <class Predicate>
  std::vector<std::string> GetLinks(Predicate pred, Context &ctx);

  /** attach a trait to this vbucket */
  Status Attach(Trait *trait, Context &ctx);

  /** detach a trait to this vbucket */
  Status Detach(Trait *trait, Context &ctx);

  /** retrieves the subset of attached traits satisfying pred */
  template <class Predicate>
  std::vector<TraitID> GetTraits(Predicate pred, Context &ctx);

  /** delete a vBucket */
  /** decrements the links counts of blobs in buckets */
  Status Delete(Context &ctx);
};  // class VBucket

}  // namespace api
}  // namespace hermes

#endif  //  VBUCKET_H_
