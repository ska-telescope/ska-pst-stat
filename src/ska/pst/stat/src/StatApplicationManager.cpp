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

#include <spdlog/spdlog.h>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/StatApplicationManager.h"

ska::pst::stat::StatApplicationManager::StatApplicationManager() :
  ska::pst::common::ApplicationManager("stat")
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::StatApplicationManager");
  initialise();
}

ska::pst::stat::StatApplicationManager::~StatApplicationManager()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::~StatApplicationManager quit()");
  quit();
}

void ska::pst::stat::StatApplicationManager::configure_from_file(const std::string &config_file)
{
  SPDLOG_DEBUG("ska::pst::stat::DiskManager::configure_from_file config_file={}",config_file);
  ska::pst::common::AsciiHeader config;
  config.load_from_file(config_file);
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::configure_from_file config={}",config.raw());

  // configure beam
  configure_beam(config);

  // configure scan
  configure_scan(config);

  // configure from file
  start_scan(config);
}

void ska::pst::stat::StatApplicationManager::validate_configure_beam(const ska::pst::common::AsciiHeader& config, ska::pst::common::ValidationContext *context)
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam config={}", config.raw());
  // Iterate through the vector and validate existince of required data header keys
  for (const std::string& config_key : data_config_keys) {
      if (config.has(config_key))
      {
        SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam data {}={}", config_key, config.get_val(config_key));
        // data_beam_config.set_val(config_key, config.get_val(config_key));
        data_beam_config.clone(config);
      } else {
        context->add_missing_field_error(config_key);
      }
  }
  // Iterate through the vector and validate existince of required weights header keys
  for (const std::string& config_key : weights_config_keys) {
      if (config.has(config_key))
      {
        SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam weights{}={}", config_key, config.get_val(config_key));
        // weights_beam_config.set_val(config_key, config.get_val(config_key));
        weights_beam_config.clone(config);
      } else {
        context->add_missing_field_error(config_key);
      }
  }
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam complete");
}

void ska::pst::stat::StatApplicationManager::validate_configure_scan(const ska::pst::common::AsciiHeader& config, ska::pst::common::ValidationContext *context)
{
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::validate_configure_scan");
  // Iterate through the vector and validate existince of desired data smrb keys
  for (const std::string& config_key : data_smrb_keys) {
      if (data_beam_config.has(config_key))
      {
        SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_scan data {}={}", config_key, data_beam_config.get_val(config_key));
      } else {
        SPDLOG_WARN("ska::pst::stat::StatApplicationManager::validate_configure_scan data key missing: {}", config_key);
      }
  }
  // Iterate through the vector and validate existince of desired weights smrb keys
  for (const std::string& config_key : weights_smrb_keys) {
      if (weights_beam_config.has(config_key))
      {
        SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_scan data {}={}", config_key, weights_beam_config.get_val(config_key));
      } else {
        SPDLOG_WARN("ska::pst::stat::StatApplicationManager::validate_configure_scan weights key missing: {}", config_key);
      }
  }
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_scan complete");

}

void ska::pst::stat::StatApplicationManager::validate_start_scan(const ska::pst::common::AsciiHeader& config)
{
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::validate_configure_start_scan placeholder");
}

void ska::pst::stat::StatApplicationManager::perform_initialise()
{
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::perform_initialise placeholder");
}

void ska::pst::stat::StatApplicationManager::perform_configure_beam()
{
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::perform_configure_beam");

  producer = std::make_unique<ska::pst::smrb::SmrbSegmentProducer>(data_beam_config.get_val("DATA_KEY"), weights_beam_config.get_val("WEIGHTS_KEY"));
  // Connect to SMRB
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam producer->connect({})", timeout);
  producer->connect(timeout);

  // Copy the rest of the headers
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam  producer->get_data_header\n:{}", producer->get_data_header().raw());
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam producer->get_weights_header\n:{}", producer->get_weights_header().raw());
  data_beam_config.clone(producer->get_data_header());
  weights_beam_config.clone(producer->get_weights_header());
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam data_beam_config:\n{}", data_beam_config.raw());
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam weights_beam_config:\n{}", weights_beam_config.raw());

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam complete");
}

void ska::pst::stat::StatApplicationManager::perform_configure_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan");
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan data_beam_config:\n{}", data_beam_config.raw());
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan weights_beam_config:\n{}", weights_beam_config.raw());
  processor = std::make_unique<StatProcessor>(data_beam_config, weights_beam_config);
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan complete");
}

void ska::pst::stat::StatApplicationManager::perform_start_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_start_scan");
  producer->open();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_start_scan complete");
}

void ska::pst::stat::StatApplicationManager::perform_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan");
  bool eod = false;
  while (!eod)
  {
    SPDLOG_DEBUG("ska::pst::stat::StreamWriter::perform_scan producer->next_segment()");
    auto segment = producer->next_segment();
    SPDLOG_DEBUG("ska::pst::stat::StreamWriter::perform_scan opened segment containing {} bytes", segment.data.size);

    if (segment.data.block == nullptr)
    {
      SPDLOG_DEBUG("ska::pst::stat::StreamWriter::perform_scan encountered end of data");
      eod = true;
    }
    else
    {
      SPDLOG_DEBUG("ska::pst::stat::StreamWriter::perform_scan processor->process");
      processor->process(segment);
      SPDLOG_DEBUG("ska::pst::stat::StreamWriter::perform_scan processor->process done");
    }
  }
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan complete");
}

void ska::pst::stat::StatApplicationManager::perform_stop_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_stop_scan placeholder");
}

void ska::pst::stat::StatApplicationManager::perform_deconfigure_scan()
{
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::perform_deconfigure_scan");
  producer->close();
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::perform_deconfigure_scan done");
}

void ska::pst::stat::StatApplicationManager::perform_deconfigure_beam()
{
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam");
  producer->disconnect();

  // beam_config cleanup
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam data_beam_config.reset()");
  data_beam_config.reset();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam weights_beam_config.reset()");
  weights_beam_config.reset();
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam done");
}

void ska::pst::stat::StatApplicationManager::perform_terminate()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_terminate");
}
