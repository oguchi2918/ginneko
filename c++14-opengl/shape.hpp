#ifndef INCLUDED_SHAPE_HPP
#define INCLUDED_SHAPE_HPP

namespace nekolib {
  namespace renderer {
    
    // 原点中心、xy平面上の辺長2.0の正方形
    class Quad {
    public:
      static Quad create(float, float, float, float, bool has_norm = false);
      void render() const;
      
      // 以下、参照回数計測クラスのテンプレート関数
      Quad() noexcept;
      ~Quad() noexcept;
      Quad(const Quad&) noexcept;
      Quad(Quad&&) noexcept;
      void release() noexcept;
      Quad& operator=(const Quad&) noexcept;
      Quad& operator=(Quad&&) noexcept;
      bool operator==(const Quad&) const noexcept;
      bool operator!=(const Quad&) const noexcept;
      explicit operator bool() const noexcept;
      
    private:
      class Impl;
      Impl* impl_;
    };

    // 原点中心、辺長1.0の立方体
    class Cube {
    public:
      static Cube create(bool has_norm = true);
      void render() const;
      
      // 以下、参照回数計測クラスのテンプレート関数
      Cube() noexcept;
      ~Cube() noexcept;
      Cube(const Cube&) noexcept;
      Cube(Cube&&) noexcept;
      void release() noexcept;
      Cube& operator=(const Cube&) noexcept;
      Cube& operator=(Cube&&) noexcept;
      bool operator==(const Cube&) const noexcept;
      bool operator!=(const Cube&) const noexcept;
      explicit operator bool() const noexcept;
      
    private:
      class Impl;
      Impl* impl_;
    };

    // 原点中心、辺長1.0の立方体(ワイヤーフレーム)
    class WireCube {
    public:
      static WireCube create();
      void render() const;
      
      // 以下、参照回数計測クラスのテンプレート関数
      WireCube() noexcept;
      ~WireCube() noexcept;
      WireCube(const WireCube&) noexcept;
      WireCube(WireCube&&) noexcept;
      void release() noexcept;
      WireCube& operator=(const WireCube&) noexcept;
      WireCube& operator=(WireCube&&) noexcept;
      bool operator==(const WireCube&) const noexcept;
      bool operator!=(const WireCube&) const noexcept;
      explicit operator bool() const noexcept;
      
    private:
      class Impl;
      Impl* impl_;
    };

    // トーラス
    class Torus {
    public:
      static Torus create(float, float, unsigned, unsigned);
      void render() const;

      // 以下、参照回数計測クラスのテンプレート関数
      Torus() noexcept;
      ~Torus() noexcept;
      Torus(const Torus&) noexcept;
      Torus(Torus&&) noexcept;
      void release() noexcept;
      Torus& operator=(const Torus&) noexcept;
      Torus& operator=(Torus&&) noexcept;
      bool operator==(const Torus&) const noexcept;
      bool operator!=(const Torus&) const noexcept;
      explicit operator bool() const noexcept;

    private:
      class Impl;
      Impl* impl_;
    };
    
    // 原点中心、半径1.0の球
    class Sphere {
    public:
      static Sphere create(unsigned slices = 40, unsigned stacks = 20);
      void render() const;
      
      // 以下、参照回数計測クラスのテンプレート関数
      Sphere() noexcept;
      ~Sphere() noexcept;
      Sphere(const Sphere&) noexcept;
      Sphere(Sphere&&) noexcept;
      void release() noexcept;
      Sphere& operator=(const Sphere&) noexcept;
      Sphere& operator=(Sphere&&) noexcept;
      bool operator==(const Sphere&) const noexcept;
      bool operator!=(const Sphere&) const noexcept;
      explicit operator bool() const noexcept;
      
    private:
      class Impl;
      Impl* impl_;
    };

    // 原点中心、半径1.0の球(ワイヤーフレーム)
    class WireSphere {
    public:
      static WireSphere create(unsigned slices = 32, unsigned stacks = 16);
      void render() const;
      
      // 以下、参照回数計測クラスのテンプレート関数
      WireSphere() noexcept;
      ~WireSphere() noexcept;
      WireSphere(const WireSphere&) noexcept;
      WireSphere(WireSphere&&) noexcept;
      void release() noexcept;
      WireSphere& operator=(const WireSphere&) noexcept;
      WireSphere& operator=(WireSphere&&) noexcept;
      bool operator==(const WireSphere&) const noexcept;
      bool operator!=(const WireSphere&) const noexcept;
      explicit operator bool() const noexcept;
      
    private:
      class Impl;
      Impl* impl_;
    };
  }
}

#endif // INCLUDED_SHAPE_HPP
