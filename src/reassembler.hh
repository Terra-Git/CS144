#pragma once

#include "byte_stream.hh"

#include <list>
#include <string>
#include <tuple>

class Reassembler
{
public:
  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring, Writer& output );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const noexcept;

private:
  // 将数据推入字节流
  void push_data_to_stream( std::string data, Writer& output ) noexcept;
  // 暂存数据
  void store_data( std::string data, uint64_t begin, uint64_t end ) noexcept;
  // 将暂存的数据推入字节流
  void push_store_data_to_stream( Writer& output ) noexcept;

  uint64_t store_data_size_ {};   // 暂存的数据大小
  uint64_t next_stream_index_ {}; // 下一个需要的字节下标
  using DataNode = std::tuple<uint64_t, uint64_t, std::string>;
  std::list<DataNode> store_buffer_ {};
  bool had_last_ {};
};
