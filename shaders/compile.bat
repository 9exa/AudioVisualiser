

::for %%f in (*) do (
 ::   if "%%~xf"==".vert" (
  ::     C:/VulkanSDK/1.3.216.0/Bin/glslc.exe %%f -o %%~nfVert.spv
 ::   );
  ::  if "%%~xf"==".frag" (
  ::      C:/VulkanSDK/1.3.216.0/Bin/glslc.exe %%f -o %%~nfFrag.spv
  ::  );

::)
C:/VulkanSDK/1.3.216.0/Bin/glslc.exe visualiser.frag -o visualiserFrag.spv
C:/VulkanSDK/1.3.216.0/Bin/glslc.exe visualiser.frag -o visualiserFrag.spv
:: C:/VulkanSDK/1.3.216.0/Bin/glslangvalidator --target-env vulkan1.2 -e main visualiser.frag -o visualiserFrag.spv
:: C:/VulkanSDK/1.3.216.0/Bin/glslangvalidator --target-env vulkan1.2 -e main visualiser.vert -o visualiserVert.spv

python ../tools/embedFiles.py -o ../gen/embedded.h visualiserVert.spv VisualiserVertShader visualiserFrag.spv VisualiserFragShader