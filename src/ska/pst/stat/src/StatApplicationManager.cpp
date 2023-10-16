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
#include <stdexcept>
#include <spdlog/spdlog.h>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/StatApplicationManager.h"

ska::pst::stat::StatApplicationManager::StatApplicationManager(const std::string& base_path) :
  ska::pst::common::ApplicationManager("stat"), stat_base_path(base_path)
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::StatApplicationManager stat_base_path={}", stat_base_path);
  initialise();
  processing_state = Idle;
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
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::validate_start_scan config={}", config.raw());
  // Iterate through the vector and validate existince of required keys
  for (const std::string& config_key : startscan_config_keys)
  {
    if (config.has(config_key))
    {
      SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_start_scan {}={}", config_key, config.get_val(config_key));
    } else {
      throw std::runtime_error("required field " + config_key + " missing in start scan configuration");
    }
  }
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::validate_start_scan complete");
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
  data_key = beam_config.get_val("DATA_KEY");
  weights_key = beam_config.get_val("WEIGHTS_KEY");

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam complete");
}

void ska::pst::stat::StatApplicationManager::perform_configure_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan");

  // Construct the SMRB segment producer
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam SmrbSegmentProducer({}, {})", data_key, weights_key);
  producer = std::make_unique<ska::pst::smrb::SmrbSegmentProducer>(data_key, weights_key);

  processing_delay = scan_config.get_uint32("STAT_PROC_DELAY_MS");
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan setting processing_delay={} ms", processing_delay);

  // set configuration parameters for the StatProcessor
  req_time_bins = scan_config.get_uint32("STAT_REQ_TIME_BINS");
  req_freq_bins = scan_config.get_uint32("STAT_REQ_FREQ_BINS");
  num_rebin = scan_config.get_uint32("STAT_NREBIN");

  // Connect to the SMRB segment producer, this triggers the read_config command on the SMRBs
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan producer->connect({})", timeout);
  producer->connect(timeout);

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan complete");
}

void ska::pst::stat::StatApplicationManager::perform_start_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_start_scan");

  // calling open on the producer will read the complete header from the SMRBs
  producer->open();

  // Acquire the data and weights beam configuration from the SMRB segment producer
  data_header.clone(producer->get_data_header());
  weights_header.clone(producer->get_weights_header());
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::perform_start_scan data_header:\n{}", data_header.raw());
  SPDLOG_TRACE("ska::pst::stat::StatApplicationManager::perform_start_scan weights_header:\n{}", weights_header.raw());

  // check that the SCAN_ID and EB_ID in the header's match the scan configuration
  if (data_header.get_val("SCAN_ID") != startscan_config.get_val("SCAN_ID"))
  {
    throw std::runtime_error("SCAN_ID mismatch between data header and startscan_config");
  }
  if (data_header.get_val("EB_ID") != scan_config.get_val("EB_ID"))
  {
    throw std::runtime_error("EB_ID mismatch between data header and startscan_config");
  }

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_start_scan SCAN_ID={} EB_ID={}", data_header.get_val("SCAN_ID"), data_header.get_val("EB_ID"));

  // set the base_path and suffix for statistics recording in the beam configuration
  data_header.set_val("STAT_BASE_PATH", stat_base_path);

  data_header.set("STAT_REQ_TIME_BINS", req_time_bins);
  data_header.set("STAT_REQ_FREQ_BINS", req_freq_bins);
  data_header.set("STAT_NREBIN", num_rebin);

  // ensure the STAT_OUTPUT_FILENAME is not present in the beam_config
  data_header.del("STAT_OUTPUT_FILENAME");

  processor = std::make_unique<ska::pst::stat::StatProcessor>(data_header, weights_header);

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::ctor add shared ScalarStatPublisher publisher to processor");
  scalar_publisher = std::make_shared<ska::pst::stat::ScalarStatPublisher>(data_header);
  processor->add_publisher(scalar_publisher);

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::ctor add shared StatHdf5FileWriter publisher to processor");
  hdf5_publisher = std::make_shared<StatHdf5FileWriter>(data_header);
  processor->add_publisher(hdf5_publisher);

  // ensure processing will execute
  {
    std::unique_lock<std::mutex> lock(processing_mutex);
    keep_processing = true;
    processing_cond.notify_one();
  }

  processing_state = Processing;
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
      processing_state = Processing;
      SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan processor->process");
      bool processing_complete = processor->process(segment);
      SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan processor->process processing_complete={}", processing_complete);
    }

    processing_state = Waiting;

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

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan closing producer connection");
  producer->close();

  // beam_config cleanup
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan data_beam_config.reset()");
  data_header.reset();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan weights_beam_config.reset()");
  weights_header.reset();
  processing_state = Idle;

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
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_scan producer->disconnect()");
  producer->disconnect();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_scan done");
}

void ska::pst::stat::StatApplicationManager::perform_deconfigure_beam()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam");

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
