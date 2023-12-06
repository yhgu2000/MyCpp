#pragma once

#include "err.hpp"
#include "util.hpp"

#ifdef _WIN32

#include <io.h>
#else

#include <sys/io.h>

int
ftruncate(int fildes, off_t length);

#endif

#include <sys/stat.h>

namespace My {

using util::Bytes;

/**
 * @brief 包装自 std::FILE*、跨平台的 64 位二进制文件读写类。
 *
 * 其表现为平凡的指针，不保有文件资源的所有权，也不会在析构时关闭文件。
 */
class CFile64
{
public:
  class Seeker;
  class Closer;
  class Closers;

public:
  /**
   * @brief 加载文件内容到 std::string。
   */
  static std::string load_s(const char* path) noexcept(false);

  /**
   * @brief 保存 std::string。
   */
  static void save_s(const char* path, const std::string& data) noexcept(false);

  /**
   * @brief 加载文件内容到 Bytes (std::vector<std::uint8_t>)。
   */
  static Bytes load_b(const char* path) noexcept(false);

  /**
   * @brief 保存 Bytes (std::vector<std::uint8_t>)。
   */
  static void save_b(const char* path, const Bytes& data) noexcept(false);

public:
  std::FILE* mPtr;

  CFile64() noexcept = default;

  CFile64(std::nullptr_t) noexcept
    : CFile64()
  {
  }

  /**
   * @param ptr 文件指针，借用语义
   */
  CFile64(std::FILE* ptr) noexcept
    : mPtr(ptr)
  {
  }

  /**
   * @param path 文件路径
   * @param mode 打开模式
   */
  CFile64(const char* path, const char* mode)
    : mPtr(std::fopen(path, mode))
  {
    if (mPtr == nullptr)
      throw err::Errno(errno);
  }

  /**
   * @brief 检查文件指针是否有效。
   */
  operator bool() const noexcept { return mPtr != nullptr; }

  operator std::FILE*() const noexcept { return mPtr; }

public:
  /**
   * @brief 显式转换为 std::FILE* 。
   */
  std::FILE* operator+() const noexcept { return mPtr; }

  template<typename T, typename = std::enable_if_t<std::is_trivial_v<T>>>
  const CFile64& operator<<(const T& t) const noexcept(false)
  {
    write(&t, sizeof(T), 1);
    return *this;
  }

  template<typename T, typename = std::enable_if_t<std::is_trivial_v<T>>>
  const CFile64& operator>>(T& t) const noexcept(false)
  {
    read(&t, sizeof(T), 1);
    return *this;
  }

  template<typename T, typename = std::enable_if_t<std::is_trivial_v<T>>>
  const CFile64& operator<<(const std::vector<T>& vec) const noexcept(false)
  {
    *this << vec.size();
    write(vec.data(), sizeof(T), vec.size());
    return *this;
  }

  template<typename T, typename = std::enable_if_t<std::is_trivial_v<T>>>
  const CFile64& operator>>(std::vector<T>& vec) const noexcept(false)
  {
    std::size_t size;
    *this >> size;
    vec.resize(size);
    read(vec.data(), sizeof(T), vec.size());
    return *this;
  }

public:
  void reopen(const char* filename, const char* mode) const noexcept(false)
  {
    if (freopen(filename, mode, mPtr) == nullptr)
      throw err::Errno(errno);
  }

  void close() const noexcept(false)
  {
    if (std::fclose(mPtr))
      throw err::Errno(errno);
  }

  void read(void* buffer, std::int64_t size, std::int64_t count) const
    noexcept(false)
  {
    auto n = std::fread(buffer, size, count, mPtr);
    if (n != count && size != 0)
      throw err::Errno(std::ferror(mPtr));
  }

  /**
   * @brief 从文件地址空间的指定偏移 addr 读取数据。
   */
  void read(void* buffer,
            std::int64_t size,
            std::int64_t count,
            std::int64_t addr) const noexcept(false);

  void write(const void* buffer, std::int64_t size, std::int64_t count) const
    noexcept(false)
  {
    if (std::fwrite(buffer, size, count, mPtr) != count && size != 0)
      throw err::Errno(std::ferror(mPtr));
  }

  /**
   * @brief 将数据写入文件地址空间的指定偏移 addr。
   */
  void write(const void* buffer,
             std::int64_t size,
             std::int64_t count,
             std::int64_t addr) const noexcept(false);

  void flush() const noexcept(false)
  {
    if (std::fflush(mPtr))
      throw err::Errno(std::ferror(mPtr));
  }

