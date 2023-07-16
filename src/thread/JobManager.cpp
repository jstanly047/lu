
#include <thread/JobManager.h>
#include <thread>
#include <algorithm>
#include <iostream>

using namespace lu::thread;

JobWorker::JobWorker(JobManager *manager)
{
  m_jobManager = manager;
  start();
}

JobWorker::~JobWorker()
{
  m_jobManager->removeWorker(this);
}

void JobWorker::start()
{
  m_thread = std::thread(&JobWorker::process, this);
}

void JobWorker::process()
{
  while (true)
  {
    // request an item from our manager (this call is blocking)
    Job *job = m_jobManager->getNextJob(this);
    if (job == nullptr)
    {
      break;
    }

    /*bool success = false;

    try
    {
      success = job->doWork();
    }
    catch (...)
    {
      std::cout << "Error processing job " << job->getType() << std::endl;
    }*/

    m_jobManager->onJobComplete(job);
  }
}

JobManager::WorkItem::WorkItem(Job *job, unsigned int id, Job::PRIORITY priority, IJobCallback *callback)
{
  m_job = job;
  m_id = id;
  m_userCallBack = callback;
  m_priority = priority;
}

bool JobManager::WorkItem::operator==(unsigned int jobID) const
{
  return m_id == jobID;
}

bool JobManager::WorkItem::operator==(const Job *job) const
{
  return m_job == job;
}

void JobManager::WorkItem::freeJob()
{
  delete m_job;
  m_job = nullptr;
}

void JobManager::WorkItem::cancel()
{
  m_userCallBack = nullptr;
}

bool Job::shouldCancel(unsigned int progress, unsigned int total) const
{
  if (m_jobMangerCallBack != nullptr)
  {
    return m_jobMangerCallBack->onJobProgress(progress, total, this);
  }

  return false;
}


JobManager::JobManager()
{
  m_jobCounter = 0;
  m_running = true;
  m_pauseJobs = false;
}

void JobManager::restart()
{
  std::unique_lock<std::mutex> lock(m_section);

  if (m_running)
  {
    throw std::logic_error("JobManager already running");
  }

  m_running = true;
}

void JobManager::cancelJobs()
{
  std::unique_lock<std::mutex> lock(m_section);
  m_running = false;

  // clear any pending jobs
  for (unsigned int priority = Job::PRIORITY_LOW_PAUSABLE; priority <= Job::PRIORITY_DEDICATED; ++priority)
  {
    std::for_each(m_jobQueue[priority].begin(), m_jobQueue[priority].end(), [](WorkItem &wi)
                  {
      if (wi.m_userCallBack)
      {
        wi.m_userCallBack->onJobAbort(wi.m_id, wi.m_job);
      }

      wi.freeJob(); });

    m_jobQueue[priority].clear();
  }

  // cancel any callbacks on jobs still processing
  std::for_each(m_processing.begin(), m_processing.end(), [](WorkItem &wi)
                {
    if (wi.m_userCallBack)
    {
      wi.m_userCallBack->onJobAbort(wi.m_id, wi.m_job);
    }

    wi.cancel(); });

  // tell our workers to finish
  while (m_workers.size())
  {
    lock.unlock();
    m_cv.notify_all();
    std::this_thread::yield(); // yield after setting the event to give the workers some time to die
    lock.lock();
  }
}

unsigned int JobManager::addJob(Job *job, IJobCallback *callback, Job::PRIORITY priority)
{
  std::unique_lock<std::mutex> lock(m_section);

  if (!m_running)
  {
    delete job;
    return 0;
  }

  // increment the job counter, ensuring 0 (invalid job) is never hit
  m_jobCounter++;
  if (m_jobCounter == 0)
    m_jobCounter++;

  // create a work item for this job
  WorkItem work(job, m_jobCounter, priority, callback);
  m_jobQueue[priority].push_back(work);
  startWorkers(priority);
  return work.m_id;
}

void JobManager::cancelJob(unsigned int jobID)
{
  std::unique_lock<std::mutex> lock(m_section);

  // check whether we have this job in the queue
  for (unsigned int priority = Job::PRIORITY_LOW_PAUSABLE; priority <= Job::PRIORITY_DEDICATED; ++priority)
  {
    auto workItemItr = find(m_jobQueue[priority].begin(), m_jobQueue[priority].end(), jobID);

    if (workItemItr != m_jobQueue[priority].end())
    {
      delete workItemItr->m_job;
      m_jobQueue[priority].erase(workItemItr);
      return;
    }
  }
  // or if we're processing it
  auto processingWorkItemItr = find(m_processing.begin(), m_processing.end(), jobID);
  if (processingWorkItemItr != m_processing.end())
  {
    processingWorkItemItr->m_userCallBack = nullptr; // job is in progress, so only thing to do is to remove callback
  }
}

