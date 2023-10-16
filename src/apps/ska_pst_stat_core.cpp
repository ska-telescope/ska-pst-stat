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

#include "ska/pst/stat/config.h"
#include "ska/pst/stat/StatApplicationManager.h"
#include "ska/pst/common/utils/AsciiHeader.h"
#include "ska/pst/common/utils/Logging.h"

#ifdef HAVE_PROTOBUF_gRPC_SKAPSTLMC
#include "ska/pst/common/lmc/LmcService.h"
#include "ska/pst/stat/lmc/StatLmcServiceHandler.h"
#endif

#include <spdlog/spdlog.h>
#include <iostream>
#include <csignal>

#include <unistd.h>

void usage();
void signal_handler(int signal_value);
auto scalar_stats_different(const ska::pst::stat::StatStorage::scalar_stats_t& a, const ska::pst::stat::StatStorage::scalar_stats_t& b) -> bool;
bool signal_received = false; // NOLINT

auto main(int argc, char *argv[]) -> int
{
  ska::pst::common::setup_spdlog();

  // configuration file to use when not controlled by LMC
  std::string config_file{};

  // directory to which statistics files will be written
  std::string stat_path = "/tmp";

  // seconds to wait when not controlled by LMC
  int64_t duration = INT64_MAX;

#ifdef HAVE_PROTOBUF_gRPC_SKAPSTLMC
  std::string service_name = "STAT.CORE";
  int control_port = -1;
#endif

  char verbose = 0;
  opterr = 0;
  int c = 0;

#ifdef HAVE_PROTOBUF_gRPC_SKAPSTLMC
  while ((c = getopt(argc, argv, "c:d:f:ht:v")) != EOF)
#else
  while ((c = getopt(argc, argv, "d:f:ht:v")) != EOF)
#endif
  {
    switch(c)
    {
#ifdef HAVE_PROTOBUF_gRPC_SKAPSTLMC
      case 'c':
        control_port = atoi(optarg);
        break;
#endif

      case 'd':
        stat_path = std::string(optarg);
        break;

      case 'f':
        config_file = std::string(optarg);
        break;

      case 'h':
        usage();
        exit(EXIT_SUCCESS);
        break;

      case 't':
        duration = static_cast<int64_t>(atoi(optarg) * ska::pst::common::microseconds_per_second);
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

#ifdef HAVE_PROTOBUF_gRPC_SKAPSTLMC
  if (config_file.length() == 0 && control_port == -1)
  {
    SPDLOG_ERROR("ERROR: require either a configuration file or control port");
#else
  if (config_file.length() == 0)
  {
    SPDLOG_ERROR("ERROR: require a configuration file");
#endif
    usage();
    return EXIT_FAILURE;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try
  {
    std::shared_ptr<ska::pst::stat::StatApplicationManager> stat = std::make_shared<ska::pst::stat::StatApplicationManager>(stat_path);

#ifdef HAVE_PROTOBUF_gRPC_SKAPSTLMC
    std::unique_ptr<ska::pst::common::LmcService> lmc_service{nullptr};
    if (control_port != -1)
    {
      SPDLOG_INFO("Setting up gRPC LMC service on port {}", control_port);
      auto lmc_handler = std::make_shared<ska::pst::stat::StatLmcServiceHandler>(stat);
      lmc_service = std::make_unique<ska::pst::common::LmcService>(service_name, lmc_handler, control_port);
      lmc_service->start();
      SPDLOG_TRACE("gRPC LMC service has been started");
    }
    else
#endif
    {
      // performs configure_beam, configure_scan and start_scan using config_file
      stat->configure_from_file(config_file);

      SPDLOG_TRACE("waiting {} seconds before self-termination", duration / ska::pst::common::microseconds_per_second);
      ska::pst::stat::StatStorage::scalar_stats_t prev{};
      while (!signal_received && duration > 0)
      {
        usleep(ska::pst::common::microseconds_per_decisecond);
        duration -= ska::pst::common::microseconds_per_decisecond;
        ska::pst::stat::StatStorage::scalar_stats_t ss = stat->get_scalar_stats();

        // print the statistics to stdout when they are changed
        if (scalar_stats_different(ss, prev))
        {
          for (unsigned ipol=0; ipol<ss.mean_frequency_avg.size(); ipol++)
          {
            for (unsigned idim=0; idim<ss.mean_frequency_avg[ipol].size(); idim++)
            {
              SPDLOG_INFO("Pol{} Dim{}: mean={} variance={} nclipped={}", ipol, idim,
                ss.mean_frequency_avg[ipol][idim],
                ss.variance_frequency_avg[ipol][idim],
                ss.num_clipped_samples[ipol][idim]);
            }
          }
        }
        prev = ss;
      }
      SPDLOG_TRACE("terminating");

      stat->stop_scan();
      stat->deconfigure_scan();
      stat->deconfigure_beam();
    }

#ifdef HAVE_PROTOBUF_gRPC_SKAPSTLMC
    if (lmc_service)
    {
      while (!signal_received)
      {
        usleep(ska::pst::common::microseconds_per_decisecond);
      }

      // signal the gRPC server to exit
      SPDLOG_INFO("Stopping gRPC LMC service");
      lmc_service->stop();
      SPDLOG_TRACE("gRPC LMC service has stopped");
    }

#endif

    stat->quit();
  }
  catch (std::exception& exc)
  {
    SPDLOG_ERROR("Exception caught: {}", exc.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void usage()
{
  std::cout << "Usage: ska_pst_stat_core" << std::endl;
  std::cout << std::endl;
#ifdef HAVE_PROTOBUF_gRPC_SKAPSTLMC
  std::cout << "  -c port     port on which to accept control commands" << std::endl;
#endif
  std::cout << "  -d path     write output files to the path [default /tmp]" << std::endl;
  std::cout << "  -f config   ascii file containing observation configuration" << std::endl;
  std::cout << "  -h          print this help text" << std::endl;
  std::cout << "  -t timeout  wait for the specified number of seconds for exiting" << std::endl;
  std::cout << "  -v          verbose output" << std::endl;
}

void signal_handler(int signal_value)
{
  SPDLOG_INFO("received signal {}", signal_value);
  if (signal_received)
  {
    SPDLOG_WARN("received signal {} twice, exiting", signal_value);
    exit(EXIT_FAILURE);
  }
  signal_received = true;
}

auto scalar_stats_different(
  const ska::pst::stat::StatStorage::scalar_stats_t& a,
  const ska::pst::stat::StatStorage::scalar_stats_t& b) -> bool
{
  bool same = (
    (a.mean_frequency_avg == b.mean_frequency_avg) &&
    (a.variance_frequency_avg == b.variance_frequency_avg) &&
    (a.num_clipped_samples == b.num_clipped_samples)
  );
  return (!same);
}
