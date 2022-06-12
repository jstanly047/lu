#include <thread/JobQueue.h>
#include <algorithm>
#include <bpms/Log.h>

using namespace thread;

JobQueue::JobPointer::JobPointer(JobManager &jobManager, Job *job) : 
  m_jobManager(jobManager),
  m_job(job),
  m_id(0)
{
}

void JobQueue::JobPointer::freeJob()
{
  delete m_job;
  m_job = nullptr;
}

bool JobQueue::JobPointer::operator==(const Job *job) const
{
  if (m_job != nullptr)
  {
    return *m_job == job;
  }

  return false;
}

void JobQueue::JobPointer::cancelJob()
{
  m_jobManager.get().cancelJob(m_id);
  m_id = 0;
}

JobQueue::JobQueue(JobManager &jobManager, bool lifo, unsigned int jobsAtOnce, Job::PRIORITY priority)
    : m_jobManager(jobManager), m_jobsAtOnce(jobsAtOnce), m_priority(priority), m_lifo(lifo)
{
}

JobQueue::~JobQueue()
{
  cancelJobs();
}

void JobQueue::onJobComplete(unsigned int jobID, bool success, Job *job)
{
  onJobNotify(job);
}

void JobQueue::onJobAbort(unsigned int jobID, Job *job)
{
  onJobNotify(job);
}

void JobQueue::cancelJob(const Job *job)
{
  std::unique_lock<std::mutex> lock(m_section);
  auto processingJobPointerItr = std::find(m_processing.begin(), m_processing.end(), job);

  if (processingJobPointerItr != m_processing.end())
  {
    processingJobPointerItr->cancelJob();
    m_processing.erase(processingJobPointerItr);
    return;
  }

  auto jobPointerItr = std::find(m_jobQueue.begin(), m_jobQueue.end(), job);

  if (jobPointerItr != m_jobQueue.end())
  {
    jobPointerItr->freeJob();
    m_jobQueue.erase(jobPointerItr);
  }
}

bool JobQueue::addJob(Job *job)
{
  std::unique_lock<std::mutex> lock(m_section);
  // check if we have this job already.  If so, we're done.
  if (std::find(m_jobQueue.begin(), m_jobQueue.end(), job) != m_jobQueue.end() ||
      std::find(m_processing.begin(), m_processing.end(), job) != m_processing.end())
  {
    delete job;
    return false;
  }

  if (m_lifo)
  {
    m_jobQueue.push_back(JobPointer(m_jobManager, job));
  }
  else
  {
    m_jobQueue.push_front(JobPointer(m_jobManager, job));
  }

  queueNextJob();
  return true;
}

void JobQueue::onJobNotify(Job *job)
{
  std::unique_lock<std::mutex> lock(m_section);

  // check if this job is in our processing list
  const auto it = std::find(m_processing.begin(), m_processing.end(), job);

  if (it != m_processing.end())
  {
    m_processing.erase(it);
  }
  else
  {
    std::cout << "JobQueue::onJobNotify failed to remove: " << job->getType() << std::endl;
  }

  // request a new job be queued
  queueNextJob();
}

void JobQueue::queueNextJob()
{
  while (m_jobQueue.size() && m_processing.size() < m_jobsAtOnce)
  {
    JobPointer &job = m_jobQueue.back();
    job.m_id = m_jobManager.addJob(job.m_job, this, m_priority);

    if (job.m_id > 0)
    {
      m_processing.emplace_back(job);
      m_jobQueue.pop_back();
      return;
    }

    m_jobQueue.pop_back();
  }
}

void JobQueue::cancelJobs()
{
  std::unique_lock<std::mutex> lock(m_section);
  std::for_each(m_processing.begin(), m_processing.end(), [](JobPointer &jp)
                { jp.cancelJob(); });
  std::for_each(m_jobQueue.begin(), m_jobQueue.end(), [](JobPointer &jp)
                { jp.freeJob(); });
  m_jobQueue.clear();
  m_processing.clear();
}

bool JobQueue::isProcessing() const
{
  return m_jobManager.m_running &&
         (!m_processing.empty() || !m_jobQueue.empty());
}

bool JobQueue::queueEmpty() const
{
  std::unique_lock<std::mutex> lock(m_section);
  return m_jobQueue.empty();
}