  void seek(std::int64_t offset, int origin) const noexcept(false)
  {
    if (
#ifdef _WIN32
      _fseeki64(mPtr, offset, origin)
#else
      assert(sizeof(long) == 8); std::fseek(mPtr, offset, origin)
#endif
    )
      throw err::Errno(std::ferror(mPtr));
  }

  std::int64_t tell() const noexcept(false)
  {
#ifdef _WIN32
    auto ret = _ftelli64(mPtr);
#else
    assert(sizeof(long) == 8);
    auto ret = std::ftell(mPtr);
#endif
    if (ret == -1)
      throw err::Errno(errno);
    return ret;
  }

  void rewind() const noexcept { std::rewind(mPtr); }

  /**
   * @brief 截断或扩展文件大小到 size。
   */
  void trunc(std::int64_t size) const noexcept(false)
  {
#ifdef _WIN32
    if (auto err = _chsize_s(fileno(mPtr), size))
#else
    assert(sizeof(long) == 8);
    if (auto err = ftruncate(fileno(mPtr), size))
#endif
      throw err::Errno(err);
  }

  /**
   * @brief 获取文件大小。
   */
  std::int64_t size() const noexcept(false)
  {
#ifdef _WIN32
    struct _stat64 st;
    if (_fstat64(fileno(mPtr), &st))
#else
    assert(sizeof(long) == 8);
    struct stat st;
    if (fstat(fileno(mPtr), &st))
#endif
      throw err::Errno(errno);
    return st.st_size;
  }

  /**
   * @brief 读取所有剩下的数据到 std::string。
   */
  std::string rest_s() const noexcept(false);

  /**
   * @brief 读取所有剩下的数据到  Bytes (std::vector<std::uint8_t>)。
   */
  Bytes rest_b() const noexcept(false);
};

class CFile64::Seeker
{
public:
  Seeker(const Seeker&) = delete;
  Seeker(Seeker&&) = delete;
  Seeker& operator=(const Seeker&) = delete;
  Seeker operator=(Seeker&&) = delete;

public:
  const CFile64& _;
  std::size_t mPos;

public:
  Seeker(const CFile64& _)
    : _(_)
    , mPos(_.tell())
  {
  }

  Seeker(const CFile64& _, std::int64_t offset, int origin)
    : Seeker(_)
  {
    _.seek(offset, origin);
  }

  ~Seeker() noexcept { std::fseek(_, mPos, SEEK_SET); }
};

class CFile64::Closer
{
public:
  Closer(const Closer&) = delete;
  Closer(Closer&&) = delete;
  Closer& operator=(const Closer&) = delete;
  Closer operator=(Closer&&) = delete;

public:
  const CFile64& _;

public:
  Closer(const CFile64& _)
    : _(_)
  {
  }

  ~Closer() noexcept { std::fclose(_); }
};

class CFile64::Closers : public std::vector<CFile64>
{
public:
  using std::vector<CFile64>::vector;

public:
  ~Closers() noexcept
  {
    for (auto i : *this)
      if (i)
        std::fclose(i);
  }
};

template<typename T, typename = std::enable_if_t<!std::is_pod_v<T>>>
const CFile64&
operator<<(const CFile64& f, const std::vector<T>& vec) noexcept(false)
{
  f << vec.size();
  for (auto&& i : vec)
    f << i;
  return f;
}

template<typename T, typename = std::enable_if_t<!std::is_pod_v<T>>>
const CFile64&
operator>>(const CFile64& f, std::vector<T>& vec) noexcept(false)
{
  std::size_t size;
  f >> size;
  vec.resize(size);
  for (auto&& i : vec)
    f >> i;
  return f;
}

} // namespace My

inline void
My::CFile64::read(void* buffer,
                  std::int64_t size,
                  std::int64_t count,
                  std::int64_t addr) const noexcept(false)
{
  Seeker seeker(*this, addr, SEEK_SET);
  read(buffer, size, count);
}

inline void
My::CFile64::write(const void* buffer,
                   std::int64_t size,
                   std::int64_t count,
                   std::int64_t addr) const noexcept(false)
{
  Seeker seeker(*this, addr, SEEK_SET);
  write(buffer, size, count);
}

inline void
My::CFile64::save_s(const char* path, const std::string& data) noexcept(false)
{
  CFile64 file(path, "wb");
  file.write(data.data(), data.size(), 1);
}

inline void
My::CFile64::save_b(const char* path, const Bytes& data) noexcept(false)
{
  CFile64 file(path, "wb");
  file.write(data.data(), data.size(), 1);
}
