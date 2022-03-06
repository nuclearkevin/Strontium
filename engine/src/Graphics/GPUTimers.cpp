#include "Graphics/GPUTimers.h"

// OpenGL includes.
#include "glad/glad.h"

namespace Strontium
{
  //----------------------------------------------------------------------------
  // Synchronous timer.
  //----------------------------------------------------------------------------
  SynchTimer::SynchTimer()
  {
	glGenQueries(2, this->queries);
  }

  SynchTimer::~SynchTimer()
  {
	glDeleteQueries(2, this->queries);
  }

  void 
  SynchTimer::begin()
  {
	glQueryCounter(queries[0], GL_TIMESTAMP);
  }

  void 
  SynchTimer::end()
  {
	glQueryCounter(queries[1], GL_TIMESTAMP);
  }

  uint 
  SynchTimer::nsTimeElapsed()
  {
	// Block until timing results are available.
	GLint startResultAvailable = GL_FALSE;
	GLint endResultAvailable = GL_FALSE;
	while (!startResultAvailable || !endResultAvailable)
	{
	  glGetQueryObjectiv(queries[0], GL_QUERY_RESULT_AVAILABLE, &startResultAvailable);
	  glGetQueryObjectiv(queries[1], GL_QUERY_RESULT_AVAILABLE, &endResultAvailable);
	}

	GLuint64 startTimestamp = 0;
	GLuint64 endTimestamp = 0;
	glGetQueryObjectui64v(queries[0], GL_QUERY_RESULT, &startTimestamp);
	glGetQueryObjectui64v(queries[1], GL_QUERY_RESULT, &endTimestamp);
	return static_cast<uint>(endTimestamp - startTimestamp);
  }

  void 
  SynchTimer::msRecordTime(float& storage)
  {
	storage = static_cast<float>(this->nsTimeElapsed()) / 1e6f;
  }

  //----------------------------------------------------------------------------
  // Asynchronous timer.
  //----------------------------------------------------------------------------
  AsynchTimer::AsynchTimer(uint numQueries)
	: capacity(numQueries)
	, start(0)
	, count(0)
  {
	assert(("Must generate at least one pair of queries.", !(numQueries <= 0)));

	this->queries = new uint[2 * numQueries];
	glGenQueries(2 * numQueries, queries);
  }

  AsynchTimer::~AsynchTimer()
  {
	glDeleteQueries(2 * this->capacity, this->queries);
	delete[] this->queries;
  }

  void 
  AsynchTimer::begin()
  {
	if (this->count < this->capacity)
	  glQueryCounter(this->queries[this->start], GL_TIMESTAMP);
  }

  void 
  AsynchTimer::end()
  {
	if (this->count < this->capacity)
	{
	  glQueryCounter(this->queries[this->start + this->capacity], GL_TIMESTAMP);
	  this->start = (this->start + 1) % this->capacity; // Wrap around to the first query.
	  this->count++;
	}
  }

  // Fetch the oldest query if its available. Returns -1 if not available.
  int 
  AsynchTimer::nsTimeElapsed()
  {
	if (this->count == 0)
	  return -1;

	uint index = (this->start + this->capacity - this->count) % this->capacity;

	// Check to see if the timer queries are available.
	GLint startResultAvailable = GL_FALSE;
	GLint endResultAvailable = GL_FALSE;
	glGetQueryObjectiv(this->queries[index], GL_QUERY_RESULT_AVAILABLE, &startResultAvailable);
	glGetQueryObjectiv(this->queries[index + this->capacity], GL_QUERY_RESULT_AVAILABLE, &endResultAvailable);

	// Queries aren't available yet.
	if (startResultAvailable == GL_FALSE || endResultAvailable == GL_FALSE)
    {
      return -1;
    }

	// Queries are available. Pop and return the oldest queries.
	this->count--;
	GLuint64 startTimestamp = 0;
	GLuint64 endTimestamp = 0;
	glGetQueryObjectui64v(this->queries[index], GL_QUERY_RESULT, &startTimestamp);
	glGetQueryObjectui64v(this->queries[index + this->capacity], GL_QUERY_RESULT, &endTimestamp);

	return static_cast<int>(endTimestamp - startTimestamp);
  }

  void 
  AsynchTimer::msRecordTime(float& storage)
  {
	int nsTime = this->nsTimeElapsed();
	storage = nsTime > -1 ? static_cast<float>(nsTime) / 1e6f : storage;
  }
}