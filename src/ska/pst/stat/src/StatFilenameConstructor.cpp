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

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/FileWriter.h"
#include "ska/pst/stat/StatFilenameConstructor.h"

#include <map>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

ska::pst::stat::StatFilenameConstructor::StatFilenameConstructor()
{
}

ska::pst::stat::StatFilenameConstructor::StatFilenameConstructor(const ska::pst::common::AsciiHeader& header)
{
  if (header.has("STAT_BASE_PATH"))
  {
    set_base_path(header.get_val("STAT_BASE_PATH"));
  }
  if (header.has("EB_ID"))
  {
    set_eb_id(header.get_val("EB_ID"));
  }
  if (header.has("SCAN_ID"))
  {
    set_scan_id(header.get_val("SCAN_ID"));
  }
  if (header.has("TELESCOPE"))
  {
    set_telescope(header.get_val("TELESCOPE"));
  }
}

void ska::pst::stat::StatFilenameConstructor::set_base_path(const std::string& stat_base)
{
  SPDLOG_TRACE("ska::pst::stat::StatFilenameConstructor::set_base_path stat_base={}", stat_base);
  stat_base_path = std::filesystem::path(stat_base);
}

void ska::pst::stat::StatFilenameConstructor::set_eb_id(const std::string& eb_id)
{
  SPDLOG_TRACE("ska::pst::stat::StatFilenameConstructor::set_eb_id eb_id={}", eb_id);
  eb_id_path = std::filesystem::path(eb_id);
}

void ska::pst::stat::StatFilenameConstructor::set_scan_id(const std::string& scan_id)
{
  SPDLOG_TRACE("ska::pst::stat::StatFilenameConstructor::set_scan_id scan_id={}", scan_id);
  scan_id_path = std::filesystem::path(scan_id);
}

auto ska::pst::stat::StatFilenameConstructor::get_subsystem_from_telescope(const std::string& telescope) -> std::string
{
  std::map<std::string, std::string> subsystem_path_map {
    { "SKALow", "pst-low" },
    { "SKAMid", "pst-mid" },
  };

  if (subsystem_path_map.find(telescope) == subsystem_path_map.end())
  {
    SPDLOG_WARN("ska::pst::stat::StatFilenameConstructor::get_subsystem_from_telescope telescope={} did not map to a subsystem path", telescope);
    throw std::runtime_error("ska::pst::stat::StatFilenameConstructor::get_subsystem_from_telescope could not map telescope to a subsystem path.");
  }
  return subsystem_path_map[telescope];
}

void ska::pst::stat::StatFilenameConstructor::set_telescope(const std::string& telescope)
{
  subsystem_id_path = std::filesystem::path(get_subsystem_from_telescope(telescope));
}

auto ska::pst::stat::StatFilenameConstructor::get_filename(
  const std::string& utc_start, uint64_t obs_offset, uint64_t file_number) -> std::filesystem::path
{
  std::filesystem::path product_path{"product"};
  std::filesystem::path stream_path{"monitoring_stats"};

  if (stat_base_path.empty())
  {
    SPDLOG_ERROR("ska::pst::stat::StatFilenameConstructor::get_filename stat_base_path has not be set");
    throw std::runtime_error("stat_base_path has not be set");
  }

  if (eb_id_path.empty())
  {
    SPDLOG_ERROR("ska::pst::stat::StatFilenameConstructor::get_filename eb_id_path has not be set");
    throw std::runtime_error("eb_id_path has not be set");
  }

  if (subsystem_id_path.empty())
  {
    SPDLOG_ERROR("ska::pst::stat::StatFilenameConstructor::get_filename subsystem_id_path has not be set");
    throw std::runtime_error("subsystem_id_path has not be set");
  }

  if (scan_id_path.empty())
  {
    SPDLOG_ERROR("ska::pst::stat::StatFilenameConstructor::get_filename scan_id_path has not be set");
    throw std::runtime_error("scan_id_path has not be set");
  }

  // construct the directory for files from the scan
  std::filesystem::path scan_path = stat_base_path / product_path / eb_id_path / subsystem_id_path / scan_id_path / stream_path;

  // file name (with no directory prefix) using FileWriter for consistent nameing
  std::filesystem::path filename = ska::pst::common::FileWriter::get_filename(utc_start, obs_offset, file_number);

  // full path to the STAT output file
  std::filesystem::path output_file = scan_path / filename.replace_extension("h5");

  return output_file;
}
