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

#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "ska/pst/stat/FileProcessor.h"

ska::pst::stat::FileProcessor::FileProcessor(
        const ska::pst::common::AsciiHeader& _config,
        const std::string& data_file_path,
        const std::string& weights_file_path)
        : processor(new StatProcessor(_config)),
	  config(_config),
	  block_loader(new ska::pst::common::DataWeightsFileBlockLoader(data_file_path, weights_file_path))
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::FileProcessor");
}

ska::pst::stat::FileProcessor::~FileProcessor()
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::~FileProcessor");
}

void ska::pst::stat::FileProcessor::process()
{
  SPDLOG_DEBUG("ska::pst::stat::FileProcessor::process");

  auto block = block_loader->next_block();
  processor->process (block.data.block, block.data.size, block.weights.block, block.weights.size);
}


// stubs to fake compilation until at3-516-develop-statprocessor is merged

#include "ska/pst/stat/StatPublisher.h"

ska::pst::stat::StatProcessor::StatProcessor(ska::pst::common::AsciiHeader const&) {}
ska::pst::stat::StatProcessor::~StatProcessor() {}
void ska::pst::stat::StatProcessor::process(char*, unsigned long, char*, unsigned long) {}
