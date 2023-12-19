#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

// 这里是值传递，data可以move操作
void Writer::push( string data ) noexcept
{
  if ( 0 == available_capacity() || data.empty()) {
    return;
  }
  auto size = min( available_capacity(), data.size() );

  if ( size < data.size() ) {
    data.resize( size );
  }

  buffer_.push( std::move( data ) );
  if ( 1 == buffer_.size() ) {
    buffer_view_ = buffer_.front();
  }
  bytes_push_size_ += size;
  bytes_buffed_size_ += size;

  return;
}

void Writer::close() noexcept
{
  is_closed_ = true;
}

void Writer::set_error() noexcept
{
  has_error_ = true;
}

bool Writer::is_closed() const noexcept
{
  return is_closed_;
}

uint64_t Writer::available_capacity() const noexcept
{
  return capacity_ - bytes_buffed_size_;
}

uint64_t Writer::bytes_pushed() const noexcept
{
  return bytes_push_size_;
}

string_view Reader::peek() const noexcept
{
  return buffer_view_;
}

bool Reader::is_finished() const noexcept
{
  return is_closed_ && bytes_buffed_size_ == 0;
}

bool Reader::has_error() const noexcept
{
  return has_error_ ;
}

void Reader::pop( uint64_t len ) noexcept
{
  if ( len > bytes_buffed_size_ ) {
    return;
  }
  bytes_buffed_size_ -= len;
  bytes_pop_size_ += len;

  while ( 0 < len ) {
    if ( buffer_view_.size() <= len ) {
      len -= buffer_view_.size();
      buffer_.pop();
      buffer_view_ = buffer_.front();
    } else {
      buffer_view_.remove_prefix( len );
      len = 0;
    }
  }
  return;
}

uint64_t Reader::bytes_buffered() const noexcept
{
  return bytes_buffed_size_;
}

uint64_t Reader::bytes_popped() const noexcept
{
  return bytes_pop_size_;
}
