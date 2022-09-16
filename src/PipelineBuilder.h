#ifndef AV_PIPELINE_BUILDER
#define AV_PIPELINE_BUILDER
// Actually copied from https://vkguide.dev/docs/chapter-2/pipeline_walkthrough/
#include <vulkan/vulkan.h>
#include <vector>


namespace AVis {

	class PipelineBuilder {
	public:

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssembly;
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineRasterizationStateCreateInfo rasterizer;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineMultisampleStateCreateInfo multisampling;
		VkPipelineLayout pipelineLayout;

		VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);
	};



}; // namespace AVis

#endif // !AV_PIPELINE_BUILDER