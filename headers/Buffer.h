#pragma once

#include "globals.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Buffer
{
public:
	GLuint buffer = 0;
	GLsizeiptr bufferSize = 0;
	GLenum bindTarget = GL_ARRAY_BUFFER;
	GLenum usagePattern = 0;
	GLvoid* bufferData = nullptr;

	bool shouldUpdateBuffer = true;

	Buffer() { }

	Buffer(GLenum target, GLsizeiptr size, GLvoid* data, GLenum usage)
		: bindTarget(target), bufferSize(size), bufferData(data), usagePattern(usage)
	{
		glGenBuffers(1, &buffer);
		update(true, true);
	}

	~Buffer()
	{
		if (buffer != 0)
		{
			auto gld = glDeleteBuffers;

			if (gld)
			{
				gld(1, &buffer);
				buffer = 0;
			}
			else
			{
				auto gldarb = glDeleteBuffersARB;
				if (gldarb)
				{
					gldarb(1, &buffer);
					buffer = 0;
				}
			}

		}
	}

	void bind()
	{
		glBindBuffer(bindTarget, buffer);
	}

	void bind(GLenum target)
	{
		glBindBuffer(target, buffer);
	}

	void update(bool force = false, bool first = false, int offset = 0, size_t bufSize = 0)
	{
		if (shouldUpdateBuffer || force)
		{
			bind();

			if (first)
			{
				glBufferData(bindTarget, bufferSize, bufferData, usagePattern);
			}
			else
			{
				glBufferSubData(bindTarget, offset, (bufSize == 0 ? bufferSize : bufSize), bufferData);
			}

			shouldUpdateBuffer = false;
		}
	}

	void fetch(GLint fetchTarget = -1)
	{
		if (fetchTarget == -1)
		{
			fetchTarget = bindTarget;
		}
		glGetBufferSubData(fetchTarget, 0, bufferSize, bufferData);
	}
};

using spBuffer = s_ptr<Buffer>;

// Object contains both the CPU and GPU access to data specified by the template type
template <typename T>
class VectorBuffer
{
public:
	std::vector<T> data;
	s_ptr<Buffer> buffer;

	VectorBuffer() { }

	// Allocates the storage with the desired amount and initializes the Buffer
	VectorBuffer(GLenum target, GLsizeiptr count, GLenum usage)
	{
		data.resize(count);
		initBuffer(target, usage);
	}

	VectorBuffer(GLenum target, const std::vector<T>& _data, GLenum usage)
		: data(_data)
	{
		initBuffer(target, usage);
	}

	VectorBuffer(GLenum target, std::vector<T>&& _data, GLenum usage)
		: data(_data)
	{
		initBuffer(target, usage);
	}

	~VectorBuffer() { }

	size_t size() const
	{
		return data.size();
	}

	bool empty() const
	{
		return data.empty();
	}

	bool refresh()
	{
		initBuffer(buffer->bindTarget, buffer->usagePattern);
		return (bool)buffer;
	}

	void initBuffer(GLenum target, GLenum usage)
	{
		buffer = s_ptr<Buffer>(new Buffer(target, sizeof(T) * data.size(), (GLvoid*)data.data(), usage));
	}

	void bind()
	{
		// Explicitly not checking for null pointer because if this fails, the program deserves 
		// to crash
		buffer->bind();
	}

	void update(int offset = 0, int count = 1)
	{
		buffer->bind();
		glBufferSubData(buffer->bindTarget, offset * sizeof(T), count * sizeof(T), data.data() + offset);
	}

};

template<typename T>
using spVectorBuffer = s_ptr<VectorBuffer<T>>;