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

#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include "ska/pst/common/lmc/LmcServiceException.h"
#include "ska/pst/common/statemodel/StateModel.h"
#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/lmc/StatLmcServiceHandler.h"
#include <spdlog/spdlog.h>

auto beam_configuration_as_ascii_header(
    const ska::pst::lmc::BeamConfiguration &configuration
) -> ska::pst::common::AsciiHeader {
  SPDLOG_TRACE("ska::pst::stat::beam_configuration_as_ascii_header()");
  if (!configuration.has_stat())
  {
    SPDLOG_WARN("BeamConfiguration protobuf message has no STAT.CORE details provided.");

    throw ska::pst::common::LmcServiceException(
      "Expected a STAT.CORE beam configuration object, but none were provided.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::INVALID_ARGUMENT
    );
  }

  const auto &stat_beam_configurations = configuration.stat();

  ska::pst::common::AsciiHeader config;
  config.set_val("DATA_KEY", stat_beam_configurations.data_key());
  config.set_val("WEIGHTS_KEY", stat_beam_configurations.weights_key());

  return config;
}

auto scan_configuration_as_ascii_header(
  const ska::pst::lmc::ScanConfiguration &configuration
) -> ska::pst::common::AsciiHeader {
  SPDLOG_TRACE("ska::pst::stat::scan_configuration_as_ascii_header()");
  if (!configuration.has_stat()) {
    SPDLOG_WARN("ScanConfiguration protobuf message has no STAT.CORE details provided.");

    throw ska::pst::common::LmcServiceException(
      "Expected a STAT.CORE scan configuration object, but none were provided.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::INVALID_ARGUMENT
    );
  }

  const auto& request = configuration.stat();

  ska::pst::common::AsciiHeader scan_configuration;
  scan_configuration.set_val("EB_ID", request.execution_block_id());
  scan_configuration.set("STAT_PROC_DELAY_MS", request.processing_delay_ms());
  scan_configuration.set("STAT_REQ_TIME_BINS", request.req_time_bins());
  scan_configuration.set("STAT_REQ_FREQ_BINS", request.req_freq_bins());
  scan_configuration.set("STAT_NREBIN", request.num_rebin());
  return scan_configuration;
}

void ska::pst::stat::StatLmcServiceHandler::validate_beam_configuration(
  const ska::pst::lmc::BeamConfiguration &request
)
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::validate_beam_configuration()");
  auto config = beam_configuration_as_ascii_header(request);
  ska::pst::common::ValidationContext context;
  stat->validate_configure_beam(config, &context);
}

void ska::pst::stat::StatLmcServiceHandler::validate_scan_configuration(
  const ska::pst::lmc::ScanConfiguration &request
)
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::validate_scan_configuration()");
  auto config = scan_configuration_as_ascii_header(request);
  ska::pst::common::ValidationContext context;
  stat->validate_configure_scan(config, &context);
}

