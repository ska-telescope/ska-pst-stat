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
#include "ska/pst/stat/StatPublisher.h"

#include <spdlog/spdlog.h>
#include <filesystem>
#include <H5Cpp.h>
#include <memory>
#include <string>
#include <cstring>

#ifndef SKA_PST_STAT_StatHdf5FileWriter_h
#define SKA_PST_STAT_StatHdf5FileWriter_h

namespace ska::pst::stat {

  typedef struct stat_hdf5_header {
    //! the execution block that the data relates to
    char* eb_id;

    //! the scan that the data relates to
    uint64_t scan_id;

    //! the beam used to capture the data
    char* beam_id;

    //! the UTC start time of the scan in ISO 8601 format
    char* utc_start;

    //! the offset the UTC start time starting at the fractional offset
    double t_min;

    //! the end of the sample time. This is equivalent to t_min + total sample time
    double t_max;

    //! the centre frequency of the data in MHz
    double freq;

    //! the bandwidth of the data in MHz
    double bandwidth;

    //! the start channel number
    uint32_t start_chan;

    //! number of polarisations represented in storage vectors
    uint32_t npol;

    //! number of dimensions represented in storage vectors
    uint32_t ndim;

    //! number of channels representated in storage vectors
    uint32_t nchan;

    //! number of bins represented in storage vectors
    uint32_t nbin;

    //! number of spectral bins in the spectrogram
    uint32_t nfreq_bins;

    //! number of temporal bins in the timeseries, timeseries_masked and spectrogram attributes
    uint32_t ntime_bins;

    //! number of rebinned bins represented in storage vectors
    uint32_t nrebin;

    //! channel centre frequencies
    hvl_t chan_freq;

    //! frequency bins
    hvl_t frequency_bins;

    //! frequency bins
    hvl_t timeseries_bins;

  } stat_hdf5_header_t;

  /**
   * @brief An implementation of the StatPublisher that dumps the data to a HDF5 file.
   *
   */
  class StatHdf5FileWriter : public StatPublisher
  {
    public:
      /**
       * @brief Create instance of a Stat HDF5 File Writer object.
       *
       * @param config the configuration current voltage data stream.
       */
      StatHdf5FileWriter(
        const ska::pst::common::AsciiHeader& config
      );

      /**
       * @brief Destroy the Stat HDF5 File Writer object.
       *
       */
      virtual ~StatHdf5FileWriter();

      /**
       * @brief publish the computed statistics as a HDF5 file.
       *
       * @param storage reference to the computed statistics to publish.
       */
      void publish(std::shared_ptr<StatStorage> storage) override;

      /**
       * @brief get the HDF5 compound data type for STAT.
       *
       */
      auto get_hdf5_header_datatype() -> H5::CompType;

      /**
       * @brief Return the full filesystem path to the HDF5 output filename.
       *
       * @param utc_start start utc of the scan
       * @param obs_offset byte offset of the data segement stat storage
       * @param file_number file sequence number produced by the publisher
       * @return std::filesystem::path full path to output file name to be created.
       */
      std::filesystem::path get_output_filename(const std::string& utc_start, uint64_t obs_offset, uint64_t file_number);

      /**
       * @brief Construct the filename path for an output HDF5 filename. The filename will be constructed as
       * [stat_base]/[eb_id]/[pst-low|pst-mid]/[scan_id]/monitoring_stats/[utc_start]_[obs_offset]_[file_number].h5
       *
       * @param stat_base base directory to which files should be written
       * @param eb_id execution block id of the scan
       * @param scan_id id of the scan
       * @param telescope telescope on which the scan is performed, must be SKALow or SKAMid
       * @param utc_start start utc of the scan
       * @param obs_offset byte offset of the first data sample in the segment
       * @param file_number file sequence number produced by the publisher
       * @return std::filesystem::path absolute path to the output filename
       */
      static std::filesystem::path construct_output_filename(const std::string& stat_base, const std::string& eb_id,
        const std::string& scan_id, const std::string& telescope,
        const std::string& utc_start, uint64_t obs_offset, uint64_t file_number);

    private:

      //! sequence number of HDF5 files produced
      unsigned file_number{0};

