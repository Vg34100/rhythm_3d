#pragma once

#include "globals.h"

#include <functional>
#include <array>

MAKE_ENUM(AnimationPlayState, int, Stopped, Paused, Playing)
MAKE_ENUM(AnimationKeyDomain, int, All, Selected)
MAKE_ENUM(AnimationKeyDisplay, int, All, ByFrame, ByShape)
MAKE_ENUM(AnimationKeyTarget, int, Skeleton, Mesh, Camera, Stroke)
MAKE_ENUM(AnimationKeyChannel, int, Translate, TranslateX, TranslateY, TranslateZ, Rotate, RotateX, RotateY, RotateZ, Scale, ScaleX, ScaleY, ScaleZ)
MAKE_ENUM(AnimationKeyBehavior, int, Overwrite, Merge)

class AnimationState
{
public:
	static AnimationState& get()
	{
		static AnimationState gs;
		return gs;
	}

	struct FrameRate
	{
		double fps = 0.0;
		double spf = 0.0;

		FrameRate(double f)
		{
			if (f > 0.0 && f == f)
			{
				fps = f;
				spf = 1.0 / f;
			}
		}
	};

	std::vector<FrameRate> commonRates =
	{
		FrameRate(12.0),
		FrameRate(16.0),
		FrameRate(24.0),
		FrameRate(29.97),
		FrameRate(30.0),
		FrameRate(50.0),
		FrameRate(60.0)
	};

	FrameRate frameRate{ 24.0 };

	AnimationPlayState playState = AnimationPlayState::Stopped;
	AnimationPlayState lastPlayState = AnimationPlayState::Stopped;

	int currentFrame = 0;
	int startFrame = 0;
	int endFrame = 360;
	int numFrames = 360;

	// Two modes for controlling animation time: discrete and continuous.
	// In discrete mode, the dt between every frame is (1 / frameRate), always.
	// In continuous mode, the dt between every frame is (now() - last()). So time
	// still changes in between frame number changes. 
	bool continuous = false;
	// This only applies to continuous time mode. If true, the playback will advance
	// by the real amount of time in between frames.
	// If false, it will advance by 1/framerate * timescale maximum
	bool skipFrames = true;

	// We can also influence the time scale to view things at half speed, 2x, etc.
	double timeScale = 1.0;

	bool frameChanged = false;
	bool backwards = false;
	//bool startSet = false;
	bool loop = true;
	// When playback started
	//_time started;
	// When the last update() occurred
	double last;
	// When the current frame number last changed
	double lastFrameChange;
	// For current update()
	double now;
	// between now and last in seconds
	double diff;
	// Current time position in seconds
	double timePosition;

	// Callback for when frame changes
	typedef std::function<void(AnimationState* state, int, int)> FrameChangedCallback;

	// Callback for when state changes
	using StateChangedCallback = std::function<void(AnimationState* state, AnimationPlayState, AnimationPlayState)>;

	std::vector<FrameChangedCallback> frameChangedCallbacks;
	std::vector<StateChangedCallback> stateChangedCallbacks;

	void frameCallbacks(int oldFrame, int newFrame)
	{
		for (auto& fc : frameChangedCallbacks)
		{
			fc(this, oldFrame, newFrame);
		}
	}

	void setState(AnimationPlayState newState);

	void initFromSeconds(double total, double fps)
	{
		setFrameRate(fps);
		setNumFrames(std::round<int>(total * fps));
	}

	double seconds()
	{
		return frameRate.spf * numFrames;
	}

	int lastFrame()
	{
		return std::min<int>(endFrame, numFrames);
	}

	void pause()
	{
		setState(AnimationPlayState::Paused);
	}
	void play()
	{
		setState(AnimationPlayState::Playing);

	}
	void stop()
	{
		setState(AnimationPlayState::Stopped);
	}

	void reset()
	{
		if (backwards)
		{
			currentFrame = lastFrame();
		}
		else
		{
			currentFrame = startFrame;
		}
		timePosition = (frameRate.spf * currentFrame);
		lastFrameChange = now;

		for (auto& sc : stateChangedCallbacks)
		{
			sc(this, AnimationPlayState::Playing, AnimationPlayState::Playing);
		}
	}

	void update();

	virtual void setFrame(int newFrame, bool suppressUpdate = false)
	{
		auto oldFrame = currentFrame;

		if (newFrame < startFrame || newFrame > lastFrame())
		{
			if (loop)
			{
				reset();
				frameChanged = true;
			}
			else
			{
				stop();
			}
		}
		else
		{
			frameChanged = currentFrame != newFrame;
			currentFrame = newFrame;
		}

		if (frameChanged)
		{
			frameCallbacks(oldFrame, newFrame);
		}
	}

	virtual void setFrameRate(double fr)
	{
		frameRate = FrameRate(fr);
	}

	virtual void setNumFrames(int nf)
	{
		if (nf > 0)
		{
			numFrames = nf;
			endFrame = nf;
		}
	}

	virtual void renderUI();
};

struct Keyframe {
	int frameNumber;
	AnimationKeyChannel channel;
	int shapeIndex;
	double value;
};

//typedef Anim::PlayState PlayState;
//typedef Anim::KeyTarget KeyTarget;
//typedef Anim::KeyBehavior KeyBehavior;
//typedef Anim::State AnimationState;