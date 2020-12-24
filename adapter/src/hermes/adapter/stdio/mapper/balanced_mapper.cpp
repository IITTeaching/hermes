//
// Created by manihariharan on 12/23/20.
//

#include "balanced_mapper.h"

#include <hermes/adapter/stdio/common/constants.h>

using hermes::adapter::stdio::BalancedMapper;
using hermes::adapter::stdio::FileStruct;
using hermes::adapter::stdio::HermesStruct;

MapperReturnType BalancedMapper::map(const FileStruct& file_op) {
  auto mapper_return = MapperReturnType();
  size_t size_mapped = 0;
  std::hash<FileID> file_hash_t;
  size_t file_hash = file_hash_t(file_op.file_id_);
  while (file_op.size_ > size_mapped) {
    FileStruct file;
    HermesStruct hermes;
    file.offset_ = file_op.offset_ + size_mapped;
    size_t page_index = file.offset_ / PAGE_SIZE;
    hermes.offset_ = file.offset_ % PAGE_SIZE;
    hermes.size_ = (page_index + 1) * PAGE_SIZE - hermes.offset_;

    file.size_ = hermes.size_;
    hermes.blob_name_ =
        std::to_string(file_hash) + "_" + std::to_string(page_index);
    mapper_return.emplace_back(file, hermes);
    size_mapped += hermes.size_;
  }
  return mapper_return;
}
