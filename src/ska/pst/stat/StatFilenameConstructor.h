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
#include <filesystem>
#include <string>

#ifndef SKA_PST_STAT_StatFilenameConstructor_h
#define SKA_PST_STAT_StatFilenameConstructor_h

namespace ska::pst::stat {

  /**
   * @brief An implementation of the StatPublisher that dumps the data to a HDF5 file.
   *
   */
  class StatFilenameConstructor
  {
    public:

      /**
       * @brief Construct a new Stat Filename Constructor object
       *
       */
      StatFilenameConstructor();

      /**
       * @brief Create instance of a Stat Filename Constructor Object
       *
       * @param header the header of the scan
       */
      StatFilenameConstructor(const ska::pst::common::AsciiHeader& header);

      /**
       * @brief Destroy the Stat HDF5 File Writer object.
       *
       */
      ~StatFilenameConstructor() = default;

      /**
       * @brief Set the base path object
       *
       * @param base_path base directory to which files should be written
       */
      void set_base_path(const std::string& base_path);

      /**
       * @brief Set the eb_id_path attribute
       *
       * @param eb_id execution block id of the scan
       */
      void set_eb_id(const std::string& eb_id);

      /**
       * @brief Set the scan_id_path attribute
       *
       * @param scan_id the id of the scan
       */
      void set_scan_id(const std::string& scan_id);

      /**
       * @brief Set the subsystem_id_path attribute to pst-low or pst-mid
       *
       * @param telescope the telescope name, must be SKALow or SKAMid
       */
      void set_telescope(const std::string& telescope);

      /**
       * @brief Get the subsystem id from telescope
       *
       * @param telescope the telescope name [SKALow or SKAMid]
       * @return std::string the subsystem id matching the telescope name
       */
      static std::string get_subsystem_from_telescope(const std::string& telescope);

      /**
       * @brief Return the full filesystem path to a HDF5 output filename.
       *
       * The filename will be constructed as:
       * [stat_base]/[eb_id]/[pst-low|pst-mid]/[scan_id]/monitoring_stats/[utc_start]_[obs_offset]_[file_number].h5
       *
       * @param utc_start start utc of the scan
       * @param obs_offset byte offset of the data segement stat storage
       * @param file_number file sequence number produced by the publisher
       * @return std::filesystem::path full path to output file name to be created.
       */
      std::filesystem::path get_filename(const std::string& utc_start, uint64_t obs_offset, uint64_t file_number);

    private:

      //! base directory to which files should be written
      std::filesystem::path stat_base_path;

      //! execution block id of the scan
      std::filesystem::path eb_id_path;

      //! id of the scan
      std::filesystem::path scan_id_path;

      //! The sub-system path for the telescope
      std::filesystem::path subsystem_id_path;
  };

} // namespace ska::pst::stat

#endif // SKA_PST_STAT_StatFilenameConstructor_h
