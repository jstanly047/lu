#pragma once

#include <string>

namespace lu::thread
{
    class Job;

    class IJobCallback
    {
    public:
        virtual ~IJobCallback() = default;

        virtual void onJobComplete(Job *job) = 0;
        virtual void onJobAbort(unsigned int jobID, Job *job) = 0;
        virtual void onJobProgress(unsigned int jobID,
                                   unsigned int progress,
                                   unsigned int total,
                                   const Job *job) = 0;
    };

    class JobManager;

    class Job
    {
    public:
        enum PRIORITY
        {
            PRIORITY_LOW_PAUSABLE = 0,
            PRIORITY_LOW,
            PRIORITY_NORMAL,
            PRIORITY_HIGH,
            PRIORITY_DEDICATED,
        };

        virtual ~Job() = default;
        virtual bool doWork() = 0;
        virtual const std::string getType() const = 0;
        virtual bool operator==(const Job *job) const = 0;

        virtual bool shouldCancel(unsigned int progress, unsigned int total) const;

    private:
        friend class JobManager;
        JobManager *m_jobMangerCallBack = nullptr;
    };
}