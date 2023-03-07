#pragma once

#include <thread/Job.h>
#include <queue>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace lu::thread
{
    class JobManager;

    class JobWorker
    {
    public:
        JobWorker(JobManager *manager);
        ~JobWorker();

        void start();

    private:
        void process();

        JobManager *m_jobManager;
        std::thread m_thread;
    };

    template <typename F>
    class LambdaJob : public Job
    {
    public:
        LambdaJob(F &&f) : m_f(std::forward<F>(f)) {}
        bool doWork() override
        {
            m_f();
            return true;
        }
        bool operator==(const Job *job) const override
        {
            return this == job;
        };

    private:
        F m_f;
    };

    class JobManager final
    {
        class WorkItem
        {
        public:
            WorkItem(Job *job, unsigned int id, Job::PRIORITY priority, IJobCallback *userCallBack);
            bool operator==(unsigned int jobID) const;
            bool operator==(const Job *job) const;
            void freeJob();
            void cancel();

            Job *m_job = nullptr;
            unsigned int m_id;
            IJobCallback *m_userCallBack = nullptr;
            Job::PRIORITY m_priority;
        };

    public:
        JobManager();
        unsigned int addJob(Job *job, IJobCallback *callback, Job::PRIORITY priority = Job::PRIORITY_LOW);

        template <typename F>
        void Submit(F &&f, Job::PRIORITY priority = Job::PRIORITY_LOW)
        {
            addJob(new LambdaJob<F>(std::forward<F>(f)), nullptr, priority);
        }

        template <typename F>
        void submit(F &&f, IJobCallback *callback, Job::PRIORITY priority = Job::PRIORITY_LOW)
        {
            addJob(new LambdaJob<F>(std::forward<F>(f)), callback, priority);
        }

        void cancelJob(unsigned int jobID);
        void cancelJobs();
        void restart();
        int isProcessing(const std::string &type) const;
        void pauseJobs();
        void unPauseJobs();
        bool isProcessing(const Job::PRIORITY &priority) const;

    protected:
        friend class JobWorker;
        friend class Job;
        friend class JobQueue;

        Job *getNextJob(const JobWorker *worker);
        void onJobComplete(bool success, Job *job);
        bool onJobProgress(unsigned int progress, unsigned int total, const Job *job) const;

    private:
        JobManager(const JobManager &) = delete;
        JobManager const &operator=(JobManager const &) = delete;

        Job *popJob();

        void startWorkers(Job::PRIORITY priority);
        void removeWorker(const JobWorker *worker);
        static unsigned int getMaxWorkers(Job::PRIORITY priority);

        unsigned int m_jobCounter;
        bool m_pauseJobs;
        bool m_running;
        std::deque<WorkItem> m_jobQueue[Job::PRIORITY_DEDICATED + 1];
        std::vector<WorkItem> m_processing;
        std::vector<JobWorker *> m_workers;
        mutable std::mutex m_section;
        std::condition_variable m_cv;
    };
}