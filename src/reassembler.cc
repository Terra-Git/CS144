#include "reassembler.hh"

#include <ranges>
#include <algorithm>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // 如果是空数据，考虑是否带有结束符，没有直接返回，带有尾符，则关闭写入流
  if( data.empty() ){
    if(is_last_substring){
      output.close();
    }
    return ;
  }
  // Your code here.
  // 当前收到数据尾号
  uint64_t data_last_index = first_index + data.size() - 1;
  // 允许写入的最高数据尾号
  uint64_t available_end_index = next_byte_index_ + output.available_capacity() - 1;
  // 已经被写入的数据 或者 无容量写入新数据的情况, 这里提供保证，store中的数据都是可以立马被推入stream中的
  if( data_last_index < next_byte_index_ ||  available_end_index < first_index ){
    return ;
  }
    // 判断能写入多少，头尾都要判断
  if( data_last_index > available_end_index ){
    data_last_index = available_end_index;
    data.resize( data_last_index - first_index );
    // 丢弃了部分数据，防止如果带了结束符的数据导致后续无法接收被丢弃的数据
    is_last_substring = false;
  }
  if( first_index < next_byte_index_ ){
    data = data.substr(next_byte_index_ - first_index);
    first_index = next_byte_index_;
  }
  // 如果收到的头刚好是要推入的下一个数据，则判断，store中是否有数据，没有就直接推； 
  // 有数据，则判断，data_last 是不是超过或等于 store 的头索引，把 data 截取到 store 头索引
  if( first_index == next_byte_index_ && ( store_data_.empty() || data_last_index < get<1>(store_data_.front()) )) {
    if( !store_data_.empty() ){
      data.resize( get<0>(store_data_.front()) - first_index);
    }
    push_data_to_byte_stream(data,output);
  }else{
    push_data_to_store(first_index, data_last_index, data);
  }
  store_finish_ |= is_last_substring;

  push_store_to_byte_stream(output);
  return ;
}

void Reassembler::push_data_to_store(uint64_t first_index, uint64_t last_index, std::string data)
{
  auto begin = store_data_.begin(),end = store_data_.end();
  auto left = first_index,right =last_index;
  auto max_left = lower_bound(begin, end, left, [](auto& node,auto& index){ return get<1>(node) < index; } );
  auto min_righlt = upper_bound(max_left, end, right, [](auto& index, auto& node){ return get<0>(node) > index; } );
  if( max_left != end ) {
    left = min(static_cast<uint64_t>(1), get<0>(*max_left));
  }
  // 这个max操作，获取了右端点，后续可以整节点copy，不用在做 string 的截取
  // 这里考虑到，data 里面，是可能包含部分其它节点的，一段段截取很复杂，取整之后，整段 copy 更好
  if( min_righlt != end ){
    right = max( static_cast<uint64_t>(right), get<1>( *prev(min_righlt)));
  }
  

  
}

void Reassembler::push_data_to_byte_stream(std::string data, Writer& output)
{
  next_byte_index_ += data.size();
  output.push(std::move(data));
}

void Reassembler::push_store_to_byte_stream(Writer& output)
{
  while( !store_data_.empty() && get<0>(store_data_.front()) == next_byte_index_ ){
    auto [first,last,data] = store_data_.front();
    store_data_size_ -= data.size();
    push_data_to_byte_stream( std::move(data),output);
    store_data_.pop_front();
  }

  if( store_data_.empty() && store_finish_){
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const noexcept
{
  return store_data_size_;
}
