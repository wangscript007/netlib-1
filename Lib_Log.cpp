/*
 * Lib_Log.cpp
 *
 *  Created on: Dec 16,2015
 *      Author: zhangyalei
 */

#include <stdarg.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include "Lib_Log.h"
#include "Thread_Mutex.h"
#include "Mutex_Guard.h"

Lib_Log::Lib_Log(void)
: switcher_(F_SYS_ABORT | F_ABORT | F_EXIT | F_SYS | F_USER | F_USER_TRACE | F_DEBUG)
{ }

Lib_Log::~Lib_Log(void) { }

Lib_Log *Lib_Log::instance_ = 0;

int Lib_Log::msg_buf_size = 4096;

int Lib_Log::backtrace_size = 512;

std::string Lib_Log::msg_head[] = {
		"[LOG_SYS_ABORT] ",		/// M_SYS_ABORT 	= 0,
		"[LOG_ABORT] ",			/// M_ABORT 		= 1,
		"[LOG_EXIT] ",			/// M_EXIT 			= 2,
		"[LOG_SYS] ",			/// M_SYS 			= 3,
		"[LOG_USER] ",			/// M_USER			= 4,
		"[LOG_USER_TRACE] ",	/// M_USER_TRACE	= 5
		"[LOG_DEBUG] ",			/// M_DEBUG			= 6,
		"[NULL]"				/// NULL_STUB,
};

std::string Lib_Log::lib_log_dir = "./lib_log";

Lib_Log *Lib_Log::instance(void) {
	if (! instance_)
		instance_ = new Lib_Log;
	return instance_;
}

void Lib_Log::destroy(void) {
	if (instance_) {
		delete instance_;
		instance_ = 0;
	}
}

void Lib_Log::on_flag(int v) {
	switcher_ |= v;
}

void Lib_Log::off_flag(int v) {
	switcher_ &= (~v);
}

void Lib_Log::msg_abort(const char *fmt, ...) {
	if (switcher_ & F_ABORT) {
		va_list	ap;

		va_start(ap, fmt);
		assembly_msg(M_ABORT, fmt, ap);
		va_end(ap);
	}
}

void Lib_Log::msg_sys_abort(const char *fmt, ...) {
	if (switcher_ & F_SYS_ABORT) {
		va_list	ap;

		va_start(ap, fmt);
		assembly_msg(M_SYS_ABORT, fmt, ap);
		va_end(ap);
	}
}

void Lib_Log::msg_exit(const char *fmt, ...) {
	if (switcher_ & F_EXIT) {
		va_list	ap;

		va_start(ap, fmt);
		assembly_msg(M_EXIT, fmt, ap);
		va_end(ap);
	}
}

void Lib_Log::msg_sys(const char *fmt, ...) {
	if (switcher_ & F_SYS) {
		va_list	ap;

		va_start(ap, fmt);
		assembly_msg(M_SYS, fmt, ap);
		va_end(ap);
	}
}

void Lib_Log::msg_user(const char *fmt, ...) {
	if (switcher_ & F_USER) {
		va_list	ap;

		va_start(ap, fmt);
		assembly_msg(M_USER, fmt, ap);
		va_end(ap);
	}
}

void Lib_Log::msg_user_trace(const char *fmt, ...) {
	if (switcher_ & F_USER) {
		va_list	ap;

		va_start(ap, fmt);
		assembly_msg(M_USER_TRACE, fmt, ap);
		va_end(ap);
	}
}

void Lib_Log::msg_debug(const char *fmt, ...) {
	if (switcher_ & F_DEBUG) {
		va_list	ap;

		va_start(ap, fmt);
		assembly_msg(M_DEBUG, fmt, ap);
		va_end(ap);
	}
}