void JobManager::startWorkers(Job::PRIORITY priority)
{
  // check how many free threads we have
  if (m_processing.size() >= getMaxWorkers(priority))
  {
    return;
  }

  // do we have any sleeping threads?
  if (m_processing.size() < m_workers.size())
  {
    m_cv.notify_all();
    return;
  }

  // everyone is busy - we need more workers
  m_workers.push_back(new JobWorker(this));
}

Job *JobManager::popJob()
{
  for (int priority = Job::PRIORITY_DEDICATED; priority >= Job::PRIORITY_LOW_PAUSABLE; --priority)
  {
    // Check whether we're pausing pausable jobs
    if (priority == Job::PRIORITY_LOW_PAUSABLE && m_pauseJobs)
    {
      continue;
    }

    if (m_jobQueue[priority].size() && m_processing.size() < getMaxWorkers(Job::PRIORITY(priority)))
    {
      // pop the job off the queue
      WorkItem job = m_jobQueue[priority].front();
      m_jobQueue[priority].pop_front();
      // add to the processing vector
      m_processing.push_back(job);
      job.m_job->m_jobMangerCallBack = this;
      return job.m_job;
    }
  }

  return nullptr;
}

void JobManager::pauseJobs()
{
  std::unique_lock<std::mutex> lock(m_section);
  m_pauseJobs = true;
}

void JobManager::unPauseJobs()
{
  std::unique_lock<std::mutex> lock(m_section);
  m_pauseJobs = false;
}

bool JobManager::isProcessing(const Job::PRIORITY &priority) const
{
  std::unique_lock<std::mutex> lock(m_section);

  if (m_pauseJobs)
  {
    return false;
  }

  for (auto &workItem : m_processing)
  {
    if (priority == workItem.m_priority)
    {
      return true;
    }
  }
  return false;
}

int JobManager::isProcessing(const std::string &type) const
{
  int jobsMatched = 0;
  std::unique_lock<std::mutex> lock(m_section);

  if (m_pauseJobs)
  {
    return 0;
  }

  for (auto &workItem : m_processing)
  {
    if (type == std::string(workItem.m_job->getType()))
    {
      jobsMatched++;
    }
  }
  return jobsMatched;
}

Job *JobManager::getNextJob(const JobWorker *worker)
{
  {
    std::unique_lock<std::mutex> lock(m_section);
    while (m_running)
    {
      // grab a job off the queue if we have one
      Job *job = popJob();

      if (job != nullptr)
      {
        return job;
      }

      // no jobs are left - sleep for 30 seconds to allow new jobs to come in
      m_cv.wait(lock);
    }
  }

  // have no jobs
  removeWorker(worker);
  return nullptr;
}

bool JobManager::onJobProgress(unsigned int progress, unsigned int total, const Job *job) const
{
  std::unique_lock<std::mutex> lock(m_section);
  // find the job in the processing queue, and check whether it's cancelled (no callback)
  auto workItemItr = find(m_processing.begin(), m_processing.end(), job);

  if (workItemItr != m_processing.end())
  {
    JobManager::WorkItem item(*workItemItr);
    lock.unlock(); // leave section prior to call

    if (item.m_userCallBack)
    {
      item.m_userCallBack->onJobProgress(item.m_id, progress, total, job);
      return false;
    }
  }

  return true; // couldn't find the job, or it's been cancelled
}


void JobManager::onJobComplete(Job *job)
{
  std::unique_lock<std::mutex> lock(m_section);
  // remove the job from the processing queue
  auto workItemItr = find(m_processing.begin(), m_processing.end(), job);

  if (workItemItr != m_processing.end())
  {
    // tell any listeners we're done with the job, then delete it
    JobManager::WorkItem item(*workItemItr);
    lock.unlock();

    try
    {
      if (item.m_userCallBack)
      {
        item.m_userCallBack->onJobComplete(item.m_job);
      }
    }
    catch (...)
    {
      std::cout << "error processing job " << item.m_job->getType() << std::endl;
    }

    lock.lock();
    auto processingWorkItem = find(m_processing.begin(), m_processing.end(), job);

    if (processingWorkItem != m_processing.end())
    {
      m_processing.erase(processingWorkItem);
    }

    lock.unlock();
    item.freeJob();
  }
}

void JobManager::removeWorker(const JobWorker *worker)
{
  std::unique_lock<std::mutex> lock(m_section);
  // remove our worker
  auto workerItr = find(m_workers.begin(), m_workers.end(), worker);
  if (workerItr != m_workers.end())
  {
    m_workers.erase(workerItr); // workers auto-delete
  }
}

unsigned int JobManager::getMaxWorkers(Job::PRIORITY priority)
{
  static const unsigned int max_workers = std::thread::hardware_concurrency() <= 2 ? 1U : std::thread::hardware_concurrency();

  if (priority == Job::PRIORITY_DEDICATED)
  {
    return 10000; // A large number..
  }

  return max_workers - (Job::PRIORITY_HIGH - priority);
}