      //! write array out to a HDF5 DataSpace
      void write_array(const std::vector<char>& data, const std::string& field_name, const H5::PredType& datatype, H5::DataSpace& dataspace);

      //! write a 1D vector to a header variable length value
      template<typename T>
      void write_1d_vec_header(std::vector<T>& data, hvl_t& header_value)
      {
        header_value.len = data.size();
        header_value.p = reinterpret_cast<void *>(&data.front());
      }

      //! write a 1D vector to the HDF5 with a given data type
      template<typename T>
      void write_1d_vec(const std::vector<T>& data, std::string field_name, const H5::PredType& datatype, std::vector<char>& temp_data) {
        SPDLOG_DEBUG("ska::pst::stat::StatHdf5FileWriter::write_1d_vec - writing {}", field_name);
        flatten_1d_vec(data, temp_data);
        hsize_t dimssf[1] = { data.size() };
        H5::DataSpace dataspace(1, dimssf);
        write_array(temp_data, field_name, datatype, dataspace);
      }

      //! write a 2D vector to the HDF5 with a given data type
      template<typename T>
      void write_2d_vec(const std::vector<std::vector<T>>& data, std::string field_name, const H5::PredType& datatype, std::vector<char>& temp_data) {
        SPDLOG_DEBUG("ska::pst::stat::StatHdf5FileWriter::write_2d_vec - writing {}", field_name);
        flatten_2d_vec(data, temp_data);
        // create HDF5 dims
        hsize_t dimssf[2] = { data.size(), data[0].size() };
        H5::DataSpace dataspace(2, dimssf);
        write_array(temp_data, field_name, datatype, dataspace);
      }

      //! write a 3D vector to the HDF5 with a given data type
      template<typename T>
      void write_3d_vec(const std::vector<std::vector<std::vector<T>>>& data, std::string field_name, const H5::PredType& datatype, std::vector<char>& temp_data) {
        SPDLOG_DEBUG("ska::pst::stat::StatHdf5FileWriter::write_3d_vec - writing {}", field_name);
        flatten_3d_vec(data, temp_data);
        hsize_t dimssf[3] = { data.size(), data[0].size(), data[0][0].size() };
        H5::DataSpace dataspace(3, dimssf);
        write_array(temp_data, field_name, datatype, dataspace);
      }

      //! flatten and copy 1D vector to output data vector
      template<typename T>
      static size_t flatten_1d_vec(const std::vector<T>& vec, std::vector<char>& data)
      {
        size_t dim1 = vec.size();

        data.resize(dim1 * sizeof(T));
        std::memcpy(data.data(), vec.data(), dim1 * sizeof(T));

        return dim1;
      }

      //! flatten and copy 2D vector to output data vector
      template<typename T>
      static size_t flatten_2d_vec(const std::vector<std::vector<T>>& vec, std::vector<char>& data)
      {
        size_t dim1 = vec.size();
        size_t dim2 = vec[0].size();

        size_t num_elements = dim1 * dim2;
        size_t stride = dim2 * sizeof(T);

        data.resize(num_elements * sizeof(T));
        size_t offset{0};
        for (auto i=0; i<dim1; i++)
        {
          std::memcpy(data.data() + offset, vec[i].data(), stride);
          offset += stride;
        }

        return num_elements;
      }

      //! flatten and copy 3D vector to output data vector
      template<typename T>
      static size_t flatten_3d_vec(const std::vector<std::vector<std::vector<T>>>& vec, std::vector<char>& data)
      {
        size_t dim1 = vec.size();
        size_t dim2 = vec[0].size();
        size_t dim3 = vec[0][0].size();

        size_t num_elements = dim1 * dim2 * dim3;
        size_t stride = dim3 * sizeof(T);

        data.resize(num_elements * sizeof(T));

        size_t offset{0};
        for (auto i=0; i<dim1; i++)
        {
          for (auto j=0; j<dim2; j++)
          {
            std::memcpy(data.data() + offset, vec[i][j].data(), stride);
            offset += stride;
          }
        }

        return num_elements;
      }

      std::shared_ptr<H5::H5File> file{nullptr};

  };

} // namespace ska::pst::stat

#endif // SKA_PST_STAT_StatHdf5FileWriter_h
