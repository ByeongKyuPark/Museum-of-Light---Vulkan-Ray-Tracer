#include "SceneList.hpp"
#include "Assets/Material.hpp"
#include "Assets/Model.hpp"
#include "Assets/Texture.hpp"
#include <functional>
#include <random>
#include <cmath>

using namespace glm;
using Assets::Material;
using Assets::Model;
using Assets::Texture;

namespace
{

}

const std::vector<std::pair<std::string, std::function<SceneAssets (SceneList::CameraInitialSate&)>>> SceneList::AllScenes =
{
	{"Interactive Gallery Scene", InteractiveGalleryScene},
};

SceneAssets SceneList::InteractiveGalleryScene(CameraInitialSate& camera)
{
	// Interactive Gallery Scene - A sophisticated Cornell Box-inspired scene
	// Demonstrates advanced ray tracing effects: global illumination, color bleeding, realistic materials
	// Perfect for showcasing technical skills in a CG portfolio
	
	camera.ModelView = lookAt(vec3(-1, 3, 6), vec3(0, 3.5, 0), vec3(0, 1, 0));
	camera.FieldOfView = 90;
	camera.Aperture = 0.02f;
	camera.FocusDistance = 6.0f;
	camera.ControlSpeed = 3.0f;
	camera.GammaCorrection = true;
	camera.HasSky = false; // Disable sky for controlled lighting

	const auto identity = mat4(1);
	std::vector<Model> models;
	std::vector<Texture> textures;

	// Gallery floor (white - Cornell Box style)
	models.push_back(Model::CreateBox(
		vec3(-7.5f, -0.1f, -7.5f), 
		vec3(7.5f, 0, 7.5f), 
		Material::Lambertian(vec3(0.73f, 0.73f, 0.73f))
	));

	// Gallery walls - Cornell Box style
	// Back wall (white)
	models.push_back(Model::CreateBox(
		vec3(-7.5f, 0, -7.5f), 
		vec3(7.5f, 8, -7.25f), 
		Material::Lambertian(vec3(0.73f, 0.73f, 0.73f))
	));
	
	// Left wall (RED - classic Cornell Box)
	models.push_back(Model::CreateBox(
		vec3(-7.5f, 0, -7.5f), 
		vec3(-7.25f, 8, 7.5f), 
		Material::Lambertian(vec3(0.65f, 0.05f, 0.05f))
	));
	
	// Right wall (GREEN - classic Cornell Box)
	models.push_back(Model::CreateBox(
		vec3(7.25f, 0, -7.5f), 
		vec3(7.5f, 8, 7.5f), 
		Material::Lambertian(vec3(0.12f, 0.45f, 0.15f))
	));
	
	// Front wall (BLUE - behind camera)
	models.push_back(Model::CreateBox(
		vec3(-7.5f, 0, 7.25f), 
		vec3(7.5f, 8, 7.5f), 
		Material::Lambertian(vec3(0.15f, 0.25f, 0.65f))
	));

	// Ceiling (white - Cornell Box style)
	models.push_back(Model::CreateBox(
		vec3(-7.5f, 7.5f, -7.5f), 
		vec3(7.5f, 8, 7.5f), 
		Material::Lambertian(vec3(0.73f, 0.73f, 0.73f))
	));

	// Central lighting panels (emissive) - Much more realistic lighting
	models.push_back(Model::CreateBox(
		vec3(-3, 7.4f, -3), 
		vec3(-1, 7.45f, -1), 
		Material::DiffuseLight(vec3(0.8f, 0.8f, 0.7f))
	));
	
	models.push_back(Model::CreateBox(
		vec3(1, 7.4f, -3), 
		vec3(3, 7.45f, -1), 
		Material::DiffuseLight(vec3(0.8f, 0.8f, 0.7f))
	));
	
	models.push_back(Model::CreateBox(
		vec3(-3, 7.4f, 1), 
		vec3(-1, 7.45f, 3), 
		Material::DiffuseLight(vec3(0.8f, 0.8f, 0.7f))
	));
	
	models.push_back(Model::CreateBox(
		vec3(1, 7.4f, 1), 
		vec3(3, 7.45f, 3), 
		Material::DiffuseLight(vec3(0.8f, 0.8f, 0.7f))
	));

	// Art installations and sculptures

	// 1. Crystal sculpture
	models.push_back(Model::CreateSphere(
		vec3(-4, 2.5, -4), 
		2.0f, 
		Material::Dielectric(1.8f), 
		true
	));

	// 2. Metallic abstract sculpture
	models.push_back(Model::CreateSphere(
		vec3(4, 1.6, -4), 
		1.6f, 
		Material::Metallic(vec3(0.9f, 0.7f, 0.3f), 0.0f), 
		true
	));

	// Remove scattered colorful spheres - keep scene clean for Cornell Box demo

	// 3. Glass cube installation
	models.push_back(Model::CreateBox(
		vec3(3, 0, 3), 
		vec3(5, 1.15f, 5), 
		Material::Lambertian(vec3(0.15f, 0.12f, 0.1f))
	));
	
	// Glass cube on pedestal
	models.push_back(Model::CreateBox(
		vec3(3.f, 1.2f, 3.f), 
		vec3(5.f, 3.4f, 5.f), 
		Material::Dielectric(1.5f)
	));
	
	// Floating metallic sphere inside glass cube
	models.push_back(Model::CreateSphere(
		vec3(4, 2.2f, 4), 
		0.95f, 
		Material::Metallic(vec3(0.7f, 0.9f, 0.7f), 0.0f), 
		true
	));

	// Add Lucy sculpture as museum centerpiece
	auto lucy = Model::LoadModel("../assets/models/lucy.obj");
	
	// Create pedestal for Lucy 
	models.push_back(Model::CreateBox(
		vec3(-1.2f, 0, -0.7f), 
		vec3(1.2f, 1.f, 0.7f), 
		Material::Lambertian(vec3(0.2f, 0.18f, 0.15f))
	));
	
	// Glass enclosure for Lucy statue (scaled to fit)
	models.push_back(Model::CreateBox(
		vec3(-1.5f, 0.f, -1.f), 
		vec3(1.5f, 1.2f, 1.f), 
		Material::Dielectric(1.5f)
	));
	
	const float lucyScale = 0.006f;
	
	// Transform Lucy: scale, translate to pedestal, rotate for best viewing angle
	lucy.Transform(
		rotate(
			scale(
				translate(identity, vec3(0, 1.f, 0)), 
				vec3(lucyScale)),
			radians(45.0f), vec3(0, 1, 0)));
	
	// Give Lucy a beautiful bronze-like material
	lucy.SetMaterial(Material::Metallic(vec3(0.7f, 0.5f, 0.3f), 0.1f));
	
	models.push_back(std::move(lucy));

	// Load textures
	textures.push_back(Texture::LoadTexture("../assets/textures/land_ocean_ice_cloud_2048.png", Vulkan::SamplerConfig()));
	textures.push_back(Texture::LoadTexture("../assets/textures/2k_mars.jpg", Vulkan::SamplerConfig()));
	textures.push_back(Texture::LoadTexture("../assets/textures/2k_moon.jpg", Vulkan::SamplerConfig()));

	return std::forward_as_tuple(std::move(models), std::move(textures));
}
