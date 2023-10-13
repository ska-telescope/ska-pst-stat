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

#include <chrono>
#include <filesystem>
#include <spdlog/spdlog.h>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/StatApplicationManager.h"

ska::pst::stat::StatApplicationManager::StatApplicationManager(const std::string& base_path) :
  ska::pst::common::ApplicationManager("stat"), stat_base_path(base_path)
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::StatApplicationManager stat_base_path={}", stat_base_path);
  initialise();
}

ska::pst::stat::StatApplicationManager::~StatApplicationManager()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::~StatApplicationManager quit()");
  quit();
}

void ska::pst::stat::StatApplicationManager::configure_from_file(const std::string &config_file)
{
  SPDLOG_DEBUG("ska::pst::stat::DiskManager::configure_from_file config_file={}", config_file);
  ska::pst::common::AsciiHeader config;
  config.load_from_file(config_file);
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::configure_from_file config={}", config.raw());

  // configure beam
  configure_beam(config);

  // configure scan
  configure_scan(config);

  // configure from file
  start_scan(config);
}

void ska::pst::stat::StatApplicationManager::validate_configure_beam(const ska::pst::common::AsciiHeader& config, ska::pst::common::ValidationContext *context)
{
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::validate_configure_beam config={}", config.raw());
  // Iterate through the vector and validate existince of required data header keys
  for (const std::string& config_key : beam_config_keys)
  {
    if (config.has(config_key))
    {
      SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam {}={}", config_key, config.get_val(config_key));
    } else {
      context->add_missing_field_error(config_key);
    }
  }
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::validate_configure_beam complete");
}

void ska::pst::stat::StatApplicationManager::validate_configure_scan(const ska::pst::common::AsciiHeader& config, ska::pst::common::ValidationContext* context)
{
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::validate_configure_scan config={}", config.raw());
  // Iterate through the vector and validate existince of required data header keys
  for (const std::string& config_key : scan_config_keys)
  {
    SPDLOG_INFO("ska::pst::stat::StatApplicationManager::validate_configure_scan assessing config_key={}", config_key);
    if (config.has(config_key))
    {
      SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_scan {}={}", config_key, config.get_val(config_key));
    } else {
      context->add_missing_field_error(config_key);
    }
  }
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::validate_configure_scan complete");
}

void ska::pst::stat::StatApplicationManager::validate_start_scan(const ska::pst::common::AsciiHeader& config)
{
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::validate_configure_start_scan config={}", config.raw());
}

void ska::pst::stat::StatApplicationManager::perform_initialise()
{
  // ensure the output directory exists on initialisation
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_initialise creating {}", stat_base_path);
  std::filesystem::path output_dir(stat_base_path);
  create_directories(output_dir);
}

void ska::pst::stat::StatApplicationManager::perform_configure_beam()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam");

  // ska::pst::common::ApplicationManager has written beam configuration parameters to beam_config
  std::string data_key = beam_config.get_val("DATA_KEY");
  std::string weights_key = beam_config.get_val("WEIGHTS_KEY");
  if (beam_config.has("STAT_PROC_DELAY"))
  {
    processing_delay = beam_config.get_uint32("STAT_PROC_DELAY");
    SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam setting processing_delay={} ms", processing_delay);
  }

  // Construct the SMRB segment producer
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam SmrbSegmentProducer({}, {})", data_key, weights_key);
  producer = std::make_unique<ska::pst::smrb::SmrbSegmentProducer>(data_key, weights_key);

  // Connect to the SMRB segment producer
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam producer->connect({})", timeout);
  producer->connect(timeout);

  // Acquire the data and weights beam configuration from the SMRB segment producer
  data_beam_config.clone(producer->get_data_header());
  weights_beam_config.clone(producer->get_weights_header());
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::perform_configure_beam data_beam_config:\n{}", data_beam_config.raw());
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::perform_configure_beam weights_beam_config:\n{}", weights_beam_config.raw());


  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam complete");
}

void ska::pst::stat::StatApplicationManager::perform_configure_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan");
  {
    std::unique_lock<std::mutex> lock(processing_mutex);
    keep_processing = true;
    processing_cond.notify_one();
  }

  // set the base_path and suffix for statistics recording in the beam configuration
  data_beam_config.set_val("STAT_BASE_PATH", stat_base_path);

  // set configuration parameters for the StatProcessor
  data_beam_config.set_val("STAT_REQ_TIME_BINS", scan_config.get_val("STAT_REQ_TIME_BINS"));
  data_beam_config.set_val("STAT_REQ_FREQ_BINS", scan_config.get_val("STAT_REQ_FREQ_BINS"));
  data_beam_config.set_val("STAT_NREBIN", scan_config.get_val("STAT_NREBIN"));

  // ensure the STAT_OUTPUT_FILENAME is not present in the beam_config
  data_beam_config.del("STAT_OUTPUT_FILENAME");

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan data_beam_config:\n{}", data_beam_config.raw());
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan weights_beam_config:\n{}", weights_beam_config.raw());
  processor = std::make_unique<ska::pst::stat::StatProcessor>(data_beam_config, weights_beam_config);

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::ctor add shared ScalarStatPublisher publisher to processor");
  scalar_publisher = std::make_shared<ska::pst::stat::ScalarStatPublisher>(data_beam_config);
  processor->add_publisher(scalar_publisher);

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::ctor add shared StatHdf5FileWriter publisher to processor");
  hdf5_publisher = std::make_shared<StatHdf5FileWriter>(data_beam_config);
  processor->add_publisher(hdf5_publisher);

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
  while (!eod && keep_processing)
  {
    SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan producer->next_segment()");
    auto segment = producer->next_segment();
    SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan opened segment containing {} bytes", segment.data.size);

    if (segment.data.block == nullptr)
    {
      SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan encountered end of data");
      eod = true;
    }
    else
    {
      SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan processor->process");
      bool processing_complete = processor->process(segment);
      SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan processor->process processing_complete={}", processing_complete);
    }

    // use processing_cond to wait on changes to the keep_processing boolean
    SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan waiting for {} ms on processing_cond", processing_delay);
    {
      using namespace std::chrono_literals;
      std::chrono::milliseconds timeout = processing_delay * 1ms;
      std::unique_lock<std::mutex> lock(processing_mutex);
      processing_cond.wait_for(lock, timeout, [&]{return (keep_processing == false);});
      lock.unlock();
      processing_cond.notify_one();
    }
    SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan keep_processing={}", keep_processing);
  }
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan complete");
}

void ska::pst::stat::StatApplicationManager::perform_stop_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_stop_scan");

  // interrupt any of the statistics computation in the the StatProcessor.
  processor->interrupt();

  // signal the thread running perform_scan via the processing_cond
  {
    std::unique_lock<std::mutex> lock(processing_mutex);
    SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_stop_scan keep_processing = false");
    keep_processing = false;
    processing_cond.notify_one();
  }
}

void ska::pst::stat::StatApplicationManager::perform_deconfigure_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_scan");
  producer->close();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_scan done");
}

void ska::pst::stat::StatApplicationManager::perform_deconfigure_beam()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam");
  producer->disconnect();

  // beam_config cleanup
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam data_beam_config.reset()");
  data_beam_config.reset();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam weights_beam_config.reset()");
  weights_beam_config.reset();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam done");
}

void ska::pst::stat::StatApplicationManager::perform_terminate()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_terminate");
}

auto ska::pst::stat::StatApplicationManager::get_scalar_stats() -> ska::pst::stat::StatStorage::scalar_stats_t
{
  return scalar_publisher->get_scalar_stats();
}
