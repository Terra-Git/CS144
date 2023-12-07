#include "reassembler.hh"

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
  uint64_t data_last_index = first_index + data.size();
  // 允许写入的最高数据尾号
  uint64_t available_end_index = next_byte_index_ + output.available_capacity();
  // 已经被写入的数据 或者 无容量写入新数据的情况
  if( data_last_index < next_byte_index_ ||  available_end_index <= first_index ){
    return ;
  }
  if( data_last_index > available_end_index ){
    data_last_index = available_end_index;
    data.resize( data_last_index - first_index );
    // 丢弃了部分数据，防止如果带了结束符的数据导致后续无法接收被丢弃的数据
    is_last_substring = false;
  }
  // 判断能写入多少
  if( first_index < next_byte_index_ ){
    data = data.substr(next_byte_index_ - first_index);
    first_index = next_byte_index_;
  }

}

uint64_t Reassembler::bytes_pending() const noexcept
{
  return store_data_size_;
}
