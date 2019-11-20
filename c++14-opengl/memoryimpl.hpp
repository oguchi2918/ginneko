#ifndef INCLUDED_MEMORY_IMPL_HPP
#define INCLUDED_MEMORY_IMPL_HPP

#include <cassert>

// This code is for 64 bit only now... (cause there is no 32bit test environment)

namespace nekolib {
  namespace memory {
    class StackAllocator
    {
    private:
      void* bottom_;
      unsigned stack_size_;
      unsigned current_;
      
      // alignment must be 2 pow (4 or 16)
      unsigned long long alloc_aligned(unsigned size_bytes, unsigned alignment) noexcept {
	assert(alignment == 4 || alignment == 16);

	unsigned long long raw_address = alloc_unaligned(size_bytes + alignment);
	unsigned long long aligned_address = (raw_address + alignment - 1) & ~(alignment - 1);

	// write adjuscent
	// unsigned* p = (unsigned*)(aligned_address - 4);
	// *p = adjuscent;

	return aligned_address;
      }

    public:
      explicit StackAllocator() noexcept : bottom_(0), stack_size_(0), current_(0) {}
      StackAllocator(void* bottom, unsigned stack_size) noexcept
	: bottom_(bottom), stack_size_(stack_size), current_(0) {
      }
      unsigned long long alloc_unaligned(unsigned size_bytes) noexcept {
	unsigned old = current_;
	current_ += size_bytes;
	assert(current_ < stack_size_);

	return (unsigned long long)bottom_ + old;
      }
      
      ~StackAllocator() = default;
      
      void* alloc(unsigned size_bytes) noexcept {
	return (void*)alloc_aligned(size_bytes, 4);
      }
      unsigned current() const noexcept { return current_; }
      void rollback(unsigned marker) noexcept { current_ = marker; }
      void clear() noexcept { current_ = 0; }
    };

    class Manager::Impl {
    private:
      StackAllocator fsas_[2]; // StackAllocators swapped per frame
      unsigned cur_;   // 0 or 1
    public:
      Impl(unsigned char* bottom, unsigned size) noexcept : cur_(1) {
	for (int i = 0; i < 2; ++i) {
	  fsas_[i] = StackAllocator(bottom + i * size, size);
	  fsas_[i].clear();
	}
      }
      ~Impl() noexcept {
	fsas_[0].clear();
	fsas_[1].clear();
      }
      void update() noexcept {
	cur_ = (1 - cur_);  // swap 0 <-> 1
	fsas_[cur_].clear();
      }
      
      void* alloc(unsigned size_bytes) noexcept {
	return fsas_[cur_].alloc(size_bytes);
      }
    };
  }
}

#endif // INCLUDED_MEMORY_IMPL_HPP