void ska::pst::stat::StatLmcServiceHandler::configure_beam(
  const ska::pst::lmc::BeamConfiguration &request
)
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::configure_beam()");

  if (stat->get_state() == ska::pst::common::State::RuntimeError)
  {
    SPDLOG_WARN("Received configure beam request when in Runtime Error.");

    throw ska::pst::common::LmcServiceException(
      "Received configure beam request when in Runtime Error.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  if (is_beam_configured())
  {
    SPDLOG_WARN("Received configure beam when beam configured already.");

    throw ska::pst::common::LmcServiceException(
      "Beam already configured for STAT.CORE.",
      ska::pst::lmc::ErrorCode::CONFIGURED_FOR_BEAM_ALREADY,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  auto config = beam_configuration_as_ascii_header(request);
  SPDLOG_TRACE("ska::pst::dsp::StatLmcServiceHandler::configure_beam stat->configure_beam(config)");
  // validation will happens on the state model as first part of configure_beam
  // so no need to do validation here.
  stat->configure_beam(config);
}

void ska::pst::stat::StatLmcServiceHandler::deconfigure_beam()
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::deconfigure_beam()");
  SPDLOG_WARN("ska::pst::stat::StatLmcServiceHandler - current state of STAT.CORE = {}", stat->get_name(stat->get_state()));

  if (stat->get_state() == ska::pst::common::State::RuntimeError)
  {
    SPDLOG_WARN("Received deconfigure beam when in Runtime Error.");
    throw ska::pst::common::LmcServiceException(
      "Received deconfigure beam when in Runtime Error.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  if (!is_beam_configured())
  {
    SPDLOG_WARN("Received deconfigure beam when beam not configured already.");
    throw ska::pst::common::LmcServiceException(
      "STAT.CORE not configured for beam.",
      ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_BEAM,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  if (is_scan_configured())
  {
    SPDLOG_WARN("Received deconfigure beam when scan is already configured.");
    throw ska::pst::common::LmcServiceException(
      "STAT.CORE is configured for scan but trying to deconfigure beam.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  stat->deconfigure_beam();
}

void ska::pst::stat::StatLmcServiceHandler::get_beam_configuration(
  ska::pst::lmc::BeamConfiguration* response
)
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::get_beam_configuration()");
  if (!is_beam_configured())
  {
    SPDLOG_WARN("Received request to get beam configuration when beam not configured.");
    throw ska::pst::common::LmcServiceException(
      "STAT.CORE not configured for beam.",
      ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_BEAM,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  const auto &resources = stat->get_beam_configuration();

  ska::pst::lmc::StatBeamConfiguration *stat_beam_configuration = response->mutable_stat();
  stat_beam_configuration->set_data_key(resources.get_val("DATA_KEY"));
  stat_beam_configuration->set_weights_key(resources.get_val("WEIGHTS_KEY"));
}

auto ska::pst::stat::StatLmcServiceHandler::is_beam_configured() const noexcept -> bool
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::is_beam_configured()");
  return (stat->is_beam_configured());
}

void ska::pst::stat::StatLmcServiceHandler::configure_scan(
  const ska::pst::lmc::ScanConfiguration &configuration
)
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::configure_scan()");

  if (stat->get_state() == ska::pst::common::State::RuntimeError)
  {
    SPDLOG_WARN("Received configure scan when in Runtime Error.");
    throw ska::pst::common::LmcServiceException(
      "Received configure scan when in Runtime Error.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  if (!is_beam_configured())
  {
    SPDLOG_WARN("Received scan configuration request when beam not configured already.");

    throw ska::pst::common::LmcServiceException(
      "STAT.CORE not configured for beam.",
      ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_BEAM,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  if (is_scan_configured())
  {
    SPDLOG_WARN("Received configure_scan when scan already configured.");

    throw ska::pst::common::LmcServiceException(
      "Scan already configured for STAT.CORE.",
      ska::pst::lmc::ErrorCode::CONFIGURED_FOR_SCAN_ALREADY,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  auto scan_configuration = scan_configuration_as_ascii_header(configuration);

  // validation will happens on the state model as first part of configure_scan
  // so no need to do validation here.
  stat->configure_scan(scan_configuration);
}

void ska::pst::stat::StatLmcServiceHandler::deconfigure_scan()
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::deconfigure_scan()");
  SPDLOG_WARN("ska::pst::stat::StatLmcServiceHandler - current state of STAT.CORE = {}", stat->get_name(stat->get_state()));

  if (stat->get_state() == ska::pst::common::State::RuntimeError)
  {
    SPDLOG_WARN("Received deconfigure scan when in Runtime Error.");
    throw ska::pst::common::LmcServiceException(
      "Received deconfigure scan when in Runtime Error.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  if (stat->get_state() != ska::pst::common::State::ScanConfigured)
  {
    SPDLOG_WARN("Received deconfigure_scan when scan not already configured.");

    throw ska::pst::common::LmcServiceException(
      "Scan not currently configured for STAT.CORE.",
      ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_SCAN,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  // Should not let a deconfigure request come through if in a scanning state.
  if (stat->get_state() == ska::pst::common::State::Scanning)
  {
    SPDLOG_WARN("Received deconfigure request when still scanning.");
    throw ska::pst::common::LmcServiceException(
      "STAT.CORE is scanning but trying to deconfigure scan.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  stat->deconfigure_scan();
}

void ska::pst::stat::StatLmcServiceHandler::get_scan_configuration(
    ska::pst::lmc::ScanConfiguration *configuration
)
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::get_scan_configuration()");
  if (!is_scan_configured())
  {
    SPDLOG_WARN("Received get_scan_configuration when scan not already configured.");

    throw ska::pst::common::LmcServiceException(
      "Scan Not currently configured for STAT.CORE.",
      ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_SCAN,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  const auto &curr_scan_configuration = stat->get_scan_configuration();
  auto *stat_scan_configuration = configuration->mutable_stat();

  stat_scan_configuration->set_execution_block_id(curr_scan_configuration.get_val("EB_ID"));
  stat_scan_configuration->set_processing_delay_ms(curr_scan_configuration.get_uint32("STAT_PROC_DELAY_MS"));
  stat_scan_configuration->set_req_time_bins(curr_scan_configuration.get_uint32("STAT_REQ_TIME_BINS"));
  stat_scan_configuration->set_req_freq_bins(curr_scan_configuration.get_uint32("STAT_REQ_FREQ_BINS"));
  stat_scan_configuration->set_num_rebin(curr_scan_configuration.get_uint32("STAT_NREBIN"));
}

auto ska::pst::stat::StatLmcServiceHandler::is_scan_configured() const noexcept -> bool
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::is_scan_configured()");
  return stat->is_scan_configured();
}

void ska::pst::stat::StatLmcServiceHandler::start_scan(const ska::pst::lmc::StartScanRequest& request)
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::start_scan()");

  if (stat->get_state() == ska::pst::common::State::RuntimeError)
  {
    SPDLOG_WARN("Received start scan when in Runtime Error.");
    throw ska::pst::common::LmcServiceException(
      "Received start scan when in Runtime Error.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  if (!is_scan_configured())
  {
    SPDLOG_WARN("Received scan request when scan not already configured.");

    throw ska::pst::common::LmcServiceException(
      "Scan not currently configured for STAT.CORE.",
      ska::pst::lmc::ErrorCode::NOT_CONFIGURED_FOR_SCAN,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  if (is_scanning())
  {
    SPDLOG_WARN("Received scan request when already scanning.");
    throw ska::pst::common::LmcServiceException(
      "STAT.CORE is already scanning.",
      ska::pst::lmc::ErrorCode::ALREADY_SCANNING,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }
  ska::pst::common::AsciiHeader start_scan_config;
  start_scan_config.set("SCAN_ID", request.scan_id());

  stat->start_scan(start_scan_config);
}

void ska::pst::stat::StatLmcServiceHandler::stop_scan()
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::stop_scan()");
  if (stat->get_state() == ska::pst::common::State::RuntimeError)
  {
    SPDLOG_WARN("Received stop scan when in Runtime Error.");
    throw ska::pst::common::LmcServiceException(
      "Received stop scan when in Runtime Error.",
      ska::pst::lmc::ErrorCode::INVALID_REQUEST,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  if (!is_scanning())
  {
    SPDLOG_WARN("Received stop_scan request when not scanning.");

    throw ska::pst::common::LmcServiceException(
      "Received stop_scan request when STAT.CORE is not scanning.",
      ska::pst::lmc::ErrorCode::NOT_SCANNING,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }

  stat->stop_scan();
}

void ska::pst::stat::StatLmcServiceHandler::reset()
{
  SPDLOG_INFO("ska::pst::stat::StatLmcServiceHandler::reset()");
  if (stat->get_state() == ska::pst::common::State::RuntimeError)
  {
    stat->reset();
  }
}

auto ska::pst::stat::StatLmcServiceHandler::is_scanning() const noexcept -> bool
{
  return (stat->is_scanning());
}

void ska::pst::stat::StatLmcServiceHandler::get_monitor_data(
  ska::pst::lmc::MonitorData* data
)
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::get_monitor_data()");
  if (!is_scanning())
  {
    SPDLOG_WARN("Received get_monitor_data request when not scanning.");

    throw ska::pst::common::LmcServiceException(
      "Received get_monitor_data request when STAT.CORE is not scanning.",
      ska::pst::lmc::ErrorCode::NOT_SCANNING,
      grpc::StatusCode::FAILED_PRECONDITION
    );
  }
  else
  {
    auto *stat_monitor_data = data->mutable_stat();
    SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data - stat->get_scalar_stats()");
    auto stats = stat->get_scalar_stats();
    SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data - stat->get_scalar_stats() done");

    std::vector<float> mean_frequency_avg(4);
    std::vector<float> mean_frequency_avg_rfi_excised(4);
    std::vector<float> variance_frequency_avg(4);
    std::vector<float> variance_frequency_avg_rfi_excised(4);
    std::vector<uint32_t> num_clipped_samples(4);
    std::vector<uint32_t> num_clipped_samples_rfi_excised(4);

    SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data mean_frequency_avg.size={}",stats.mean_frequency_avg.size());
    SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data mean_frequency_avg_rfi_excised.size={}",stats.mean_frequency_avg_rfi_excised.size());
    SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data variance_frequency_avg.size={}",stats.variance_frequency_avg.size());
    SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data variance_frequency_avg_rfi_excised.size={}",stats.variance_frequency_avg_rfi_excised.size());
    SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data num_clipped_samples.size={}",stats.num_clipped_samples.size());
    SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data num_clipped_samples_rfi_excised.size={}",stats.num_clipped_samples_rfi_excised.size());

    flatten_2d_vec(stats.mean_frequency_avg, mean_frequency_avg);
    flatten_2d_vec(stats.mean_frequency_avg_rfi_excised, mean_frequency_avg_rfi_excised);
    flatten_2d_vec(stats.variance_frequency_avg, variance_frequency_avg);
    flatten_2d_vec(stats.variance_frequency_avg_rfi_excised, variance_frequency_avg_rfi_excised);
    flatten_2d_vec(stats.num_clipped_samples, num_clipped_samples);
    flatten_2d_vec(stats.num_clipped_samples_rfi_excised, num_clipped_samples_rfi_excised);

    if (mean_frequency_avg.empty() ||
        mean_frequency_avg_rfi_excised.empty() ||
        variance_frequency_avg.empty() ||
        variance_frequency_avg_rfi_excised.empty() ||
        num_clipped_samples.empty() ||
        num_clipped_samples_rfi_excised.empty())
    {
      SPDLOG_WARN("StatLmcServiceHandler::get_monitor_data empty scalar stats");
    }
    else{
      *stat_monitor_data->mutable_mean_frequency_avg() = { mean_frequency_avg.begin(), mean_frequency_avg.end() };
      *stat_monitor_data->mutable_mean_frequency_avg_masked() = { mean_frequency_avg_rfi_excised.begin(), mean_frequency_avg_rfi_excised.end() };
      *stat_monitor_data->mutable_variance_frequency_avg() = { variance_frequency_avg.begin(), variance_frequency_avg.end() };
      *stat_monitor_data->mutable_variance_frequency_avg_masked() = { variance_frequency_avg_rfi_excised.begin(), variance_frequency_avg_rfi_excised.end() };
      *stat_monitor_data->mutable_num_clipped_samples() = { num_clipped_samples.begin(), num_clipped_samples.end() };
      *stat_monitor_data->mutable_num_clipped_samples_masked() = { num_clipped_samples_rfi_excised.begin(), num_clipped_samples_rfi_excised.end() };

      SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data stat_monitor_data->mean_frequency_avg().size()={}", stat_monitor_data->mean_frequency_avg().size());
      SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data stat_monitor_data->mean_frequency_avg_masked().size()={}", stat_monitor_data->mean_frequency_avg_masked().size());
      SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data stat_monitor_data->variance_frequency_avg().size()={}", stat_monitor_data->variance_frequency_avg().size());
      SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data stat_monitor_data->variance_frequency_avg_masked().size()={}", stat_monitor_data->variance_frequency_avg_masked().size());
      SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data stat_monitor_data->num_clipped_samples().size()={}", stat_monitor_data->num_clipped_samples().size());
      SPDLOG_TRACE("StatLmcServiceHandler::get_monitor_data stat_monitor_data->num_clipped_samples_masked().size()={}", stat_monitor_data->num_clipped_samples_masked().size());
    }
  }
}

void ska::pst::stat::StatLmcServiceHandler::get_env(
  ska::pst::lmc::GetEnvironmentResponse* /*response*/
) noexcept
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::get_env() placeholder");
}

void ska::pst::stat::StatLmcServiceHandler::go_to_runtime_error(
  std::exception_ptr exc
)
{
  SPDLOG_TRACE("ska::pst::stat::StatLmcServiceHandler::go_to_runtime_error()");
  stat->go_to_runtime_error(exc);
}
