#include "LLDBDebugger.h"
#include <wx/msgdlg.h>
#include <wx/app.h>

#include "lldb/API/SBEvent.h"
#include "lldb/API/SBCommandInterpreter.h"
#include "lldb/API/SBCommandReturnObject.h"
#include "lldb/API/SBTarget.h"
#include "LLDBEvent.h"
#include <algorithm>
#include "LLDBDebuggerThread.h"
#include <lldb/API/SBTarget.h>

#define CHECK_RUNNING_RET_FALSE() if ( !IsValid() ) return false
#define CHECK_RUNNING_RET() if ( !IsValid() ) return

static char** _wxArrayStringToCharPtrPtr(const wxArrayString &arr)
{
    char** argv = new char*[arr.size()+1]; // for the NULL
    for(size_t i=0; i<arr.size(); ++i) {
        argv[i] = strdup(arr.Item(i).mb_str(wxConvUTF8).data());
    }
    argv[arr.size()] = NULL;
    return argv;
}

static void _deleteCharPtrPtr(char** argv)
{
    size_t i=0;
    while ( argv[i] ) {
        delete [] argv[i];
        ++i;
    }
    delete [] argv;
}

LLDBDebugger::LLDBDebugger()
    : m_thread(NULL)
    , m_canInteract(false)
    , m_debugeePid(wxNOT_FOUND)
{
}

LLDBDebugger::~LLDBDebugger()
{
}

bool LLDBDebugger::Start(const wxString& filename)
{
    if ( m_thread ) {
        // another instance is already running
        ::wxMessageBox(_("A Debug session is already in progress!"));
        return false;
    }
    
    m_debugeePid = wxNOT_FOUND;
    m_debugger = lldb::SBDebugger::Create();
    m_target = m_debugger.CreateTarget(filename.mb_str().data());
    if ( !m_target.IsValid() ) {
        ::wxMessageBox(_("Could not create target"));
        return false;
    }

    m_debugger.SetAsync(true);

    // Notify successful start of the debugger
    NotifyStarted();
    return true;
}

void LLDBDebugger::Initialize()
{
    lldb::SBDebugger::Initialize();
}

void LLDBDebugger::Terminate()
{
    lldb::SBDebugger::Terminate();
}

bool LLDBDebugger::Run( const wxString &in, const wxString& out, const wxString &err, 
                        const wxArrayString& argvArr,
                        const wxArrayString& envArr,
                        const wxString &workingDirectory)
{
    if ( m_thread ) {
        return false;
    }
    
    if ( m_debugger.IsValid() ) {
        // Construct char** arrays
        const char** argv = (const char**)_wxArrayStringToCharPtrPtr(argvArr);
        const char** envp = (const char**)_wxArrayStringToCharPtrPtr(envArr);
        const char* pin  = in.mb_str(wxConvUTF8).data();
        const char* pout = out.mb_str(wxConvUTF8).data();
        const char* perr = err.mb_str(wxConvUTF8).data();
        const char* wd = workingDirectory.mb_str(wxConvUTF8).data();
        
        wxUnusedVar(pin);
        wxUnusedVar(pout);
        wxUnusedVar(perr);
        
        lldb::SBLaunchInfo launchInfo(argv);
        lldb::SBError error;
        
        // Set the launch flags
        launchInfo.SetLaunchFlags(lldb::eLaunchFlagStopAtEntry | lldb::eLaunchFlagLaunchInSeparateProcessGroup);
        launchInfo.SetEnvironmentEntries(envp, false);
        launchInfo.SetWorkingDirectory(wd);
        launchInfo.AddOpenFileAction(STDIN_FILENO,  pin,  true, false);
        launchInfo.AddOpenFileAction(STDERR_FILENO, perr, false, true);
        launchInfo.AddOpenFileAction(STDOUT_FILENO, pout, false, true);
        bool isOk = m_target.Launch(launchInfo, error).IsValid();
        
        //bool isOk = m_target.LaunchSimple(argv, envp, wd).IsValid();
        _deleteCharPtrPtr( const_cast<char**>(argv) );
        _deleteCharPtrPtr( const_cast<char**>(envp) );
        if ( !isOk ) {
            Cleanup();
            NotifyExited();
        }
        
        m_debugeePid = m_target.GetProcess().GetProcessID();
        m_thread = new LLDBDebuggerThread(this, m_debugger.GetListener(), m_target.GetProcess());
        m_thread->Start();
        return isOk;
    }
    return false;
}

