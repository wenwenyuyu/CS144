#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + static_cast<uint32_t>(n);
}

// checkpoint表示上一次的绝对索引
// 本次相对索引转化成绝对索引，应该为最近的索引
// 即使相差再大，也不超过2^32 - 1
// 0____________ckpt32____________raw_value______2^32-1
// raw_value可能比checkpoint大dis或者小2^32-dis
// checkpoint可能也比2^32-dis小，那就只能选dis
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  auto const ckpt32 = wrap(checkpoint, zero_point);
  uint32_t dis = raw_value_ - ckpt32.raw_value_;
  if(dis <= (1ul << 31) || checkpoint + dis < (1ul << 32)){
    return checkpoint + dis;
  }
  return checkpoint + dis - (1ul << 32);
}
