#include "PhysicsSystem/API/PhysicsThreadPool.h"

// Project includes.
#include "Core/JobSystem.h"

// STL includes
#include <chrono>

//------------------------------------------------------------------------------
// Semaphore
//------------------------------------------------------------------------------
Strontium::PhysicsThreadPool::Semaphore::Semaphore()
  : count(0)
{ }

void Strontium::PhysicsThreadPool::Semaphore::release(uint inNumber)
{
  std::lock_guard cLock(this->lock);
  this->count += static_cast<int>(inNumber);
  if (inNumber > 1)
    this->waitVariable.notify_all();
  else
    this->waitVariable.notify_one();
}

void Strontium::PhysicsThreadPool::Semaphore::acquire(uint inNumber)
{
  std::unique_lock cLock(this->lock);
  this->count -= static_cast<int>(inNumber);
  this->waitVariable.wait(cLock, [this]() { return this->count >= 0; });
}

//------------------------------------------------------------------------------
// Barrier
//------------------------------------------------------------------------------
Strontium::PhysicsThreadPool::PhysicsBarrier::PhysicsBarrier()
  : inUse(false)
  , jobReadIndex(0u)
  , jobWriteIndex(0u)
  , numToAcquire(0)
{
  for (std::atomic<Job *>& j : this->jobs)
    j = nullptr;
}

Strontium::PhysicsThreadPool::PhysicsBarrier::~PhysicsBarrier()
{
  assert(this->isEmpty());
}

void 
Strontium::PhysicsThreadPool::PhysicsBarrier::AddJob(const JobHandle& inJob)
{
  bool releaseSemaphore = false;
  Job* job = inJob.GetPtr();

  if (job->SetBarrier(this))
  {
    this->numToAcquire++;
    if (job->CanBeExecuted())
    {
      releaseSemaphore = true;
      this->numToAcquire++;
    }

    job->AddRef();
    uint writeIndex = this->jobWriteIndex++;
    while (writeIndex - this->jobReadIndex >= this->maxJobs)
    {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    this->jobs[writeIndex & (this->maxJobs - 1)] = job;
  }
  
  if (releaseSemaphore)
    this->semaphore.release();
}

void 
Strontium::PhysicsThreadPool::PhysicsBarrier::AddJobs(const JobHandle* inHandles, uint inNumHandles)
{
  bool releaseSemaphore = false;

  for (const JobHandle* handle = inHandles, *handles_end = inHandles + inNumHandles; handle < handles_end; ++handle)
  {
    Job* job = handle->GetPtr();

    if (job->SetBarrier(this))
    {
      this->numToAcquire++;
      if (!releaseSemaphore && job->CanBeExecuted())
      {
        releaseSemaphore = true;
        this->numToAcquire++;
      }

      job->AddRef();
      uint writeIndex = this->jobWriteIndex++;
      while (writeIndex - this->jobReadIndex >= this->maxJobs)
      {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }

      this->jobs[writeIndex & (this->maxJobs - 1)] = job;
    }
  }

  if (releaseSemaphore)
    this->semaphore.release();
}

void 
Strontium::PhysicsThreadPool::PhysicsBarrier::OnJobFinished(Job* inJob)
{
  this->semaphore.release();
}

void 
Strontium::PhysicsThreadPool::PhysicsBarrier::wait()
{
  while (this->numToAcquire > 0)
  {
    bool hasExecuted;
    do
    {
      hasExecuted = false;

      while (this->jobReadIndex < this->jobWriteIndex)
      {
        std::atomic<Job*>& job = this->jobs[this->jobReadIndex & (this->maxJobs - 1)];
        Job* jobptr = job.load();
        if (jobptr == nullptr || !jobptr->IsDone())
          break;

        jobptr->Release();
        job = nullptr;
        ++this->jobReadIndex;
      }

      for (uint index = this->jobReadIndex; index < jobWriteIndex; ++index)
      {
        std::atomic<Job*>& job = this->jobs[index & (this->maxJobs - 1)];
        Job* jobptr = job.load();
        if (jobptr != nullptr && jobptr->CanBeExecuted())
        {
          jobptr->Execute();
          hasExecuted = true;
          break;
        }
      }

    } while (hasExecuted);
  }

  int toAcquire = std::max(1, this->semaphore.getValue());
  this->semaphore.acquire(toAcquire);
  this->numToAcquire -= numToAcquire;

  while (this->jobReadIndex < this->jobWriteIndex)
  {
    std::atomic<Job*>& job = this->jobs[this->jobReadIndex & (this->maxJobs - 1)];
    Job* jobptr = job.load();
    assert(("All jobs should be freed.", jobptr != nullptr && jobptr->IsDone()));
    jobptr->Release();
    job = nullptr;
    ++this->jobReadIndex;
  }
}

//------------------------------------------------------------------------------
// Threadpool specifically for physics.
//------------------------------------------------------------------------------
Strontium::PhysicsThreadPool::PhysicsThreadPool(uint maxNumBarriers)
  : maxNumBarriers(maxNumBarriers)
{
  this->barriers = new Strontium::PhysicsThreadPool::PhysicsBarrier[maxNumBarriers];
}

Strontium::PhysicsThreadPool::~PhysicsThreadPool()
{
  delete[] this->barriers;
}

int 
Strontium::PhysicsThreadPool::GetMaxConcurrency() const
{
  return Strontium::JobSystem::getMaxConcurrency();
}

JPH::JobHandle
Strontium::PhysicsThreadPool::CreateJob(const char* inName, JPH::ColorArg inColor, const JobFunction &inJobFunction, JPH::uint32 inNumDependencies)
{
  Job* newJob = new Job(inName, inColor, this, inJobFunction, inNumDependencies);
  JobHandle handle(newJob);

  if (inNumDependencies == 0)
    this->QueueJob(newJob);

  return handle;
}

JPH::JobSystem::Barrier*
Strontium::PhysicsThreadPool::CreateBarrier()
{
  for (uint i = 0; i < this->maxNumBarriers; ++i)
  {
    bool expected = false;
    if (this->barriers[i].inUse.compare_exchange_strong(expected, true))
      return &this->barriers[i];
  }

  return nullptr;
}

void 
Strontium::PhysicsThreadPool::DestroyBarrier(Barrier* inBarrier)
{
  assert(("Barrier must be empty.", static_cast<Strontium::PhysicsThreadPool::PhysicsBarrier*>(inBarrier)->isEmpty()));

  bool expected = true;
  static_cast<Strontium::PhysicsThreadPool::PhysicsBarrier*>(inBarrier)->inUse.compare_exchange_strong(expected, false);
  assert(expected);
}

void 
Strontium::PhysicsThreadPool::WaitForJobs(Barrier* inBarrier)
{
  static_cast<Strontium::PhysicsThreadPool::PhysicsBarrier*>(inBarrier)->wait();
}

void 
Strontium::PhysicsThreadPool::queueJobInternal(Job* job)
{
  job->AddRef();

  auto jobFunction = [](Job* job)
  {
    job->Execute();
    job->Release();
  };

  Strontium::JobSystem::push(jobFunction, job);
}

void 
Strontium::PhysicsThreadPool::QueueJob(Job* inJob)
{
  this->queueJobInternal(inJob);
}

void 
Strontium::PhysicsThreadPool::QueueJobs(Job** inJobs, uint inNumJobs)
{
  for (Job **job = inJobs, **job_end = inJobs + inNumJobs; job < job_end; ++job)
    this->queueJobInternal(*job);
}

void 
Strontium::PhysicsThreadPool::FreeJob(Job* inJob)
{
  delete inJob;
}
