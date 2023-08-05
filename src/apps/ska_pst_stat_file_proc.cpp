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

#include "ska/pst/stat/FileProcessor.h"
#include "ska/pst/common/utils/Logging.h"

#include <spdlog/spdlog.h>
#include <iostream>

#include <unistd.h>

void usage();

auto main(int argc, char *argv[]) -> int
{
  ska::pst::common::setup_spdlog();

  std::string config_filename;
  std::string data_filename;
  std::string weights_filename;

  char verbose = 0;
  opterr = 0;
  int c = 0;

  while ((c = getopt(argc, argv, "hc:d:w:v")) != EOF)
  {
    switch(c)
    {
      case 'h':
        usage();
        exit(EXIT_SUCCESS);
        break;

      case 'c':
        config_filename = optarg;
        break;

      case 'd':
        data_filename = optarg;
        break;

      case 'w':
        weights_filename = optarg;
        break;

      case 'v':
        verbose++;
        break;

      default:
        std::cerr << "ERROR: unrecognised option: -" << static_cast<char>(optopt) << std::endl;
        usage();
        return EXIT_FAILURE;
        break;
    }
  }

  if (verbose > 0)
  {
    spdlog::set_level(spdlog::level::debug);
    if (verbose > 1)
    {
      spdlog::set_level(spdlog::level::trace);
    }
  }

  // Check arguments
  if (config_filename.empty())
  {
    SPDLOG_ERROR("ERROR: config filename not specified");
    usage();
    return EXIT_FAILURE;
  }

  if (data_filename.empty())
  {
    SPDLOG_ERROR("ERROR: data filename not specified");
    usage();
    return EXIT_FAILURE;
  }

  if (weights_filename.empty())
  {
    SPDLOG_ERROR("ERROR: weights filename not specified");
    usage();
    return EXIT_FAILURE;
  }

  int return_code = 0;

  try
  {
    // prepare the config and header
    ska::pst::common::AsciiHeader config;

    // load FileProcessor configuration
    SPDLOG_DEBUG("loading configuration from {}", config_filename);
    config.load_from_file(config_filename);

    SPDLOG_DEBUG("constructing FileProcessor from data filename={} and weights filename={}", data_filename, weights_filename);
    ska::pst::stat::FileProcessor file_processor(config, data_filename, weights_filename);

    SPDLOG_DEBUG("constructing calling FileProcessor::process");
    file_processor.process();

  }
  catch (std::exception& exc)
  {
    SPDLOG_ERROR("Exception caught: {}", exc.what());
    return_code = 1;
  }

  SPDLOG_DEBUG("return return_code={}", return_code);
  return return_code;
}

void usage()
{
  std::cout << "Usage: ska_pst_stat_file_proc -c config -d data -w weights" << std::endl;
  std::cout << std::endl;
  std::cout << "  -c config   name of file processor configuration file" << std::endl;
  std::cout << "  -d data     name of data file" << std::endl;
  std::cout << "  -w weights  name of weights file" << std::endl;
  std::cout << "  -h          print this help text" << std::endl;
  std::cout << "  -v          verbose output" << std::endl;
}
