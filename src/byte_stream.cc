#include <iostream>
#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

// 这里是值传递，data可以move操作
void Writer::push( string data ) noexcept
{
  if ( is_closed() ) {
    return;
  }
  auto size = min( available_capacity(), data.size() );

  if ( 0 == size ) {
    return;
  } else if ( size < data.size() ) {
    data.resize( size );
  }

  buffer_.push( std::move( data ) );
  if ( 1 == buffer_.size() ) {
    buffer_view_ = buffer_.front();
  }
  bytes_push_size_ += size;
  return;
}

void Writer::close() noexcept
{
  stream_state_ |= ( 1 << StreamState::CLOSE );
}

void Writer::set_error() noexcept
{
  stream_state_ |= ( 1 << StreamState::ERROR );
}

bool Writer::is_closed() const noexcept
{
  return stream_state_ & ( 1 << StreamState::CLOSE );
}

uint64_t Writer::available_capacity() const noexcept
{
  return ( capacity_ - reader().bytes_buffered() );
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
  return ( writer().is_closed() && 0 == bytes_buffered() );
}

bool Reader::has_error() const noexcept
{
  return stream_state_ & ( 1 << StreamState::ERROR );
}

void Reader::pop( uint64_t len ) noexcept
{
  if ( len > bytes_buffered() ) {
    return;
  }

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
  return writer().bytes_pushed() - bytes_popped();
}

uint64_t Reader::bytes_popped() const noexcept
{
  return bytes_pop_size_;
}