bool LLDBDebugger::Continue()
{
    CHECK_RUNNING_RET_FALSE();
    return m_target.GetProcess().Continue().Success();
}

bool LLDBDebugger::StepOver()
{
    CHECK_RUNNING_RET_FALSE();
    lldb::SBCommandReturnObject ret;
    lldb::ReturnStatus status = m_debugger.GetCommandInterpreter().HandleCommand("next", ret);
    return  status == lldb::eReturnStatusStarted                    ||
            status == lldb::eReturnStatusSuccessContinuingNoResult  ||
            status == lldb::eReturnStatusSuccessFinishNoResult      ||
            status == lldb::eReturnStatusSuccessFinishResult        ||
            status == lldb::eReturnStatusSuccessContinuingResult;
}

bool LLDBDebugger::IsValid() const
{
    return m_target.IsValid() && m_debugger.IsValid();
}

void LLDBDebugger::Stop(bool notifyExit)
{
    wxDELETE(m_thread);
    m_target.GetProcess().Kill();
    Cleanup();
    if ( notifyExit ) {
        NotifyExited();
    }
}

void LLDBDebugger::NotifyBacktrace()
{
    lldb::SBThread thread = m_target.GetProcess().GetSelectedThread();
    if ( thread.IsValid() ) {
        LLDBEvent event(wxEVT_LLDB_BACKTRACE);
        LLDBBacktrace bt(thread);
        event.SetBacktrace( bt );
        event.SetEventObject(this);
        AddPendingEvent( event );
    }
}

void LLDBDebugger::NotifyStopped()
{
    lldb::SBThread thread = m_target.GetProcess().GetSelectedThread();
    if ( thread.IsValid() ) {
        LLDBBacktrace bt( thread );
        if ( !bt.GetCallstack().empty() ) {
            LLDBEvent event(wxEVT_LLDB_STOPPED);
            event.SetFileName(bt.GetCallstack().at(0).filename);
            event.SetLinenumber(bt.GetCallstack().at(0).line);
            AddPendingEvent( event );
        }
    }
}

void LLDBDebugger::NotifyExited()
{
    wxDELETE(m_thread);
    LLDBEvent event(wxEVT_LLDB_EXITED);
    AddPendingEvent( event );
}

void LLDBDebugger::NotifyStarted()
{
    LLDBEvent event(wxEVT_LLDB_STARTED);
    AddPendingEvent( event );
}

void LLDBDebugger::NotifyStoppedOnFirstEntry()
{
    LLDBEvent event(wxEVT_LLDB_STOPPED_ON_FIRST_ENTRY);
    AddPendingEvent( event );
}

bool LLDBDebugger::StepIn()
{
    CHECK_RUNNING_RET_FALSE();
    lldb::SBCommandReturnObject ret;
    lldb::ReturnStatus status = m_debugger.GetCommandInterpreter().HandleCommand("step", ret);
    return  status == lldb::eReturnStatusStarted                    ||
            status == lldb::eReturnStatusSuccessContinuingNoResult  ||
            status == lldb::eReturnStatusSuccessFinishNoResult      ||
            status == lldb::eReturnStatusSuccessFinishResult        ||
            status == lldb::eReturnStatusSuccessContinuingResult;
}

