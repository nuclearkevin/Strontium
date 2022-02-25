#pragma once

#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"

namespace Strontium
{
  //----------------------------------------------------------------------------
  // Synchronous timer. Result in pipeline stalls, use sparingly.
  //----------------------------------------------------------------------------
  class SynchTimer
  {
  public:
	SynchTimer();
	~SynchTimer();

	// Delete the copy constructor and the assignment operator. Prevents
	// issues related to the underlying API.
	SynchTimer(const SynchTimer&) = delete;
	SynchTimer(SynchTimer&&) = delete;
	SynchTimer& operator=(const SynchTimer&) = delete;
	SynchTimer& operator=(SynchTimer&&) = delete;

	// Begin and end functions. Always call begin before calling end. 
	// Always call end after you call begin. Don't call begin OR end twice in a row.
	void begin();
	void end();

	// Get the elapsed time.
	uint nsTimeElapsed();

	// Record a new timestamp into the provided float "storage".
	void msRecordTime(float& storage);
  private:
	uint queries[2];
  };

  //----------------------------------------------------------------------------
  // Asynchronous timer. Does not result in pipeline stalls.
  //----------------------------------------------------------------------------
  class AsynchTimer
  {
  public:
	AsynchTimer(uint numQueries);
	~AsynchTimer();

	// Delete the copy constructor and the assignment operator. Prevents
	// issues related to the underlying API.
	AsynchTimer(const AsynchTimer&) = delete;
	AsynchTimer(AsynchTimer&&) = delete;
	AsynchTimer& operator=(const AsynchTimer&) = delete;
	AsynchTimer& operator=(AsynchTimer&&) = delete;

	// Begin and end functions. Always call begin before calling end. 
	// Always call end after you call begin. Don't call begin OR end twice in a row.
	// High likelyhood that the results returned will be beind by a few frames.
	void begin();
	void end();

	// Fetch the oldest query if its available. Returns -1 if not available.
	int nsTimeElapsed();

	// Record a new timestamp (if available) into the provided float "storage".
	void msRecordTime(float &storage);
  private:
	uint start;
	uint count;
	const uint capacity;
	uint* queries; // 2 * capacity.
  };

  //----------------------------------------------------------------------------
  // Scoped timer. Wraps one of the timers above.
  //----------------------------------------------------------------------------
  template<class T>
  class ScopedTimer
  {
  public:
	ScopedTimer()
	{ }

	~ScopedTimer()
	{ }

  private:
  };

  template<>
  class ScopedTimer<SynchTimer>
  {
  public:
	ScopedTimer(SynchTimer& timer)
	  : timer(timer)
	{
	  this->timer.begin();
	}
	  
	~ScopedTimer()
	{
	  this->timer.end();
	}

  private:
	SynchTimer& timer;
  };

  template<>
  class ScopedTimer<AsynchTimer>
  {
  public:
	ScopedTimer(AsynchTimer& timer)
	  : timer(timer)
	{
	  this->timer.begin();
	}
	  
	~ScopedTimer()
	{
	  this->timer.end();
	}

  private:
	AsynchTimer& timer;
  };
}