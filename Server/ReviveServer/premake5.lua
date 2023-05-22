project "ReviveServer"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp",
		"lib/**.lib",
		"src/**.lua",
	}

	includedirs
	{
		
		"%{wks.location}/ReviveServer/src",
		"%{wks.location}/ReviveServer/lib/spdlog/include"
	}
	

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "ReviveServer_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "ReviveServer_RELEASE"
		optimize "On"