#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

// Jolt includes.
#include "Jolt/Jolt.h"
#include "Jolt/Core/JobSystem.h"

// STL includes.
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace Strontium::PhysicsEngine
{
  // Implements the Jolt jobsystem API on top of Strontium's jobsystem. 
  // Its an adapted version of Jolt/Core/JobSystemThreadPool.h 
  class ThreadPool final : public JPH::JobSystem
  {
  public:
	ThreadPool(uint maxNumBarriers);
	~ThreadPool() override;

	int GetMaxConcurrency() const override;

	JobHandle CreateJob(const char* inName, JPH::ColorArg inColor, const JobFunction &inJobFunction, JPH::uint32 inNumDependencies = 0) override;

	Barrier* CreateBarrier() override;
	void DestroyBarrier(Barrier* inBarrier) override;
	void WaitForJobs(Barrier* inBarrier) override;

  private:
	// Inner class emulating a semaphore.
	class Semaphore
	{
	public:
	  // TODO: OS specific implementations.
	  inline Semaphore();
	  inline ~Semaphore() = default;

	  inline void release(uint inNumber = 1);
	  inline void acquire(uint inNumber = 1);

	  inline int getValue() const { return this->count; }
	private:
	  std::mutex lock;
	  std::condition_variable waitVariable;
	  int count;
	};

	// Inner barrier class.
	class PhysicsBarrier : public Barrier
	{
	public:
	  PhysicsBarrier();
	  ~PhysicsBarrier() override;

	  void AddJob(const JobHandle& inJob) override;
	  void AddJobs(const JobHandle* inHandles, uint inNumHandles) override;

	  inline bool isEmpty() const { return this->jobReadIndex == this->jobWriteIndex; }

	  void wait();

	  std::atomic<bool> inUse;
	private:
	  void OnJobFinished(Job* inJob) override;

	  static constexpr uint maxJobs = 1024;
	  std::atomic<Job*> jobs[maxJobs];
	  alignas(JPH_CACHE_LINE_SIZE) std::atomic<uint> jobReadIndex;
	  alignas(JPH_CACHE_LINE_SIZE) std::atomic<uint> jobWriteIndex;
	  std::atomic<int> numToAcquire;
	  Semaphore semaphore;
	};

	void queueJobInternal(Job* job);

	void QueueJob(Job* inJob) override;
	void QueueJobs(Job** inJobs, uint inNumJobs) override;
	void FreeJob(Job* inJob) override;

	const uint maxNumBarriers;
	PhysicsBarrier* barriers;
  };
}