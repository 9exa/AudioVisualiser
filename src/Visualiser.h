#ifndef AVIS_VISUALISER
#define AVIS_VISUALIZER

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <stdexcept>

#include <vector>
#include "VisualiserTypes.h"

namespace AVis {

class Visualiser {
	// Object in charge of rendering an implementation of an audio visualiser to a glfw window
	// For now just do a flat color visualiser
public:
	// Doesn't actually recieve audio data. It's GUI after all

	// Application requesting a redraw
	void resizeImage(int width, int height);

	//valled by validation layers to print errors to console
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	// Visualizer swap chain not ideal, it wants a resize
	bool requestingResize = false;

	//set up and fill up this renderer using the supplied vulkan instance
	void initVulkan(GLFWwindow* window, int width, int height);

	//read new peak info from the application into a form the shader can reader
	void updatePeaks(std::vector<Peak> peaks);

	// rotates the presented camera on the given axis
	void rotateCamera(float anglex, float angley);

	// moves the center of the camera in 3D space
	void translateCamera(float dx, float dy, float dz);

	//draw the current state
	void drawFrame();

	//destory all active resources
	void cleanup();

	

	// clear the current Imgui visuals from buffers/windows
	void newImguiFrame();

	//debug, print a property
	void printThing();

private:
	// I want to try bindless
	bool bindlessAvailable = false;
	// todo: move to Rendering Module. Abstract the heck out of the vulkan API

	struct QueueFamilyIndices {
		uint32_t graphicsFamily = UINT32_MAX;
		uint32_t presentFamily = UINT32_MAX;
		uint32_t transferFamily = UINT32_MAX;

		bool isComplete() {
			return (graphicsFamily != UINT32_MAX) &&
				(presentFamily != UINT32_MAX) &&
				(transferFamily != UINT32_MAX);
		}
	};

	//holds what descriptions a particular swapchain supports
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	//be explicit about alignment
	// When a certain sets of frequency ranges are recieved parts of a 2-d Plane will rise. Basically a height map
	// Abstracted from Application Wide Peak
	struct PeakUbo {
		// The epicenter of a rising point in the 2D plane
		//What formula should be used to calculate its curve
		alignas(4) Peak::Type type;
		// It's position on the plane
		alignas(8) glm::vec2 point = {0.0f, 0.0f};

		// How pronounced it is
		alignas(4) float energy = 0.0f;

		// The light waves in reflects 
		alignas(16) glm::vec4 color = Colors::Blue;
	};

	struct Vertex {
		static inline const uint32_t Size = 2;

		glm::vec3 pos;
		glm::vec2 uv;

		//function; not constant because we might want to change the binding
		static VkVertexInputBindingDescription getBindingDescription(uint32_t binding) {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static const std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			/*std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
				VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexData, pos)},
				VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexData, uv)}
			};*/
			return attributeDescriptions;
		}
	};

	static const std::vector<Vertex> vertices;
	static const std::vector<uint16_t> vIndices;

	struct MaterialUbo {
		//allowed to have either reflect or refract, not both
		alignas(4) float weight;
		alignas(4) float refractIndex = 1.0f;
	};
	//shove all the peaks data into one object
	struct UniformBufferObject {
		alignas(16) PeakUbo peaks[Peak::MaxNumPeaks];
		//alignas(16) Material materials[MaxMaterials];
		alignas(4) uint32_t nPeaks = 0;
		alignas(16) glm::mat4 view;
		alignas(8) glm::vec2 fov; // radians
		alignas(4) float aspect;
		alignas(16) Color skyCol;
		alignas(16) Color floorCol;
	};

	UniformBufferObject ubo = {
		{},
		0,
		{
			{1.0f,0.0f,0.0f,1.0f},
			{0.0f,1.0f,0.0f,1.0f},
			{0.0f,0.0f,1.0f,1.0f},
			{0.0f,0.0f,0.0f,1.0f}
		}, //view
		{ 3.14159f / 4.0f, 3.14159f / 4.0f}, //fov
		1.0,
		Colors::NeonBlue ,
		Colors::NeonIndigo
	};

	

	// don't bother changing shaderside ubos unless it has been changed here
	bool uboChanged = false;

#ifdef _DEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
	};

	enum { GraphicsCommandPoolInd, TransferCommandPoolInd, NumCommandPools };

	static const uint16_t NTransferBuffers = 1;

