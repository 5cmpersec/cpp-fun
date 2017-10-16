#include "Process.hpp"

#include <iostream>
#include <cassert>

namespace process {
    Status::Status(int wstatus)
        : exited{WIFEXITED(wstatus)}, signaled{WIFSIGNALED(wstatus)},
        crashed{signaled ? WCOREDUMP(wstatus) == 1 : false},
        paused{WIFSTOPPED(wstatus)}, resumed{WIFCONTINUED(wstatus)},
        exitStatus{exited ? static_cast<int8_t>(WEXITSTATUS(wstatus)) : 0},
        signal{signaled ? WTERMSIG(wstatus) : 0},
        pauseSignal{paused ? WSTOPSIG(wstatus) : 0}, wstatus{wstatus} {}

    bool Status::isStillRunning() const
    {
        return !signaled && !exited;
    }

    std::ostream& operator<<(std::ostream& out, Status const& s) {
        return out << "{exited: " << s.exited << ", exitStatus: " << s.exitStatus
            << ", signaled: " << s.signaled << ", signal: " << s.signal
            << ", crashed: " << s.crashed << ", paused: " << s.paused
            << ", pauseSignal: " << s.pauseSignal << ", resumed: " << s.resumed
            << "}";
    }

    Child::Child(pid_t pid)
        : pid{pid} {
            std::cout << "will monitor pid:" << pid << "\n";
            auto task = std::packaged_task<Status()>{[pid] {
                assert(pid != process::NO_CHILD);
                int wstatus = 0;
                while (::waitpid(pid, &wstatus, 0) != pid && errno == EINTR) {
                    errno = 0;
                }
                Status s{wstatus};
                std::cout << "monitoring of pid:" << pid << "ended with wstatus:" << wstatus << " status:" << s << "\n";
                return s;
            }};

            this->futureStatus = std::move(task.get_future());
            std::thread{std::move(task)}.detach();
        }

    bool operator==(Child const& a, Child const& b) { return a.pid == b.pid; }
    bool operator!=(Child const& a, Child const& b) { return a.pid != b.pid; }

    bool Child::isReady() {
        if (this->futureStatus.valid()) {
            assert(this->pid != process::NO_CHILD);
            return this->futureStatus.wait_for(std::chrono::microseconds(0)) == std::future_status::ready;
        }
        return false;
    }

    Status Child::wait() {
        if (this->futureStatus.valid()) {
            assert(this->pid != process::NO_CHILD);

            std::cout << "waiting" << this->pid << "\n";
            this->futureStatus.wait();
            std::cout << "wait over for" << this->pid << "\n";
            this->status = this->futureStatus.get();
            this->pid = process::NO_CHILD;
        }

        return this->status;
    }

    Status Child::wait(std::chrono::nanoseconds const& timeout) {
        if (this->futureStatus.valid()) {
            assert(this->pid != process::NO_CHILD);

            auto status = this->futureStatus.wait_for(timeout);
            if (status == std::future_status::ready) {
                this->status = this->futureStatus.get();
                this->pid = process::NO_CHILD;
            }
        }

        return this->status;
    }

    Status Child::triggerExit() {
        if (this->futureStatus.valid()) {
            assert(this->pid != process::NO_CHILD);

            std::cout << "quitting" << this->pid << "\n";

            ::kill(this->pid, SIGTERM);
        }

        this->wait();

        if (this->status.signaled && this->status.signal == SIGTERM) {
            this->status.signaled = false;
            this->status.exited = 1;
            this->status.exitStatus = 0;
        }

        return this->status;
    }

    Status Child::triggerExit(std::chrono::nanoseconds const& timeout) {
        if (this->futureStatus.valid()) {
            assert(this->pid != process::NO_CHILD);

            std::cout << "quitting" << this->pid << "with" << timeout.count() << "ns timeout" << "\n";

            ::kill(this->pid, SIGTERM);
        }

        this->wait(timeout);

        if (this->status.signaled && this->status.signal == SIGTERM) {
            this->status.signaled = false;
            this->status.exited = 1;
            this->status.exitStatus = 0;
        }

        return this->status;
    }

    Status Child::terminate() {
        if (this->futureStatus.valid()) {
            assert(this->pid != process::NO_CHILD);

            std::cout << "terminating" << this->pid << "\n";

            ::kill(-this->pid, SIGKILL);
        }

        return this->wait();
    }

    Status Child::pause() {
        assert(this->pid != process::NO_CHILD);

        std::cout << "pausing" << this->pid << "\n";

        ::kill(this->pid, SIGSTOP);

        int wstatus = 0;
        ::waitpid(this->pid, &wstatus, WUNTRACED);

        auto ret = Status{wstatus};
        assert(!ret.exited);
        assert(!ret.signaled);
        assert(!ret.crashed);
        assert(ret.paused);
        assert(ret.pauseSignal == SIGSTOP);
        assert(!ret.resumed);

        return ret;
    }

