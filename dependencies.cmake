include(FetchContent)

FetchContent_Declare (
		hecs
		GIT_REPOSITORY https://github.com/Nignik/HECS.git
		#GIT_TAG v1.0.2
)

FetchContent_Declare(
		glm
		GIT_REPOSITORY https://github.com/g-truc/glm.git
		GIT_TAG 1.0.1
		FIND_PACKAGE_ARGS NAMES glm
)
