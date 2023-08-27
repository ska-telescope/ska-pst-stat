/*
 * Copyright 2023 Square Kilometre Array Observatory
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ska/pst/stat/FileProcessor.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>

ska::pst::stat::FileProcessor::FileProcessor()
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::ctor empty");
}

ska::pst::stat::FileProcessor::FileProcessor(
        const std::string& data_filename,
        const std::string& weights_filename)
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::ctor data_filename={} weights_filename={}", data_filename, weights_filename);

  segment_producer = std::make_unique<ska::pst::common::FileSegmentProducer>(data_filename, weights_filename);
  auto data_config = segment_producer->get_data_header();
  auto weights_config = segment_producer->get_weights_header();

  // test that the data and weights files start at the same heap offset
  assert_equal_heap_offsets(data_config, weights_config);

  data_config.set("STAT_OUTPUT_FILENAME", get_output_filename(data_filename));

  set_defaults(data_config);

  processor = std::make_shared<StatProcessor>(data_config, weights_config);
}

ska::pst::stat::FileProcessor::~FileProcessor()
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::~FileProcessor");
}

static void set_default(ska::pst::common::AsciiHeader& config, const char* key, unsigned default_value)
{
  if (!config.has(key))
  {
    SPDLOG_WARN("ska::pst::stat::FileProcessor::set_default {} not specified in data header set to default value of {}", key, default_value);
    config.set(key, default_value);
  }
}

void ska::pst::stat::FileProcessor::set_defaults(ska::pst::common::AsciiHeader& config)
{
  set_default (config, "STAT_NREBIN", 256); // NOLINT
  set_default (config, "STAT_REQ_TIME_BINS", 4); // NOLINT
  set_default (config, "STAT_REQ_FREQ_BINS", 4); // NOLINT
}

void ska::pst::stat::FileProcessor::process()
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::process");

  auto segment = segment_producer->next_segment();
  processor->process (segment);
}

auto ska::pst::stat::FileProcessor::get_output_filename(const std::string& data_filename) const -> std::string
{
  std::filesystem::path data_file_path = data_filename;

  // create stat output filename using the stem of the data filename
  std::filesystem::path stat_output_filename = data_file_path.filename().replace_extension("h5");

  if (data_file_path.has_parent_path())
  {
    // create stat/ output folder using the parent_path of the data subfolder
    std::filesystem::path data_folder = data_file_path.parent_path();
    std::filesystem::path parent_folder = data_folder.parent_path();
    SPDLOG_DEBUG("ska::pst::stat::FileProcessor::ctor parent_folder={}", parent_folder.generic_string());

    std::filesystem::path stat_output_folder("stat");
    stat_output_folder = parent_folder / stat_output_folder;

    std::filesystem::create_directory(stat_output_folder);
    stat_output_filename = stat_output_folder / stat_output_filename;
  }

  std::string result = stat_output_filename.generic_string();
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::get_output_filename result={}", result);

  return result;
}

auto get_heap_offset (const std::string& name, const ska::pst::common::AsciiHeader& config, uint64_t heap_stride) -> uint64_t
{
  auto byte_offset = config.get_uint64("OBS_OFFSET");

  if (byte_offset % heap_stride != 0)
  {
    SPDLOG_ERROR("ska::pst::stat::FileProcessor::assert_equal_heap_offsets {} OBS_OFFSET={} is not a multiple of heap stride", name, byte_offset, heap_stride);
    throw std::runtime_error("ska::pst::stat::FileProcessor::assert_equal_heap_offsets "+name+" OBS_OFFSET is not a multiple of heap stride");
  }

  return byte_offset / heap_stride;
}

void ska::pst::stat::FileProcessor::assert_equal_heap_offsets(const ska::pst::common::AsciiHeader& data_config, const ska::pst::common::AsciiHeader& weights_config) const
{
  ska::pst::common::HeapLayout layout;
  layout.configure(data_config, weights_config);

  // ensure that the data and weights blocks start on the same integer heap
  auto data_heap_offset = get_heap_offset("data", data_config, layout.get_data_heap_stride());
  auto weights_heap_offset = get_heap_offset("weights", weights_config, layout.get_weights_heap_stride());

  if (data_heap_offset != weights_heap_offset)
  {
    SPDLOG_ERROR("ska::pst::stat::FileProcessor::assert_equal_heap_offsets data_heap_offset={} does not equal weights_heap_offset={}", data_heap_offset, weights_heap_offset);
    throw std::runtime_error("ska::pst::stat::FileProcessor::assert_equal_heap_offsets data_heap_offset does not equal weights_heap_offset");
  }
}
