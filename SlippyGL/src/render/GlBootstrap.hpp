#pragma once
#include <cstdint>

struct GLFWwindow;

namespace slippygl::render
{
	// 윈도우 설정 구조체
	struct WindowConfig 
	{ 
		int width = 800;
		int height = 600; 
		const char* title = "SlippyGL"; 
	};

	/**
	 * OpenGL 컨텍스트 및 윈도우 생성/관리
	 * GLFW+GLAD 기반
	 */
	class GlBootstrap 
	{
	public:
		GlBootstrap() = default;
		~GlBootstrap();

		// 복사/이동 금지 (RAII)
		GlBootstrap(const GlBootstrap&) = delete;
		GlBootstrap& operator=(const GlBootstrap&) = delete;

		bool init(const WindowConfig& cfg);     // GLFW+GLAD init
		void shutdown();                        // 리소스 정리
		bool shouldClose() const;               // 창 닫기 요청
		void poll();                            // 입력/이벤트
		void beginFrame(float r = 0.1f, float g = 0.1f, float b = 0.1f);  // glClear
		void endFrame();                        // glfwSwapBuffers
		
		// 뷰포트 크기 조회
		int width() const noexcept { return width_; }
		int height() const noexcept { return height_; }

		GLFWwindow* window() const noexcept { return window_; }

	private:
		GLFWwindow* window_ = nullptr;
		int width_ = 0;
		int height_ = 0;
	};
}
