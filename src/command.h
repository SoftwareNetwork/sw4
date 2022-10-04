#pragma once

#include "helpers.h"
#include "win32.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace sw {

struct raw_command {
    using argument = variant<string,string_view,path>;
    std::vector<argument> arguments;
    path working_directory;
    // environment

    // exit_code
    // pid, pidfd

    // stdin,stdout,stderr

    //sync()
    //async()

    // execute?
    void start_job_object() {
        //ShellExecuteA(NULL, "open", "C:\\WINDOWS\\system32\\calc.exe", "/k ipconfig", 0, SW_SHOWNORMAL);
        //run_win32{}
    }
    void run_win32() {
#ifdef _WIN32
        DWORD flags = 0;
        STARTUPINFOEXW si = { 0 };
        si.StartupInfo.cb = sizeof(si);
        PROCESS_INFORMATION pi = { 0 };
        LPVOID env = 0;
        LPCWSTR dir = 0;
        int inherit_handles = 1; // must be 1

        flags |= NORMAL_PRIORITY_CLASS;
        flags |= CREATE_UNICODE_ENVIRONMENT;
        flags |= EXTENDED_STARTUPINFO_PRESENT;
        // flags |= CREATE_NO_WINDOW;

        si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;

        win32::executor e;
        win32::pipe<win32::executor> out{e,true}, err{e,true};

        si.StartupInfo.hStdOutput = out.w;
        si.StartupInfo.hStdError = err.w;

        auto job = CreateJobObject(0, 0);
        if (!job) {
            throw std::runtime_error{"cannot CreateJobObject"};
        }
        /*if (!AssignProcessToJobObject(job, GetCurrentProcess())) {
            throw std::runtime_error{"cannot AssignProcessToJobObject"};
        }*/

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION ji;
        if (!QueryInformationJobObject(job, JobObjectExtendedLimitInformation, &ji, sizeof(ji), 0)) {
            throw std::runtime_error{"cannot QueryInformationJobObject"};
        }
        ji.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &ji, sizeof(ji))) {
            throw std::runtime_error{"cannot SetInformationJobObject"};
        }

        JOBOBJECT_ASSOCIATE_COMPLETION_PORT jcp{};
        jcp.CompletionPort = e.port;
        jcp.CompletionKey = (void *)10;
        if (!SetInformationJobObject(job, JobObjectAssociateCompletionPortInformation, &jcp, sizeof(jcp))) {
            throw std::runtime_error{"cannot SetInformationJobObject"};
        }

        std::wstring s;
        for (auto &&a : arguments) {
            auto quote = [&](auto &&as) {
                if (as.contains(L" ")) {
                    s += L"\"" + as + L"\" ";
                } else {
                    s += as + L" ";
                }
            };
            visit(a, overload{
                [&](string &s){
                    quote(path{s}.wstring());
                },
                [&](string_view &s){
                    quote(path{string{s.data(),s.size()}}.wstring());
                },
                [&](path &p){
                    quote(p.wstring());
                }});
        }
        std::wcout << s << "\n";
        auto r = CreateProcessW(0, s.data(), 0, 0,
            inherit_handles,
            flags,
            env,
            dir,
            (LPSTARTUPINFOW)&si, &pi
        );
        if (!r) {
            auto err = GetLastError();
            throw std::runtime_error("CreateProcessW failed, code = " + std::to_string(err));
        }
        if (!AssignProcessToJobObject(job, pi.hProcess)) {
            throw std::runtime_error{"cannot AssignProcessToJobObject"};
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        out.w.reset();
        err.w.reset();

        out.read_async([&](this auto &&f, auto &&buf, auto &&ec) {
            if (ec) {
                out.r.reset();
                return;
            }
            std::cout << string{buf.data(), buf.data()+buf.size()};
            out.read_async(f);
        });
        err.read_async([&](this auto &&f, auto &&buf, auto &&ec) {
            if (ec) {
                err.r.reset();
                return;
            }
            err.read_async(f);
        });
        e.run();
#endif
    }
    void run_linux() {
        // clone3()
    }
    void run_macos() {
        //
    }
    void run() {
#ifdef _WIN32
        run_win32();
#endif
    }

    void add(auto &&p) {
        arguments.push_back(p);
    }
    void add(const char *p) {
        arguments.push_back(string{p});
    }
    auto operator+=(auto &&arg) {
        add(arg);
        return appender{[&](auto &&v){operator+=(v);}};
    }
};

struct io_command : raw_command {
    std::set<path> inputs;
    std::set<path> outputs;
};

struct cl_exe_command : io_command {
    void run() {
        add("/showIncludes");
        scope_exit se{[&]{arguments.pop_back();}};
        io_command::run();
    }
};

using command = variant<io_command, cl_exe_command>;

}
