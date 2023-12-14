#include "reassembler.hh"

#include <algorithm>
#include <ranges>
#include <iostream>

using namespace std;

/**
 * 1. 数据可以直接写入stream， 可能部分超出，需要做截断
 * 2. 数据无法缓存，数据的index 超出了能写入的最大index
 * 3. 数据可以缓存，1）数据可以完全缓存  2）数据超出容量需要做截断  3）数据存在重复，需要合并缓存数据
 * 4. 数据为空
 * 5. 是否最后的数据，1） 可以全部缓存或写入  2）只能缓存或插入部分
 */
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  if ( data.empty() ) {
    if ( is_last_substring ) {
      output.close();
    }
    return;
  }
  auto data_left = first_index, data_right = first_index + data.size();     // [left,right)
  auto enable_end_index = next_stream_index_ + output.available_capacity(); // [left,right)
  // 没有可用容量
  if ( data_right < next_stream_index_ || enable_end_index <= first_index ) {
    return;
  }

  // 下面两个判断保证了，暂存的数据不会超过 stream 的可用容量，如果 store 中的数据满足条件，可以立即推入 stream 中
  // 尾巴超过可用容量
  if ( enable_end_index < data_right ) {
    data_right = enable_end_index;
    data.resize( enable_end_index - data_left );
    is_last_substring = false;
  }
  // 头有重复
  if( data_left < next_stream_index_){
    data = data.substr(next_stream_index_ - first_index );
    data_left = next_stream_index_;
  }

  // 判断数据是否可以直接写入 stream
  if ( data_left == next_stream_index_
       && ( store_buffer_.empty() || data_right < get<1>( store_buffer_.front() ) ) ) {
    if ( !store_buffer_.empty() ) {
      data.resize( min( data_right, get<0>( store_buffer_.front() ) ) - data_left );
    }
    push_data_to_stream( std::move( data ), output );
  } else {
    store_data( std::move( data ), data_left, data_right - 1 );
  }
  had_last_ |= is_last_substring;
  push_store_data_to_stream( output );
  return;
}

// 将数据推入字节流
void Reassembler::push_data_to_stream( std::string data, Writer& output ) noexcept
{
  next_stream_index_ += data.size();
  output.push( std::move( data ) );
}

// 暂存数据
void Reassembler::store_data( std::string data, uint64_t begin, uint64_t end ) noexcept
{
  auto data_left = begin, data_right = end;
  auto store_left = store_buffer_.begin(), store_right = store_buffer_.end();
  auto left = std::lower_bound( store_left, store_right, data_left, []( auto& node, auto& left_index ) {return get<1>( node ) < left_index;} );
  auto right = std::upper_bound( left, store_right, data_right, []( auto& right_index, auto& node ) { return get<0>( node ) > right_index; } );
  if ( left != store_right )  {data_left = min( data_left, get<0>( *left ) );}
  if ( right != left ) {data_right = max( data_right, get<1>( *prev(right) ) );}

  store_data_size_ += data_right - data_left + 1;
  if ( data.size() == data_right - data_left + 1 && left == right ) {
    store_buffer_.emplace( left, data_left, data_right, std::move( data ) );
    return;
  }
  std::string temp_s( data_right - data_left + 1, 0 );

  for ( auto&& node : std::views::iota( left, right ) ) {
    auto& [l, r, s] = *node;
    store_data_size_ -= s.size();
    std::ranges::copy( s, temp_s.begin() + l - data_left );
  }
  std::ranges::copy( data, temp_s.begin() + begin - data_left );
  store_buffer_.emplace( store_buffer_.erase( left, right ), data_left, data_right, std::move( temp_s ) );
}

// 将暂存的数据推入字节流
void Reassembler::push_store_data_to_stream( Writer& output ) noexcept
{
  while ( !store_buffer_.empty() && get<0>( store_buffer_.front() ) == next_stream_index_ ) {
    store_data_size_ -= get<2>( store_buffer_.front() ).size();
    push_data_to_stream( std::move( get<2>( store_buffer_.front() ) ), output );
    store_buffer_.pop_front();
  }
  if ( had_last_ && store_buffer_.empty() ) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const noexcept
{
  return store_data_size_;
}
