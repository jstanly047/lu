#pragma once
#include <thread/JobManager.h>
#include <functional>
#include <mutex>
#include <vector>
#include <queue>

namespace thread
{
    class JobQueue : public IJobCallback
    {
        class JobPointer
        {
        public:
            explicit JobPointer(JobManager &jobManager, Job *job);
            void cancelJob();
            void freeJob();
            bool operator==(const Job *job) const;

            std::reference_wrapper<JobManager> m_jobManager;
            Job *m_job;
            unsigned int m_id;
        };

    public:
        JobQueue(JobManager &jobManager, bool lifo = false, unsigned int jobsAtOnce = 1, Job::PRIORITY priority = Job::PRIORITY_LOW);
        ~JobQueue() override;
        bool addJob(Job *job);
        template <typename F>
        void submit(F &&f)
        {
            addJob(new LambdaJob<F>(std::forward<F>(f)));
        }

        void cancelJob(const Job *job);
        void cancelJobs();
        bool isProcessing() const;
        void onJobComplete(unsigned int jobID, bool success, Job *job) override;
        void onJobAbort(unsigned int jobID, Job *job) override;

    protected:
        bool queueEmpty() const;

    private:
        void onJobNotify(Job *job);
        void queueNextJob();

        JobManager &m_jobManager;
        unsigned int m_jobsAtOnce;
        Job::PRIORITY m_priority;
        mutable std::mutex m_section;
        bool m_lifo;
        std::deque<JobPointer> m_jobQueue;
        std::vector<JobPointer> m_processing;
    };
}