	static const uint16_t MaxFramesInFlight = 3;

	//which frame in flight this draw is
	uint8_t currentFrame = 0;
	
	//Build the pipeline here first, refactor later when it is clearer what the overlapping code is
	GLFWwindow *window;
	VkInstance instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;


	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;

	std::vector<VkImage> swapChainImages { VK_NULL_HANDLE };
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkQueue transferQueue = VK_NULL_HANDLE;

	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;

	VkCommandPool commandPools[NumCommandPools] = { VK_NULL_HANDLE };

	VkCommandBuffer commandBuffers[MaxFramesInFlight] = { VK_NULL_HANDLE };

	VkCommandBuffer transferBuffers[NTransferBuffers] = { VK_NULL_HANDLE };
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

	VkBuffer uniformBuffers[MaxFramesInFlight] = { VK_NULL_HANDLE };
	VkDeviceMemory uniformBuffersMemory[MaxFramesInFlight] = { VK_NULL_HANDLE };

	VkDescriptorPool descriptorPool = { VK_NULL_HANDLE };
	VkDescriptorSet descriptorSets[MaxFramesInFlight] = { VK_NULL_HANDLE };

	VkDescriptorPool imguiPool = {};
	bool imguiHooked = false;

	VkSemaphore imageAvailableSemaphores[MaxFramesInFlight] = { VK_NULL_HANDLE };
	VkSemaphore renderFinishedSemaphores[MaxFramesInFlight] = { VK_NULL_HANDLE };
	VkFence inFlightFences[MaxFramesInFlight] = { VK_NULL_HANDLE };

	//setup the vulkan instance
	void createInstance();

	//make sure this computer supports the required validation layers
	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	//create a validation layer and set it up to output to the console
	void setupDebugMessenger();

	//surface that instance draws to
	void createSurface(GLFWwindow *window);
		
	//  Physical device
	//pick an available gpu that supports the desired features
	std::vector<const char*> getRequiredExtensions();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	// scores how esirable a physsical device is given what it supports
	int rateDeviceSuitability(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	bool isDeviceSuitable(VkPhysicalDevice device);
	
	void pickPhysicalDevice(VkInstance instance);
	
	// conceptual device that recieves commands and bufferes and components
	void createLogicalDevice(VkInstance instance);


	// Swapchain
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);

	//vulkan writes to hidden layer of the swap chain whilst 1st layer is presenting
	void createSwapChain(int width, int height);


	//find index within the physical device with suitable memory types
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	
	//object that pipline draws textures/render to
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	//view that shader draws to
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createImageViews();

	//read compiled shader code into a shder module
	VkShaderModule createShaderModule(const char *code, size_t size);

	// define tell shaders that they will be expecting uniforms
	void createDescriptorSetLayout();
	
	// creates command for one time operation
	VkCommandBuffer beginSingleTimeCommands(uint32_t commandPoolInd = TransferCommandPoolInd);
	//executes and deletes a one time command
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, uint32_t commandPoolInd = TransferCommandPoolInd);

	//helper for creating vkbuffers
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	// performs a copy from one buffer (usually in the cpu) to another (usually in the gpu)
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	//buffer that stores vertex and vertex index data
	void createVertexBuffer();
	void createIndexBuffer();


	// buffer that holds uniform data
	void createUniformBuffers();

	// describes how each uniform buffer opject should be interperated in the shader
	void createDescriptorPool();
	void createDescriptorSets();

	// create pools for graphics, presentation, transfer commands etc
	void createCommandPools();

	//reading a binary spirv file 
	static std::vector<char> readFile(const std::string& filename);

	// initialise actual process to taking cpu resources into the gpu
	void createGraphicsPipeline();

	// thing renderpass draws to
	void createFramebuffers();

	//objects that hold persistent commands
	void createCommandBuffers();

	//objects that prevent gpu from drawing to utself twice at the same time
	void createSyncObjects();

	//what the gpu will do on a drawpass
	void createRenderPass();

	// hook the vkinstance to an instance if Imgui
	void initImgui();

	//initialize the rendering pipeline
	// void createPipeline();

	//perform a renderpass and send it to the corresponding frame buffer in swapchain
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	// recalculate the uniforms if they changed
	void updateUniformBuffer(uint32_t currentImage);

	//destroy resources associated with the current swapchain
	void cleanupSwapChain();

};

}; // namespace AudioVisualiser


#endif // !AV_VISUALISER



