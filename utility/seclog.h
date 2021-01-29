#pragma once

#include "glog/logging.h"
#include "glog/log_severity.h"

namespace secsdk
{
    typedef enum {
        INFO = google::GLOG_INFO,
        WARNING = google::GLOG_WARNING,
        DETAIL = google::GLOG_DETAIL,
        ERROR = google::GLOG_ERROR,
        FATAL = google::GLOG_FATAL
    } SDKLOGLEVEL;
}

#define SECLOG(severity) google::LogMessage(__FILE__, __FUNCTION__, __LINE__, severity).stream()