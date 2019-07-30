#include "shader_vulkan.h"

namespace cgb
{
	shader shader::prepare(shader_info pInfo)
	{
		shader result;
		result.mInfo = std::move(pInfo);
		result.mTracker.setTrackee(result);
		return result;
	}

	shader shader::create(shader_info pInfo)
	{
		auto shdr = shader::prepare(std::move(pInfo));
		try {
			shdr.mShaderModule = shader::build_from_file(shdr.info().mPath);
		}
		catch (std::runtime_error&) {
			std::string secondTry = shdr.info().mPath + ".spv";
			shdr.mShaderModule = shader::build_from_file(shdr.info().mPath);
			binFileContents = cgb::load_binary_file(secondTry);
			LOG_INFO(fmt::format("Couldn't load '{}' but loading '{}' was successful => going to use the latter, fyi!", pPath, secondTry));
		}
		return shdr;
	}

	vk::UniqueShaderModule shader::build_from_file(const std::string& pPath)
	{
		auto binFileContents = cgb::load_binary_file(pPath);
		return shader::build_from_binary_code(binFileContents);
	}

	vk::UniqueShaderModule shader::build_from_binary_code(const std::vector<char>& pCode)
	{
		auto createInfo = vk::ShaderModuleCreateInfo()
			.setCodeSize(pCode.size())
			.setPCode(reinterpret_cast<const uint32_t*>(pCode.data()));

		return context().logical_device().createShaderModuleUnique(createInfo);
	}

	bool shader::has_been_built() const
	{
		return static_cast<bool>(mShaderModule);
	}

}