void Lib_Log::assembly_msg(int log_flag, const char *fmt, va_list ap) {

	std::ostringstream msg_stream;

	struct tm tm_v;
	time_t time_v = time(NULL);

	localtime_r(&time_v, &tm_v);

#ifndef LOCAL_DEBUG
	msg_stream << "<pid=" << (int)getpid() << "|tid=" << pthread_self()
			<< ">(" << (tm_v.tm_hour) << ":" << (tm_v.tm_min) << ":" << (tm_v.tm_sec) << ")";
#endif

	msg_stream << msg_head[log_flag];

	char line_buf[msg_buf_size];
	memset(line_buf, 0, sizeof(line_buf));
	vsnprintf(line_buf, sizeof(line_buf), fmt, ap);

	msg_stream << line_buf;

	switch (log_flag) {
	case M_SYS_ABORT: {
		msg_stream << "errno = " << errno;

		memset(line_buf, 0, sizeof(line_buf));
		strerror_r(errno, line_buf, sizeof(line_buf));
		msg_stream << ", errstr=[" << line_buf << "]" << std::endl;
		logging(msg_stream);
		abort();

		break;
	}
	case M_ABORT: {
		msg_stream << std::endl;
		logging(msg_stream);
		abort();

		break;
	}
	case M_EXIT: {
		msg_stream << std::endl;
		logging(msg_stream);
		exit(1);

		break;
	}
	case M_SYS: {
		msg_stream << ", errno = " << errno;

		memset(line_buf, 0, sizeof(line_buf));
		msg_stream << ", errstr=[" << (strerror_r(errno, line_buf, sizeof(line_buf))) << "]" << std::endl;

		logging(msg_stream);

		break;
	}
	case M_USER: {
		msg_stream << std::endl;
		logging(msg_stream);

		break;
	}
	case M_USER_TRACE: {
		int nptrs;
		void *buffer[backtrace_size];
		char **strings;

		nptrs = backtrace(buffer, backtrace_size);
		strings = backtrace_symbols(buffer, nptrs);
		if (strings == NULL)
			return ;

		msg_stream << std::endl;

		for (int i = 0; i < nptrs; ++i) {
			msg_stream << (strings[i]) << std::endl;
		}

		free(strings);

		logging(msg_stream);

		break;
	}
	case M_DEBUG: {
		msg_stream << std::endl;
		logging(msg_stream);

		break;
	}
	default: {
		break;
	}
	}


	return ;
}

void Lib_Log::make_lib_log_dir(void) {
	int ret = mkdir(lib_log_dir.c_str(), 0775);
	if (ret == -1 && errno != EEXIST) {
		perror("mkdir error");
	}
}

void Lib_Log::make_lib_log_filepath(std::string &path) {
	time_t time_v = time(NULL);
	struct tm tm_v;
	localtime_r(&time_v, &tm_v);

	std::stringstream stream;
	stream 	<< lib_log_dir << "/" << (tm_v.tm_year + 1900) << "-" << (tm_v.tm_mon + 1) << "-" << (tm_v.tm_mday)
			<< "-" << (tm_v.tm_hour) << ".log";

	path = stream.str().c_str();
}

void Lib_Log::logging(std::ostringstream &msg_stream) {
#ifdef LOCAL_DEBUG
	std::cerr << msg_stream.str();
#else
	GUARD(Lib_Log_File_Lock, mon, log_lock_);

	if (! log_file_.fp) {
		make_lib_log_dir();
		make_lib_log_filepath(log_file_.filepath);

		if ((log_file_.fp = fopen(log_file_.filepath.c_str(), "a")) == NULL) {
			printf("filepath=[%s]\n", log_file_.filepath.c_str());
			perror("fopen error");
			return;
		}
		log_file_.tv = Time_Value::gettimeofday();
	} else {
		if (! is_same_hour(log_file_.tv, Time_Value::gettimeofday())) {
			fclose(log_file_.fp);
			log_file_.tv = Time_Value::gettimeofday();
			make_lib_log_dir();
			make_lib_log_filepath(log_file_.filepath);

			if ((log_file_.fp = fopen(log_file_.filepath.c_str(), "a")) == NULL) {
				printf("filepath=[%s]", log_file_.filepath.c_str());
				perror("fopen error");
				return;
			}
		}
	}

	fputs(msg_stream.str().c_str(), log_file_.fp);
	fflush(log_file_.fp);

#endif
}