    Status Child::resume() {
        assert(this->pid != process::NO_CHILD);

        std::cout << "resuming" << this->pid << "\n";

        ::kill(this->pid, SIGCONT);

        int wstatus = 0;
        ::waitpid(this->pid, &wstatus, WCONTINUED);

        auto ret = Status{wstatus};
        assert(!ret.exited);
        assert(!ret.signaled);
        assert(!ret.crashed);
        assert(!ret.paused);
        assert(ret.resumed);

        return ret;
    }

    void Child::waitThenTerminate(std::chrono::nanoseconds const& timeout) {
        if (*this != process::Child{}) {
            std::cout << "Process is still running.\n";
            auto status = wait(timeout);
            if (status.isStillRunning()) {
                std:: << "Process took too long to stop.\n";
                terminate();
            }
        }
    }

    Child execute(std::string const& commandLine,
            std::string const& workingDirectory,
            bool silenceOutput,
            bool unblockSignals) {
        auto pid = process::NO_CHILD;
        int pipefd[2] = {};

        std::cout << "running" << commandLine << "\n";

        if (::pipe(pipefd) == -1)
        {
          //"unable to create pipe"
          return;
        }

        auto mask = sigset_t{0};
        auto omask = sigset_t{0};
        ::sigemptyset(&mask);
        ::sigaddset(&mask, SIGINT);
        ::sigaddset(&mask, SIGTERM);
        ::sigaddset(&mask, SIGQUIT);
        ::sigprocmask(SIG_BLOCK, &mask, &omask);

        errno = 0;
        if ((pid = ::fork()) == 0) {
            auto arguments = std::vector<char*>{};
            auto args = commandLine;

            {
                size_t i = 0;
                while (i < args.size()) {
                    arguments.emplace_back(&args[i]);

                    do {
                        i = args.find_first_of("\"' \t", i);
                        if (i == std::string::npos || (i == 0 || args[i - 1] != '\\'))
                            break;
                    } while (true);
                    if (i == std::string::npos)
                        break;

                    // skip quoted arguments until the next corresponding quote
                    if (args[i] == '"' || args[i] == '\'') {
                        do {
                            i = args.find_first_of(args[i], i + 1);
                            if (i == std::string::npos || (i == 0 || args[i - 1] != '\\'))
                                break;
                        } while (true);
                        if (i == std::string::npos)
                            break;
                        i++;

                        // FIXME: here we could loop back to whitespace / quotes search above,
                        // but we will instead assume that there won't be tricks in the
                        // command-line.
                        assert(i == args.size() || args[i] == ' ' || args[i] == '\t');
                    }

                    // ignore any number of whitespace between arguments
                    while (i < args.size() && (args[i] == ' ' || args[i] == '\t')) {
                        args[i] = '\0';
                        i++;
                    }
                }
            }
            arguments.emplace_back(nullptr);

            setStaskSize(0, s_child_process_stacksize);

            if (silenceOutput) {
                int devnull = open("/dev/null", O_RDWR);
                if (-1 != devnull) {
                    //(child) silencing stdout/stderr
                    dup2(devnull, STDOUT_FILENO);
                    dup2(devnull, STDERR_FILENO);
                    close(devnull);
                } else {
                    //error : "(child) couldn't open /dev/null for silencing"
                }
            }

            if (::setpgid(0, 0) == -1)
                //(child) unable to change to own process group"
            else if (unblockSignals && ::sigprocmask(SIG_SETMASK, &omask, NULL))
                //"(child) couldn't set original signal mask"
            else if (::close(pipefd[0]) == -1)
                //"(child) couldn't close pipefd[0]"
            else if (workingDirectory != "" &&
                    filesystem::makeDirs(workingDirectory.c_str()).getError() != NO_ERROR)
                //(child) couldn't change working directory to workingDirectory
            else if (workingDirectory != "" && ::chdir(workingDirectory.c_str()) == -1)
                //(child) couldn't change working directory to workingDirectory);
            else if (::execv(arguments[0], &arguments[0]) == -1)
                //(child) couldn't execute:", commandLine

            auto code = errno;
            errno = 0;
            if (::write(pipefd[1], &code, sizeof(code)) != sizeof(code)) {
                //(child) couldn't write error to parent
                ::close(pipefd[1]);
                std::exit(errno);
            }

            ::close(pipefd[1]);
            std::exit(255);
        }

        if (pid == process::NO_CHILD)
        {
          //unable to fork new process for child
        }

        errno = 0;
        if (::setpgid(pid, pid) == -1 && errno != EACCES)
            //unable to change child process group
        else if (::sigprocmask(SIG_SETMASK, &omask, NULL) == -1)
            //unable to restore process signal mask
        else if (::close(pipefd[1]) == -1)
            //couldn't close pipefd[1]

        auto code = errno = 0;
        ::read(pipefd[0], &code, sizeof(code));
        ::close(pipefd[0]);

        if (code != 0) {
            errno = code;
            //couldn't execute commandLine

            int wstatus = 0;
            ::waitpid(pid, &wstatus, 0);

            auto result = Status{wstatus};
            assert(result.exited);
            assert(result.exitStatus == 255);

            // unable to execute commandLine in child
        }

        return Child{pid};
    }

    void daemonize() {
        if (::daemon(0, 0) == -1)
        {
          //"unable to daemonize
        }
        return NO_ERROR;
    }

}
