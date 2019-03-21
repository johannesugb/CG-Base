#pragma once

namespace cgb
{
	struct PointLightGpuData
	{
		glm::vec4 m_position;
		glm::vec4 m_light_color;
		glm::vec4 m_attenuation;
	};

	class PointLight
	{
	public:
		PointLight(const glm::vec3& color, const glm::vec3& position);
		PointLight(const glm::vec3& color, const glm::vec3& position, const glm::vec4& attenuation);
		PointLight(const glm::vec3& color, const glm::vec3& position, float const_atten, float lin_atten, float quad_atten, float cub_atten);
		PointLight(const glm::vec3& color, Transform transform, float const_atten, float lin_atten, float quad_atten, float cub_atten);
		PointLight(const PointLight& other) noexcept = default;
		PointLight(PointLight&& other) noexcept = default;
		PointLight& operator=(const PointLight& other) noexcept = default;
		PointLight& operator=(PointLight&& other) noexcept = default;
		~PointLight();

		glm::vec3 position() const { return m_position; }
		const glm::vec3& light_color() const { return m_light_color; }
		glm::vec4 attenuation() const { return m_attenuation;  }
		float const_attenuation() const { return m_attenuation.x; }
		float linear_attenuation() const { return m_attenuation.y; }
		float quadratic_attenuation() const { return m_attenuation.z; }
		float cubic_attenuation() const { return m_attenuation.w; }
		//Transform& transform() { return m_transform; }
		bool enabled() const { return m_enabled; }

		void set_position(glm::vec3 position);
		void set_light_color(glm::vec3 color);
		void set_attenuation(glm::vec4 attenuation);
		void set_const_attenuation(float attenuation);
		void set_linear_attenuation(float attenuation);
		void set_quadratic_attenuation(float attenuation);
		void set_cubic_attenuation(float attenuation);
		void set_enabled(bool is_enabled);

		PointLightGpuData GetGpuData() const;
		PointLightGpuData GetGpuData(const glm::mat4& mat) const;
		void FillGpuDataIntoTarget(PointLightGpuData& target) const;
		void FillGpuDataIntoTarget(PointLightGpuData& target, const glm::mat4& mat) const;

		operator PointLightGpuData() const;

	private:
		//Transform m_transform;
		glm::vec3 m_position;
		glm::vec3 m_light_color;
		glm::vec4 m_attenuation;
		bool m_enabled;
	};

}
