#pragma once
#include "PCH.hpp"

#define ERROR_COL glm::fvec3(1.0,0.2,0.2)

void postMessage(std::string msg, glm::fvec3 col);

struct EngineConfig
{
	enum Group { SSAO_Group };
	enum Special { Render_Resolution };

	EngineConfig() : render(changedGroups, changedSpecials) {}

	struct Render
	{
		Render() = delete;
		Render(std::set<Group>& cg, std::set<Special>& cs) : changedGroups(cg), changedSpecials(cs), ssao(cg,cs) {}
		struct SSAO
		{
		public:
			SSAO() = delete;
			SSAO(std::set<Group>& cg, std::set<Special>& cs) : changedGroups(cg), changedSpecials(cs) {}

			void setSamples(int set) {
				if (set < 0) {
					postMessage("Invalid SSAO samples setting. Range [0,inf]", ERROR_COL);
					return;
				}
				samples = set;
				changedGroups.insert(SSAO_Group);
			}
			int getSamples() const { return samples; }

			void setSpiralTurns(int set) {
				if (set <= 0) {
					postMessage("Invalid SSAO spiralTurns setting. Range (0,inf]", ERROR_COL);
					return;
				}
				spiralTurns = set;
				changedGroups.insert(SSAO_Group);
			}
			int getSpiralTurns() const { return spiralTurns; }

			void setProjScale(float set) {
				if (set <= 0) {
					postMessage("Invalid SSAO projScale setting. Range (0,inf]", ERROR_COL);
					return;
				}
				projScale = set;
				changedGroups.insert(SSAO_Group);
			}
			float getProjScale() const { return projScale; }

			void setRadius(float set) {
				if (set <= 0) {
					postMessage("Invalid SSAO radius setting. Range (0,inf]", ERROR_COL);
					return;
				}
				radius = set;
				intensityInternal = intensity / std::pow(radius, 6);
				changedGroups.insert(SSAO_Group);
			}
			float getRadius() const { return radius; }

			void setBias(float set) {
				if (set < 0) {
					postMessage("Invalid SSAO bias setting. Range [0,inf]", ERROR_COL);
					return;
				}
				bias = set;
				changedGroups.insert(SSAO_Group);
			}
			float getBias() const { return bias; }

			void setIntensity(float set) {
				if (set < 0) {
					postMessage("Invalid SSAO intensity setting. Range [0,inf]", ERROR_COL);
					return;
				}
				intensity = set;
				intensityInternal = intensity / std::pow(radius, 6);
				changedGroups.insert(SSAO_Group);
			}
			float getIntensity() const { return intensity; }

			float getIntensityInternal() const { return intensityInternal; }

		private:
			int samples;
			int spiralTurns;
			float projScale;
			float radius;
			float bias;
			float intensity;
			float intensityInternal;

			std::set<Group>& changedGroups;
			std::set<Special>& changedSpecials;
		} ssao;

		void setResolution(glm::ivec2 set);
		glm::ivec2 getResolution() { return resolution; }

	private:
		glm::ivec2 resolution;

		std::set<Group>& changedGroups;
		std::set<Special>& changedSpecials;
	} render;

	std::set<Group> changedGroups;
	std::set<Special> changedSpecials;
};