#include "Interpolation.h"
#include "Application.h"

void InterpolationData::updatePointers() {
	// Part 1: update the pointer to the interpolation function
	switch (interp) {
	case InterpolationFunctionType::stepmin:
		this->f = stepmin;
		break;
	case InterpolationFunctionType::stepmax:
		this->f = stepmax;
		break;
	case InterpolationFunctionType::lerp:
		this->f = lerp;
		break;
	case InterpolationFunctionType::hermite:
		this->f = hermite;
		break;
	case InterpolationFunctionType::bounceOut:
		this->f = bounceOut;
		break;
	case InterpolationFunctionType::customInterp:
		// No change to f: assume it already has the correct function pointer
		break;
	default:
		this->f = nullptr;
		break;
	}

	// Part 2: to update the pointer to the property being interpolated, we use three things:
	// the shape, the channel, and the target. 
	// 
	// Part 2.1: find the channel's offset. The channel is just telling us which dimension
	// X, Y, Z, (and Alpha or A for colors) to use when animating the target. If the target
	// were a float array (and it really is just a float array, no matter what), then 
	// the offset tells us how far into the float array we should look to find the right 
	// property pointer. 
	int offset = 0;
	switch (channel) {
	case InterpolationChannel::X:
		offset = 0;
		break;
	case InterpolationChannel::Y:
		offset = 1;
		break;
	case InterpolationChannel::Z:
		offset = 2;
		break;
	case InterpolationChannel::Alpha:
		offset = 3;
		break;
	default:
		offset = 0;
		break;
	}

	// Part 2.2: Now that the channel offset is known, we can add it to the base pointer for
	// our target to get the exact location of the property we're interpolating.
	const float* data = nullptr;
	switch (target) {
	case InterpolationTarget::Position:
		data = glm::value_ptr(shape->localPosition);
		break;
	case InterpolationTarget::Rotation:
		data = glm::value_ptr(shape->localRotation);
		break;
	case InterpolationTarget::Scale:
		data = glm::value_ptr(shape->localScale);
		break;
	case InterpolationTarget::Color:
		data = glm::value_ptr(shape->color);
		break;
	}

	this->property = (float*)data + offset;
}

void to_json(json& j, const InterpolationData& s) {
	j = json{ {"shapeIndex", s.shape->index}, {"enabled", s.enabled}, {"startFrame", s.startFrame},
		{"endFrame", s.endFrame}, {"minValue", s.minValue}, {"maxValue", s.maxValue},
		{"target", s.target}, {"channel", s.channel}, {"interp", s.interp}
	};
}
void from_json(const json& j, InterpolationData& s) {
	int shapeIndex = -1;
	j.at("shapeIndex").get_to(shapeIndex);

	auto& app = Application::get();
	for (auto& object : app.objects) {
		if (object.index == shapeIndex) {
			s.shape = &object
				;
		}
	}
	j.at("enabled").get_to(s.enabled);
	j.at("startFrame").get_to(s.startFrame);
	j.at("endFrame").get_to(s.endFrame);
	j.at("minValue").get_to(s.minValue);
	j.at("maxValue").get_to(s.maxValue);
	j.at("target").get_to(s.target);
	j.at("channel").get_to(s.channel);
	j.at("interp").get_to(s.interp);

	s.updatePointers();
}