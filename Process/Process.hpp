#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#include <chrono>
#include <future>

namespace process {
    static constexpr auto NO_CHILD = pid_t{-1};

    struct Status {
        Status(int wstatus);

        Status() = default;
        Status(Status const&) = default;
        Status(Status&&) = default;
        Status& operator=(Status const&) = default;
        Status& operator=(Status&&) = default;

        bool isStillRunning() const;

        bool exited = false;
        bool signaled = false;
        bool crashed = false;
        bool paused = false;
        bool resumed = false;
        int exitStatus = 0;
        int signal = 0;
        int pauseSignal = 0;

        int wstatus = 0;
    };
    std::ostream& operator<<(std::ostream& out, Status const& s);

    struct Child {
        Status status = {};
        pid_t pid = NO_CHILD;
        std::future<Status> futureStatus;

        Child(pid_t pid);

        Child() = default;
        Child(Child&&) = default;
        Child& operator=(Child&&) = default;

        Child(Child const&) = delete;
        Child& operator=(Child const&) = delete;

        bool isReady();

        Status wait();

        Status wait(std::chrono::nanoseconds const& timeout);

        Status triggerExit();

        Status triggerExit(std::chrono::nanoseconds const& timeout);

        Status terminate();
        Status pause();
        Status resume();

        void waitThenTerminate(std::chrono::nanoseconds const& timeout);

        friend bool operator==(Child const& a, Child const& b);
        friend bool operator!=(Child const& a, Child const& b);
    };

    Child execute(std::string const& commandLine,
            std::string const& workingDirectory,
            bool silenceOutput,
            TransitionEvents&& events = TransitionEvents(),
            bool unblockSignals=true);

    void daemonize();
}
#endif // __PROCESS_HPP__
