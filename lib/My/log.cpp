#include "log.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace My::log {

namespace bl = boost::log;

void
format(const bl::record_view& rec, bl::formatting_ostream& strm)
{
  strm << bl::extract<unsigned int>("LineID", rec);
  strm << " [";
  strm << boost::posix_time::to_iso_extended_string(
    bl::extract<boost::posix_time::ptime>("TimeStamp", rec).get());
  strm << ' ';

  const char* level;
  if (auto p = bl::extract<Level>("Severity", rec).get_ptr()) {
    switch (*p) {
      case verb:
        level = "v";
        break;
      case info:
        level = "i";
        break;
      case noti:
        level = "n";
        break;
      case warn:
        level = "w";
        break;
      case crit:
        level = "c";
        break;
      case fatal:
        level = "f";
        break;
      case debug:
        level = "d";
        break;
      default:
        level = "?";
        break;
    }
  } else {
    level = "~";
  }
  strm << level;

  strm << ' ';

  if (auto p = bl::extract<std::string>("Channel", rec).get_ptr())
    strm << *p;

  strm << "] <";
  strm << bl::extract<bl::attributes::current_process_id::value_type>(
            "ProcessID", rec)
            .get();
  strm << ' ';
  strm << bl::extract<bl::attributes::current_thread_id::value_type>("ThreadID",
                                                                     rec)
            .get();

  if (auto p = bl::extract<const void*>("ObjectID", rec).get_ptr())
    strm << ' ' << *p;

  strm << "> ";
  auto& message = bl::extract<std::string>("Message", rec).get();
  strm << message.size() << '\n';
  strm << message;
  strm << "\n\n";
}

} // namespace My::log
