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

ska::pst::stat::FileProcessor::FileProcessor(
        const std::string& data_file_path,
        const std::string& weights_file_path)
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::ctor");

  segment_producer = std::make_unique<ska::pst::common::FileSegmentProducer>(data_file_path, weights_file_path);
  auto data_config = segment_producer->get_data_header();
  auto weights_config = segment_producer->get_weights_header();

  // create stat/ output folder
  std::filesystem::path stat_output_path("stat");
  std::filesystem::create_directory(stat_output_path);

  // create stat output filename using the stem of the data filename
  std::filesystem::path data_output_filename(data_file_path);

  std::filesystem::path stat_output_filename(data_output_filename.stem());
  stat_output_filename.replace_extension("h5");
  stat_output_filename = stat_output_path / stat_output_filename;

  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::ctor stat output filename={}", stat_output_filename.generic_string());

  data_config.set("STAT_OUTPUT_FILENAME",stat_output_filename.generic_string());

  processor = std::make_shared<StatProcessor>(data_config, weights_config);
}

ska::pst::stat::FileProcessor::~FileProcessor()
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::~FileProcessor");
}

void ska::pst::stat::FileProcessor::process()
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::process");

  auto segment = segment_producer->next_segment();
  processor->process (segment);
}

