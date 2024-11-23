#include "AnimationGlobals.h"
#include "Application.h"

#include <GL/glew.h>
#include "GLFW/glfw3.h"

#include "imgui.h"

void AnimationState::setState(AnimationPlayState newState)
{
	if (playState != newState)
	{
		auto oldState = playState;
		for (auto& sc : stateChangedCallbacks)
		{
			sc(this, oldState, newState);
		}
		playState = newState;
		lastFrameChange = Application::get().timeSinceStart;
	}
}

void AnimationState::update()
{
	last = now;
	now = Application::get().timeSinceStart;
	if (playState == +AnimationPlayState::Playing)
	{
		//if (!startSet && (lastPlayState != playState 
		//	|| (currentFrame == startFrame && !backwards)
		//	|| (currentFrame == std::min<int>(endFrame, numFrames) && backwards)))
		//{
		//	started = now;
		//	last = now;
		//	startSet = true;
		//	reset();
		//	
		//}


		auto realDt = now - last;

		double dts = (realDt * timeScale * (backwards ? -1.0 : 1.0));

		// Discrete: update the frame when the real amount of time for it passes
		if (!continuous)
		{
			frameChanged = false;
			// Increment current frame when timer says. 
			// Note: using an if statement here means the current time/frame number will change at most once
			// per update() call, but using a while statement will let it increment until it "catches up". 
			// So a long-running operation that takes a long time on one frame will not interfere with the
			// real-time playback when a while is used. This means the animation looks like it's skipping around.
			// Using an if statement lets us always see the frames in between the would-be, real-time skips.

			double sinceLastFrame = ((now - lastFrameChange) * timeScale);

			if (sinceLastFrame >= (frameRate.spf))
			{
				int df = (backwards ? -1 : 1);
				do
				{
					sinceLastFrame -= frameRate.spf;
					lastFrameChange = now;

					setFrame(currentFrame + df);
					timePosition += (frameRate.spf * df);
				} while (skipFrames && sinceLastFrame >= frameRate.spf);
			}
		}
		// Continuous mode
		else
		{
			if (dts > frameRate.spf && !skipFrames)
			{
				dts = frameRate.spf;
			}

			timePosition += dts;
			int newFrame = (int)glm::floor(timePosition / frameRate.spf);

			if (currentFrame != newFrame)
			{
				setFrame(newFrame);
			}
		}

		/*
		diff = _elapsed((now - last).count() * timeScale * (backwards ? -1.0 : 1.0));

		if (continuous)
		{
			if (!skipFrames)
			{
				auto maxDiff = frameRate.spf * timeScale * (backwards ? -1.0 : 1.0);
				if (glm::abs(diff.count()) > glm::abs(maxDiff.count()))
				{
					diff = maxDiff;
				}
			}

			timePosition += diff;

		}

		_elapsed sinceLastFrame = _elapsed((now - lastFrameChange).count() * timeScale);

		frameChanged = false;
		// Increment current frame when timer says.
		// Note: using an if statement here means the current time/frame number will change at most once
		// per update() call, but using a while statement will let it increment until it "catches up".
		// So a long-running operation that takes a long time on one frame will not interfere with the
		// real-time playback when a while is used. This means the animation looks like it's skipping around.
		// Using an if statement lets us always see the frames in between the would-be, real-time skips.
		if (sinceLastFrame >= (frameRate.spf / timeScale))
		{
			setFrame(currentFrame + (backwards ? -1 : 1));

			if (continuous)
			{
				lastFrameChange = now;
				sinceLastFrame -= (skipFrames ? frameRate.spf : diff );
			}
			else
			{
				lastFrameChange = now;
				sinceLastFrame -= frameRate.spf;
				timePosition += frameRate.spf;
			}

			//lastFrameChange = now;
			//sinceLastFrame -= frameRate.spf;
			frameChanged = true;
		}
		*/

		lastPlayState = playState;
	}
	else
	{
		// Set diff for fbxtime based on current frame and start
		auto frameDiff = (double)currentFrame / frameRate.fps;
		timePosition = frameDiff;
	}
}

void AnimationState::renderUI()
{
	// Create playbar
	auto cf = this->currentFrame;
	ImGui::Text("Playbar");
	ImGui::SameLine();
	if (ImGui::SliderInt("##HiddenPlaybarLabel", &cf, 0, this->numFrames))
	{
		this->setFrame(cf);
	}

	ImGui::SameLine();
	ImGui::Text("Time: %f", this->timePosition);

	// First frame button
	if (ImGui::Button("<<"))
	{
		this->setFrame(0);
	}

	// Previous frame button
	ImGui::SameLine();
	if (ImGui::Button("<"))
	{
		this->setFrame(this->currentFrame - 1);
	}

	// What text to display for each state
	static std::map<AnimationPlayState, std::string> playPauseMap =
	{
		{ AnimationPlayState::Paused, "|>" },
		{ AnimationPlayState::Stopped, "|>" },
		{ AnimationPlayState::Playing, "||" }
	};

	// Same button for play/pause
	ImGui::SameLine();
	if (ImGui::Button(playPauseMap[this->playState].c_str()))
	{
		if (this->playState == +AnimationPlayState::Playing)
		{
			this->pause();
		}
		else
		{
			this->play();
		}
	}

	// Stop button
	ImGui::SameLine();
	if (ImGui::Button("[]"))
	{
		this->stop();
		this->setFrame(0);
	}

	// Next frame button
	ImGui::SameLine();
	if (ImGui::Button(">"))
	{
		this->setFrame(this->currentFrame + 1);
	}

	// Go to end buttons
	ImGui::SameLine();
	if (ImGui::Button(">>"))
	{
		this->setFrame(this->numFrames);
	}

	// Loop and time option
	ImGui::Checkbox("Loop", &this->loop);
	ImGui::SameLine();
	if (ImGui::Button(this->continuous ? "Continuous" : "Discrete"))
	{
		this->continuous = !this->continuous;
	}
	ImGui::SameLine();
	ImGui::PushItemWidth(40.0f);
	ImGui::InputDouble("Time scale", &this->timeScale);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Checkbox("Backwards", &this->backwards);
	ImGui::Checkbox("Skip frames", &this->skipFrames);

	// Start and end frames
	ImGui::PushItemWidth(100.0f);
	ImGui::InputInt("Start frame", &this->startFrame);
	ImGui::SameLine();
	ImGui::InputInt("End frame", &this->endFrame);
	ImGui::SameLine();
	int nf = this->numFrames;
	if (ImGui::InputInt("Num frames", &nf))
	{
		this->setNumFrames(nf);
	}
	double fps = this->frameRate.fps;
	if (ImGui::InputDouble("FPS", &fps))
	{
		this->setFrameRate(fps);
	}
	ImGui::SameLine();
	ImGui::Text("SPF: %f", this->frameRate.spf);

	if (ImGui::BeginCombo("Defaults", fmt::format("{0}", this->frameRate.fps).c_str()))
	{
		for (auto& preset : this->commonRates)
		{
			ImGui::PushID((const void*)&preset);
			bool s = this->frameRate.fps == preset.fps;
			auto str = fmt::format("{0}", preset.fps);
			if (ImGui::Selectable(str.c_str(), &s))
			{
				this->setFrameRate(preset.fps);
			}
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}

	ImGui::PopItemWidth();
}