bool LLDBDebugger::Finish()
{
    CHECK_RUNNING_RET_FALSE();
    lldb::SBCommandReturnObject ret;
    lldb::ReturnStatus status = m_debugger.GetCommandInterpreter().HandleCommand("finish", ret);
    return  status == lldb::eReturnStatusStarted                    ||
            status == lldb::eReturnStatusSuccessContinuingNoResult  ||
            status == lldb::eReturnStatusSuccessFinishNoResult      ||
            status == lldb::eReturnStatusSuccessFinishResult        ||
            status == lldb::eReturnStatusSuccessContinuingResult;
}

void LLDBDebugger::AddBreakpoint(const wxFileName& filename, int line)
{
    LLDBBreakpoint breakpoint(filename, line);
    if ( !IsBreakpointExists(breakpoint) ) {
        m_breakpoints.push_back( breakpoint );
    }
}

void LLDBDebugger::AddBreakpoint(const wxString& name)
{
    LLDBBreakpoint breakpoint(name);
    if ( !IsBreakpointExists(breakpoint) ) {
        m_breakpoints.push_back( breakpoint );
    }
}

void LLDBDebugger::ApplyBreakpoints()
{
    CHECK_RUNNING_RET();
    // Apply all breakpoints with an-invalid breakpoint ID
    for(size_t i=0; i<m_breakpoints.size(); ++i) {
        LLDBBreakpoint& breakPoint = m_breakpoints.at(i);
        if ( !breakPoint.IsApplied() ) {
            switch( breakPoint.GetType() ) {
            case LLDBBreakpoint::kFunction: {
                lldb::SBBreakpoint bp = m_target.BreakpointCreateByName(breakPoint.GetName().mb_str().data(), NULL);
                if ( bp.IsValid() ) {
                    breakPoint.SetId( bp.GetID() );
                }
                break;
            }
            case LLDBBreakpoint::kFileLine: {
                lldb::SBBreakpoint bp = m_target.BreakpointCreateByLocation(breakPoint.GetFilename().mb_str().data(), breakPoint.GetLineNumber());
                if ( bp.IsValid() ) {
                    breakPoint.SetId( bp.GetID() );
                }
                break;
            }
            }
        }
    }
}

void LLDBDebugger::InvalidateBreakpoints()
{
    for(size_t i=0; i<m_breakpoints.size(); ++i) {
        m_breakpoints.at(i).Invalidate();
    }
}

bool LLDBDebugger::IsBreakpointExists(const LLDBBreakpoint& bp) const
{
    return std::find( m_breakpoints.begin(), m_breakpoints.end(), bp ) != m_breakpoints.end();
}

void LLDBDebugger::DeleteBreakpoint(const LLDBBreakpoint& breakpoint)
{
    LLDBBreakpoint::Vec_t::iterator iter = std::find( m_breakpoints.begin(), m_breakpoints.end(), breakpoint );
    if ( iter != m_breakpoints.end() ) {
        // If the debugger is active and the breakpoint was applied, delete it
        if ( iter->IsApplied() && m_debugger.IsValid() && m_target.IsValid() ) {
            m_target.BreakpointDelete( iter->GetId() );
        }
        // and remove it from the list of breakpoints
        m_breakpoints.erase( iter );
    }
}

void LLDBDebugger::DeleteAllBreakpoints()
{
    while ( !m_breakpoints.empty() ) {
        if ( m_breakpoints.at(0).IsApplied() && m_debugger.IsValid() && m_target.IsValid() ) {
            m_target.BreakpointDelete( m_breakpoints.at(0).GetId() );
        }
        m_breakpoints.erase(m_breakpoints.begin());
    }
}

void LLDBDebugger::Cleanup()
{
    if ( m_target.IsValid() ) {
        m_debugger.DeleteTarget( m_target );
    }
    if ( m_debugger.IsValid() ) {
        lldb::SBDebugger::Destroy( m_debugger );
    }
}
