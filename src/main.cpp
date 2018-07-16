#include <random>
#include <algorithm>
#include <memory>
#include <numeric>
#include <array>
#include <sstream>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <streambuf>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include "Particles/ParticleArray.hpp"

constexpr uint16_t win_size[] = {800, 600};

#define PARSIM_DEBUG_GRAPHICS

#ifdef PARSIM_DEBUG_GRAPHICS
void GLAPIENTRY
MessageCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}
#endif

int main() {
	if ( ! glfwInit()) {
		std::cout << "Couldn't init glfw!" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#ifdef PARSIM_DEBUG_GRAPHICS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_DEBUG_PROFILE);
#endif
	GLFWwindow* window = glfwCreateWindow(win_size[0], win_size[1], "parsim", nullptr, nullptr);

	if ( ! window) {
		std::cout << "Couldn't create window!" << std::endl;
		glfwTerminate();
		return -2;
	}

	glfwMakeContextCurrent(window);

	if ( ! gladLoadGL()) {
		std::cout << "Couldn't load GL!" << std::endl;
		glfwTerminate();
		return -3;
	}

#ifdef PARSIM_DEBUG_GRAPHICS
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
#endif

	auto particles = std::make_unique<ParticleArray<(win_size[0] * win_size[1]) / 3>>();

	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> pos_x(0.f, 1.f);
		std::uniform_real_distribution<float> pos_y(0.f, 1.f);

		for (auto &v : particles->positions) {
			v.x = pos_x(gen);
			v.y = pos_y(gen);
		}

		std::uniform_real_distribution<float> vel_x(-1.f, 1.f);
		std::uniform_real_distribution<float> vel_y(-1.f, 1.f);

		for (auto &v : particles->velocities) {
			v.x = vel_x(gen);
			v.y = vel_y(gen);
		}
	}
	
	glfwSwapInterval(1);

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	{
		std::ifstream f("res/shaders/particles/point.vert");
		std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		const char *c_str = str.c_str();
		glShaderSource(vertex_shader, 1, &c_str, nullptr);
		glCompileShader(vertex_shader);
	}

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	{
		std::ifstream f("res/shaders/particles/point.frag");
		std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		const char *c_str = str.c_str();
		glShaderSource(fragment_shader, 1, &c_str, nullptr);
		glCompileShader(fragment_shader);
	}

	GLuint shader_program = glCreateProgram();
	{
		glAttachShader(shader_program, vertex_shader);
		glAttachShader(shader_program, fragment_shader);
		glLinkProgram(shader_program);
	}

	glUseProgram(shader_program);

	GLuint buff;
	glCreateBuffers(1, &buff);
	glBindBuffer(GL_ARRAY_BUFFER, buff);
	glBufferData(GL_ARRAY_BUFFER, particles->size() * sizeof(decltype(particles->positions)::value_type), particles->positions.data(), GL_STATIC_DRAW);

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexArrayAttrib(vao, 0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	//glPointSize(3.f);
	std::array<float, 30> times;
	std::fill(times.begin(), times.end(), 16.f);
	std::size_t time_i = 0;
	constexpr float step = 16.6e-3f;
	bool sim = false;
	while ( ! glfwWindowShouldClose(window)) {
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
		glfwPollEvents();

		int state = glfwGetKey(window, GLFW_KEY_SPACE);
		if (state == GLFW_PRESS)
			sim = true;

		if (sim) {
			for (uint64_t i = 0; i < particles->size(); ++i) {
				particles->positions[i] += step * particles->velocities[i];
			}
		}


		glm::ivec2 win;
		glfwGetWindowSize(window, &win.x, &win.y);
		glViewport(0, 0, win.x, win.y);
		

		glClear(GL_COLOR_BUFFER_BIT);
		glBufferData(GL_ARRAY_BUFFER, particles->size() * sizeof(decltype(particles->positions)::value_type), particles->positions.data(), GL_STATIC_DRAW);
		glDrawArrays(GL_POINTS, 0, particles->size());
		glfwSwapBuffers(window);

		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		times[time_i] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1e6f;
		time_i = (time_i + 1) % times.size();
		float avg = std::accumulate(times.begin(), times.end(), 0.f) / static_cast<float>(times.size());
		std::stringstream title;
		title << "parsim :: "
		      << std::setprecision(3) << std::setw(7)
		      << avg << " ms";
		glfwSetWindowTitle(window, title.str().c_str());
	}

	glfwTerminate();
	return 0;
}
