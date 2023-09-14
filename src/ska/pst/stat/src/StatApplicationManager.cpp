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
  SPDLOG_DEBUG("ska::pst::dsp::DiskManager::configure_from_file config_file={}",config_file);
  ska::pst::common::AsciiHeader config;
  config.load_from_file(config_file);
  SPDLOG_INFO("ska::pst::stat::StatApplicationManager::configure_from_file config={}",config.raw());

  // configure beam
  ska::pst::common::AsciiHeader beam_config;
  // TODO: beam_config.set_val($KEY, config.get_val($KEY));
  configure_beam(beam_config);

  // configure scan
  ska::pst::common::AsciiHeader scan_config;
  // TODO: scan_config.set_val($KEY, config.get_val($KEY));
  configure_scan(scan_config);

  // configure from file
  ska::pst::common::AsciiHeader start_scan_config;
  // TODO: start_scan_config.set_val($KEY, config.get_val($KEY));
  start_scan(start_scan_config);
}

void ska::pst::stat::StatApplicationManager::validate_configure_beam(const ska::pst::common::AsciiHeader& config, ska::pst::common::ValidationContext *context)
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam config={}", config.raw());

  // Iterate through the vector and validate existince of required data header keys
  for (const std::string& config_key : data_config_keys) {
      if (config.has(config_key))
      {
        SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam data {}={}", config_key, config.get_val(config_key));
        data_beam_config.set_val(config_key, config.get_val(config_key));
      } else {
        context->add_missing_field_error(config_key);
      }
  }// Iterate through the vector and validate existince of required weights header keys
  for (const std::string& config_key : weights_config_keys) {
      if (config.has(config_key))
      {
        SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam weights{}={}", config_key, config.get_val(config_key));
        weights_beam_config.set_val(config_key, config.get_val(config_key));
      } else {
        context->add_missing_field_error(config_key);
      }
  }
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam complete");
}

void ska::pst::stat::StatApplicationManager::validate_configure_scan(const ska::pst::common::AsciiHeader& config, ska::pst::common::ValidationContext *context)
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_scan");
  /* TODO
  if (config.has($KEY))
  {
    SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam DATA_KEY={}", config.get_val($KEY));
    scan_config.set_val($KEY, config.get_val($KEY));
  } else {
    context->add_missing_field_error($KEY);
  }
  */
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_scan done");
}

void ska::pst::stat::StatApplicationManager::validate_start_scan(const ska::pst::common::AsciiHeader& config)
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_start_scan");
  /* TODO
  if (config.has($KEY))
  {
    SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_configure_beam DATA_KEY={}", config.get_val($KEY));
    startscan_config.set_val($KEY, config.get_val($KEY));
  } else {
    context->add_missing_field_error($KEY);
  }
  */
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::validate_start_scan done");
}

void ska::pst::stat::StatApplicationManager::perform_initialise()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_initialise()");
  /* TODO:
  - initialise stat computer?
  - initialise stat publisher?
  */
}

void ska::pst::stat::StatApplicationManager::perform_configure_beam()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam");
  // Initialise pointers
  data_rb_view = std::make_unique<ska::pst::smrb::DataBlockView>(data_beam_config.get_val("DATA_KEY"));
  weights_rb_view = std::make_unique<ska::pst::smrb::DataBlockView>(weights_beam_config.get_val("WEIGHTS_KEY"));

  // TODO: Refactor data_rb_view and weights_rb_view with SMRB Segment Producer
  // Connect to SMRB
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam data_rb_view->connect({})", timeout);
  data_rb_view->connect(timeout);
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam data_rb_view->lock()");
  data_rb_view->lock();

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam weights_rb_view->connect({})", timeout);
  weights_rb_view->connect(timeout);
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam weights_rb_view->lock()");
  weights_rb_view->lock();

  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_beam complete");
}

void ska::pst::stat::StatApplicationManager::perform_configure_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan");
  processor = std::make_unique<StatProcessor>(data_beam_config, weights_beam_config);
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_configure_scan complete");
}

void ska::pst::stat::StatApplicationManager::perform_start_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_start_scan");
  // TODO: start scan config validation??
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_start_scan complete");
}

void ska::pst::stat::StatApplicationManager::perform_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan");
  // processor->process();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_scan complete");
}

void ska::pst::stat::StatApplicationManager::perform_stop_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_stop_scan");
}

void ska::pst::stat::StatApplicationManager::perform_deconfigure_scan()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_scan");
  // processor->drop()?
}

void ska::pst::stat::StatApplicationManager::perform_deconfigure_beam()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam");
  // data_rb_view cleanup
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam data_rb_view->unlock()");
  data_rb_view->unlock();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam data_rb_view->disconnect()");
  data_rb_view->disconnect();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam data_rb_view.reset()");
  data_rb_view.reset();

  // weights_rb_view cleanup
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam weights_rb_view->unlock()");
  weights_rb_view->unlock();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam weights_rb_view->disconnect()");
  weights_rb_view->disconnect();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam weights_rb_view.reset()");
  weights_rb_view.reset();

  // beam_config cleanup
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam data_beam_config.reset()");
  data_beam_config.reset();
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_deconfigure_beam weights_beam_config.reset()");
  weights_beam_config.reset();
}

void ska::pst::stat::StatApplicationManager::perform_terminate()
{
  SPDLOG_DEBUG("ska::pst::stat::StatApplicationManager::perform_terminate");
}
