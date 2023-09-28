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


#include "ska/pst/stat/testutils/DataBlockTestHelper.h"

#include <iostream>
#include <spdlog/spdlog.h>

namespace ska::pst::smrb::test {

DataBlockTestHelper::DataBlockTestHelper(const std::string& _id, unsigned _num_readers)
{
  id = _id;
  num_readers = _num_readers;
  counter = 0;
}

void DataBlockTestHelper::set_header_block_nbufs (uint64_t nbufs)
{
  if (db)
    throw std::runtime_error("DataBlockTestHelper::set_header_block_nbufs ring buffer already created");

  hdr_nbufs = nbufs;
}

void DataBlockTestHelper::set_header_block_bufsz (uint64_t bufsz)
{
  if (db)
    throw std::runtime_error("DataBlockTestHelper::set_header_block_bufsz ring buffer already created");

  hdr_bufsz = bufsz;
}

void DataBlockTestHelper::set_data_block_nbufs (uint64_t nbufs)
{
  if (db)
    throw std::runtime_error("DataBlockTestHelper::set_data_block_nbufs ring buffer already created");

  dat_nbufs = nbufs;
}

void DataBlockTestHelper::set_data_block_bufsz (uint64_t bufsz)
{
  if (db)
    throw std::runtime_error("DataBlockTestHelper::set_data_block_bufsz ring buffer already created");

  dat_bufsz = bufsz;
}

void DataBlockTestHelper::setup()
{
  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::setup construct DataBlockCreate with id='{}'", id);
  db = std::make_shared<DataBlockCreate>(id);

  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::setup call DataBlockCreate::create");
  db->create(hdr_nbufs, hdr_bufsz, dat_nbufs, dat_bufsz, num_readers, device_id);

  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::setup construct DataBlockWrite with id='{}'", id);
  writer = std::make_shared<DataBlockWrite>(id);

  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::setup call DataBlockWrite::connect");
  writer->connect(1);

  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::setup call DataBlockWrite::lock");
  writer->lock();

  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::setup return");
}

void DataBlockTestHelper::enable_reader()
{
  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::enable reader");

  if (reader)
    throw std::runtime_error("DataBlockTestHelper::enable_reader already enabled");

  if (!db)
    throw std::runtime_error("DataBlockTestHelper::enable_reader ring buffer not created (call setup first)");

  if (num_readers == 0)
    throw std::runtime_error("DataBlockTestHelper::enable_reader ring buffer configured with zero readers");

  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::setup construct DataBlockRead with id='{}'", id);
  reader = std::make_shared<DataBlockRead>(id);

  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::setup call DataBlockRead::connect");
  reader->connect(1);

  SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::setup call DataBlockRead::lock");
  reader->lock();
}

template<typename Accessor>
void teardown_one (Accessor& accessor)
{
  if (accessor)
  {
    if (accessor->get_opened())
    {
      accessor->close();
    }
    if (accessor->get_locked())
    {
      accessor->unlock();
    }
    accessor->disconnect();
  }
  accessor = nullptr;
}

void DataBlockTestHelper::teardown()
{
  teardown_one (writer);
  teardown_one (reader);

  if (db)
  {
    db->destroy();
  }
  db = nullptr;
}

void DataBlockTestHelper::set_config (const ska::pst::common::AsciiHeader& hdr)
{
  config = hdr;
}

void DataBlockTestHelper::set_header (const ska::pst::common::AsciiHeader& hdr)
{
  header = hdr;
}

void DataBlockTestHelper::start()
{
  writer->write_config(config.raw());
  writer->write_header(header.raw());
  writer->open();

  SPDLOG_DEBUG("DataBlockTestHelper::start prime the pump with {} data bytes", dat_bufsz);
  std::vector<char> data(dat_bufsz, 0);
  writer->write_data(&data[0], dat_bufsz);

  if (reader)
  {
    reader->read_config();
    reader->read_header();
    reader->open();
  }
}

void DataBlockTestHelper::write_and_close(size_t nblocks, float delay_ms)
{
  write(nblocks,delay_ms);
  writer->close();
  writer->unlock();
}

void DataBlockTestHelper::write(size_t nblocks, float delay_ms)
{
  static constexpr int microseconds_per_millisecond = 1000;
  int delay_us = delay_ms * microseconds_per_millisecond;

  std::vector<char> data(dat_bufsz, 0);
  char* ptr = &data[0];
  uint64_t* count_ptr = reinterpret_cast<uint64_t*>(ptr);

  for (unsigned iblock=0; iblock < nblocks; iblock++)
  {
    *count_ptr = counter;
    counter ++;

    SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::write write {} data bytes", dat_bufsz);

    writer->write_data(ptr, dat_bufsz);
    SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::write {} data bytes written", dat_bufsz);

    if (delay_us)
      usleep(delay_us);

    if (reader)
    {
      SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::write reader open_block");
      reader->open_block();
      SPDLOG_DEBUG("ska::pst::smrb::test::DataBlockTestHelper::write reader close_block");
      reader->close_block(dat_bufsz);
    }
  }
}

} // namespace ska::pst::smrb::test
