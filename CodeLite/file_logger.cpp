#include "file_logger.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <sys/time.h>
#include <wx/log.h>
#include <wx/crt.h>
#include "cl_standard_paths.h"

static FileLogger theLogger;
static bool initialized = false;

FileLogger::FileLogger()
    : m_verbosity(FileLogger::Error)
    , m_fp(NULL)
{
}

FileLogger::~FileLogger()
{
    if(m_fp) {
        fclose(m_fp);
        m_fp = NULL;
    }
}

void FileLogger::AddLogLine(const wxString &msg, int verbosity)
{
    if(m_verbosity >= verbosity && m_fp) {
        wxString formattedMsg;

        timeval tim;
        gettimeofday(&tim, NULL);
        int ms = (int)tim.tv_usec / 1000.0;

        wxString msStr = wxString::Format(wxT("%03d"), ms);

        formattedMsg << wxT("[ ")
                     << wxDateTime::Now().FormatISOTime()
                     << wxT(":")
                     << msStr;

        switch(verbosity) {
        case System:
            formattedMsg << wxT(" SYS ] ");
            break;

        case Error:
            formattedMsg << wxT(" ERR ] ");
            break;

        case Warning:
            formattedMsg << wxT(" WRN ] ");
            break;

        case Dbg:
            formattedMsg << wxT(" DBG ] ");
            break;

        case Developer:
            formattedMsg << wxT(" DVL ] ");
            break;

        }


        formattedMsg << msg;
        formattedMsg.Trim().Trim(false);
        formattedMsg << wxT("\n");

        wxFprintf(m_fp, wxT("%s"), formattedMsg.c_str());
        fflush(m_fp);
    }
}

FileLogger* FileLogger::Get()
{
    if(!initialized) {
        wxString filename;
        filename << clStandardPaths::Get().GetUserDataDir() << wxFileName::GetPathSeparator() << wxT("codelite.log");
        theLogger.m_fp = wxFopen(filename, wxT("a+"));
        initialized = true;
    }
    return &theLogger;
}

void FileLogger::SetVerbosity(int level)
{
    if(level > FileLogger::Warning)
        CL_SYSTEM(wxT("Log verbosity is now set to %s"), FileLogger::GetVerbosityAsString(level).c_str());
    m_verbosity = level;
}

int FileLogger::GetVerbosityAsNumber(const wxString& verbosity)
{
    if (verbosity == wxT("Debug")) {
        return FileLogger::Dbg;

    } else if(verbosity == wxT("Error")) {
        return FileLogger::Error;

    } else if(verbosity == wxT("Warning")) {
        return FileLogger::Warning;

    } else if(verbosity == wxT("System")) {
        return FileLogger::System;

    } else if(verbosity == wxT("Developer")) {
        return FileLogger::Developer;

    } else {
        return FileLogger::Error;
    }
}

wxString FileLogger::GetVerbosityAsString(int verbosity)
{
    switch(verbosity) {
    case FileLogger::Dbg:
        return wxT("Debug");

    case FileLogger::Error:
        return wxT("Error");

    case FileLogger::Warning:
        return wxT("Warning");

    case FileLogger::Developer:
        return wxT("Developer");

    default:
        return wxT("Error");
    }
}

void FileLogger::SetVerbosity(const wxString& verbosity)
{
    SetVerbosity( FileLogger::GetVerbosityAsNumber(verbosity) );
}
