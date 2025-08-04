#pragma once
#include "Utilities/Glm.hpp"
#include <functional>
#include <string>
#include <tuple>
#include <vector>

namespace Assets
{
	class Model;
	class Texture;
}

typedef std::tuple<std::vector<Assets::Model>, std::vector<Assets::Texture>> SceneAssets;

class SceneList final
{
public:

	struct CameraInitialSate
	{
		glm::mat4 ModelView;
		float FieldOfView;
		float Aperture;
		float FocusDistance;
		float ControlSpeed;
		bool GammaCorrection;
		bool HasSky;
	};

	static SceneAssets InteractiveGalleryScene(CameraInitialSate& camera);

	static const std::vector<std::pair<std::string, std::function<SceneAssets (CameraInitialSate&)>>> AllScenes;
};
