#include "HttpMatpowsum.hpp"

#include <My/util.hpp>
#include <boost/url/parse.hpp>

using namespace My::util;

namespace {

class Mat
{
public:
  Mat(std::uint32_t rank)
    : mRank(rank)
    , mData(new double[rank * rank])
  {
  }

  std::uint32_t rank() const { return mRank; }

  double* data() const { return mData.get(); }

  Mat& set(double v)
  {
    for (std::uint32_t i = 0; i < mRank * mRank; ++i)
      mData[i] = v;
    return *this;
  }

  Mat mul(const Mat& other) const
  {
    if (mRank != other.mRank)
      throw std::runtime_error("Matrix ranks do not match");

    Mat ret(mRank);
    for (std::uint32_t i = 0; i < mRank; ++i) {
      for (std::uint32_t j = 0; j < mRank; ++j) {
        ret.mData[i * mRank + j] = 0;
        for (std::uint32_t k = 0; k < mRank; ++k)
          ret.mData[i * mRank + j] +=
            mData[i * mRank + k] * other.mData[k * mRank + j];
      }
    }

    return ret;
  }

  Mat pow(std::uint32_t n) const
  {
    Mat ret(mRank);
    for (std::uint32_t i = 0; i < mRank * mRank; ++i)
      ret.mData[i] = 0;
    for (std::uint32_t i = 0; i < mRank; ++i)
      ret.mData[i * mRank + i] = 1;

    for (std::uint32_t i = 0; i < n; ++i)
      ret = ret.mul(*this);

    return ret;
  }

  double sum() const
  {
    double s = 0;
    for (std::uint32_t i = 0; i < mRank * mRank; ++i)
      s += mData[i];
    return s;
  }

private:
  std::uint32_t mRank;
  std::unique_ptr<double[]> mData;
};

double
matpowsum(std::uint32_t k, std::uint32_t n)
{
  return Mat(k).set(1.0 / k).pow(n).sum();
}

} // namespace

namespace MyHttp {

void
HttpMatpowsum::do_handle() noexcept
{
  mResponse.set(http::field::content_type, "text/plain");

  auto r = boost::urls::parse_uri(mRequest.target());
  if (!r) {
    mResponse = { http::status::bad_request, 11 };
    mResponse.body() = "Invalid URI"_b;
    on_handle(nullptr);
    return;
  }

  auto params = r->params();
  auto kIt = params.find("k");
  auto nIt = params.find("n");
  if (kIt == params.end() || nIt == params.end()) {
    mResponse = { http::status::bad_request, 11 };
    mResponse.body() = "Missing parameter 'k' or 'n'"_b;
    on_handle(nullptr);
    return;
  }

  auto k = std::stoul((*kIt).value);
  auto n = std::stoul((*nIt).value);
  auto ans = matpowsum(k, n);
  mResponse.result(http::status::ok);
  mResponse.body() =
    to_bytes("matpowsum(k=" + std::to_string(k) + ", n=" + std::to_string(n) +
             ") = " + std::to_string(ans));
  on_handle(nullptr);
}

void
HttpMatpowsum::Server::come(Socket&& sock)
{
  std::make_shared<HttpMatpowsum>(std::move(sock), mConfig)->start();
}

} // namespace MyHttp
