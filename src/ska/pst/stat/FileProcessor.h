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

#include <memory>
#include <string>

#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/stat/StatProcessor.h"

#ifndef __SKA_PST_STAT_FileProcessor_h
#define __SKA_PST_STAT_FileProcessor_h

namespace ska::pst:stat {

  /**
   * @brief Class used for processing input voltage data files to
   * calculate the output statistics.
   */
  class FileProcessor
  {
    public:
      /**
       * @brief Create instance of a File Processor object.
       *
       * @param config the configuration for the file processing job.
       */
      FileProcessor(
        const ska::pst::common::AsciiHeader& config,
        const std::string& data_file_path,
        const std::string& weights_file_path
      );

      /**
       * @brief Destroy the File Processor object.
       *
       */
      virtual ~FileProcessor();

      /**
       * @brief Process the file based on the configuration passed in the constructor.
       */
      void process();

    private:
      //! shared pointer a statistics processor
      std::shared_ptr<StatProcessor> processor;

      //! the configuration for the current file processing
      ska::pst::common::AsciiHeader config;

      //! the file reader that us used to mmap the data and weights files
      FileReader reader;
  }

} // ska::pst::stat

#endif __SKA_PST_STAT_FileProcessor_h
