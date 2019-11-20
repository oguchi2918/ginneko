#ifndef INCLUDED_MEMORY_HPP
#define INCLUDED_MEMORY_HPP

// 毎Frame内容が破棄されるテンポラリメモリアロケーター
// 現状では前Frameの入力記憶用にnekolib::inputが利用している
namespace nekolib {
  namespace memory {
    class Manager
    {
    public:
      static void init();
      static void update() noexcept;
      static void* alloc(unsigned size_bytes) noexcept;
      class Impl;

      Manager() = delete;
      ~Manager() = delete;
    };
  }
}

#endif // INCLUDED_MEMORY_HPP
