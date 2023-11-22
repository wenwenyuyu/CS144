#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  // 判断是否可以插入或保存
  if(data.size() == 0 && is_last_substring){
    output.close();
    return;
  }

  if(output.available_capacity() == 0){
    return;
  }

  first_unacceptable_index_ = first_unassembled_index_ + output.available_capacity();
  uint64_t end_index = first_index + data.size();
  if(first_index >= first_unacceptable_index_ || end_index <= first_unassembled_index_){
    return;
  }
  //现在可以保存了，但要判断是否有交叉状况
  if(end_index > first_unacceptable_index_){
    data = data.substr(0, first_unacceptable_index_ - first_index);
    is_last_substring = false;
  }

  if(is_last_substring){
    is_last_ = true;
    last_substring_index = end_index;
  }
  if(first_index > first_unassembled_index_){
    //存进map中
    insert_into_map(first_index, std::move(data));
  } else {
    //这种状况是可以更新了
    data = data.substr(first_unassembled_index_ - first_index, end_index - first_unassembled_index_);
    first_unassembled_index_ += data.size();
    output.push(std::move(data));
    update_buffer(output);
  }

  if(is_last_ && first_unassembled_index_ >= last_substring_index){
    output.close();
  }

  return;  
}

// 区间合并
void Reassembler::insert_into_map(uint64_t first_index, string data){
  uint64_t begin_index = first_index;
  uint64_t end_index = first_index + data.size();
  for(auto it = buffer_.begin(); it != buffer_.end() && begin_index < end_index; ){
    auto &[index, value] = *it;
    auto last_index = index + value.size();
    if(begin_index >= index){
      begin_index = max(begin_index, last_index);
      ++it;
      continue;
    }
    auto right_index = min(end_index, index);
    unassembled_bytes_ += right_index - begin_index;
    buffer_.emplace_hint(it, begin_index, data.substr(begin_index - first_index, right_index - begin_index));
    begin_index = right_index;
  }

  if(begin_index < end_index){
    unassembled_bytes_ += end_index - begin_index;
    buffer_.emplace(begin_index, data.substr(begin_index - first_index));
  }
}

void Reassembler::update_buffer(Writer& output){
  if(buffer_.empty()){
    return;
  }
  for(auto it = buffer_.begin(); it != buffer_.end();){
    auto &[index, data] = *it;
    if(index > first_unassembled_index_){
      break;
    }
    auto end = index + data.size();      
    unassembled_bytes_ -= data.size();
    if(end > first_unassembled_index_){
      data = data.substr(first_unassembled_index_ - index, end - first_unassembled_index_);
      first_unassembled_index_ += data.size();
      output.push(std::move(data));
    }
    it = buffer_.erase(it);
  }

  if(buffer_.empty() && is_last_){
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return unassembled_bytes_;
}
