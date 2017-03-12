#pragma once

// necessary when linking the boost_lo library dynamically
//#define BOOST_LOG_DYN_LINK 1
//
#include <boost/log/trivial.hpp>
//#include <boost/log/sources/global_logger_storage.hpp>
//
//#define LOGFILE "logfile.log"
//#define SEVERITY_THRESHOLD logging::trivial::warning
//
//BOOST_LOG_GLOBAL_LOGGER(logger, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>)
//
//#define LOG(severity) BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity)
//
//#define LOG_TRACE     LOG(trace)
//#define LOG_DEBUG     LOG(debug)
//#define LOG_INFO      LOG(info)
//#define LOG_WARNING   LOG(warning)
//#define LOG_ERROR     LOG(error)
//#define LOG_FATAL     LOG(fatal)

#define LOG(severity) BOOST_LOG_TRIVIAL(severity)
