#include "DirectionalLight.h"

namespace cgb
{

	DirectionalLight::DirectionalLight(const glm::vec3& color, const glm::vec3& direction)
		: m_light_direction(glm::normalize(direction)),
		m_light_color(color),
		m_enabled(true)
	{
		
	}

	DirectionalLight::~DirectionalLight()
	{
	}


	void DirectionalLight::set_light_direction(const glm::vec3& direction)
	{
		m_light_direction = glm::normalize(direction);
	}

	void DirectionalLight::set_light_direction(Transform& transform)
	{
		m_light_direction = transform.GetFrontVector();
	}

	void DirectionalLight::set_light_color(glm::vec3 color)
	{
		m_light_color = std::move(color);
	}

	void DirectionalLight::set_enabled(bool is_enabled)
	{
		m_enabled = is_enabled;
	}

	DirectionalLightGpuData DirectionalLight::GetGpuData() const
	{
		DirectionalLightGpuData gdata;
		FillGpuDataIntoTarget(gdata);
		return gdata;
	}

	DirectionalLightGpuData DirectionalLight::GetGpuData(const glm::mat3& nrm_mat) const
	{
		DirectionalLightGpuData gdata;
		FillGpuDataIntoTarget(gdata, nrm_mat);
		return gdata;
	}

	void DirectionalLight::FillGpuDataIntoTarget(DirectionalLightGpuData& target) const
	{
		if (m_enabled)
		{
			target.m_light_dir_vs = glm::vec4(m_light_direction, 0.0f);
			target.m_light_color = glm::vec4(m_light_color, 1.0f);
		}
		else
		{
			target.m_light_dir_vs = glm::vec4(1.0f);
			target.m_light_color = glm::vec4(0.0f);
		}
	}

	void DirectionalLight::FillGpuDataIntoTarget(DirectionalLightGpuData& target, const glm::mat3& nrm_mat) const
	{
		if (m_enabled)
		{
			target.m_light_dir_vs = glm::vec4(nrm_mat * m_light_direction, 0.0f);
			target.m_light_color = glm::vec4(m_light_color, 1.0f);
		}
		else
		{
			target.m_light_dir_vs = glm::vec4(1.0f);
			target.m_light_color = glm::vec4(0.0f);
		}
	}

	DirectionalLight::operator DirectionalLightGpuData() const
	{
		return GetGpuData();
	}
}
