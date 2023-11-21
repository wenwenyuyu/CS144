#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ){}

void Writer::push( string data )
{
  // Your code here.
  if(available_capacity() == 0 || data.empty())
    return;
  
  uint64_t data_len = min(available_capacity(), data.size());
  if(data_len < data.size())
    data = data.substr(0, data_len);
  
  stream_.push_back(std::move(data));
  stream_view_.emplace_back(stream_.back().c_str(), data_len);
  has_pushed_ += data_len;
  data_buffer_ += data_len;
  return;
}

void Writer::close()
{
  // Your code here.
  is_closed_ = true;
}

void Writer::set_error()
{
  // Your code here.
  is_error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return is_closed_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - data_buffer_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return has_pushed_;
}

string_view Reader::peek() const
{
  // Your code here.
  if(stream_view_.empty())
    return {};
  return stream_view_.front();
}

bool Reader::is_finished() const
{
  // Your code here.
  return is_closed_ && data_buffer_ == 0;
}

bool Reader::has_error() const
{
  // Your code here.
  return is_error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t n = min(len, data_buffer_);
  while(n > 0){
    auto sz = stream_view_.front().size();
    if(n < sz){
      stream_view_.front().remove_prefix(n);
      data_buffer_ -= n;
      has_poped_ += n;
      return;
    }
    stream_.pop_front();
    stream_view_.pop_front();
    n -= sz;
    data_buffer_ -= sz;
    has_poped_ += sz;
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return data_buffer_;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return has_poped_;
}
