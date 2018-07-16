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
#include "Particles/Grid.hpp"

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
			v.y = pos_x(gen);
		}

		std::bernoulli_distribution negative;
		std::uniform_real_distribution<float> vel_x(0.0f, 1.f);
		std::uniform_real_distribution<float> vel_y(0.0f, 1.f);

		for (auto &v : particles->velocities) {
			v.x = vel_x(gen) * (negative(gen) ? 1.f : -1.f);
			v.y = vel_x(gen) * (negative(gen) ? 1.f : -1.f);
		}
	}


	auto grid = std::make_unique<Grid<128>>();

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

	std::array<float, 30> times;
	std::array<float, 30> sim_times;
	std::fill(times.begin(), times.end(), 16.f);
	std::fill(sim_times.begin(), sim_times.end(), 16.f);
	std::size_t time_i = 0;
	std::size_t sim_time_i = 0;
	constexpr float step = 16.6e-4f;
	glm::vec2 gravity{0.f, -9.89e-1f};
	bool sim = false;
	while ( ! glfwWindowShouldClose(window)) {
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
		glfwPollEvents();

		int state = glfwGetKey(window, GLFW_KEY_SPACE);
		if (state == GLFW_PRESS)
			sim = true;

		if (sim) {
			for (std::size_t i = 0; i < particles->size(); ++i) {
				particles->positions[i] += step * particles->velocities[i];
				particles->velocities[i] += step * gravity;

				if (particles->positions[i].x < 0.f || particles->positions[i].x > 1.f)
					particles->velocities[i].x = -particles->velocities[i].x;

				if (particles->positions[i].y < 0.f || particles->positions[i].y > 1.f)
					particles->velocities[i].y = -particles->velocities[i].y;
			}

			for (std::size_t i = 0; i < particles->size(); ++i) {
				grid->add_element(i, particles->positions[i]);
			}

			uint64_t colls = 0;
			//handle collisions..
			for (int16_t cell_y = 0; cell_y < grid->get_num_cells()-1; ++cell_y) {
				for (int16_t cell_x = 0; cell_x < grid->get_num_cells()-1; ++cell_x) {
					const CellIndex cell_index{cell_x, cell_y};
					const auto cell_int_index = grid->get_cell(cell_index);
					const auto &cell = grid->cells[cell_int_index];

					for (std::size_t j = 0; j < grid->cell_sizes[cell_int_index]; ++j) {
						const auto &a = cell.members[j];

						for (int16_t adj_cell_y = cell_y; adj_cell_y <= cell_y; ++adj_cell_y) {
							for (int16_t adj_cell_x = cell_x; adj_cell_x <= cell_x; ++adj_cell_x) {
								CellIndex adj_index = {adj_cell_x, adj_cell_y};
								uint16_t adj_int_index = grid->get_cell(adj_index);
								const auto &adj_cell = grid->cells[adj_int_index];

								std::size_t start_index = (cell_int_index == adj_int_index) ? (j+1) : 0;
								for (std::size_t k = start_index; k < grid->cell_sizes[adj_int_index]; ++k) {
									const auto &b = adj_cell.members[k];
									const auto diff = glm::abs(particles->positions[a] - particles->positions[b]);
									if (diff.x < 0.0001f && diff.y < 0.0001f) {
										// collision!
										particles->velocities[a] = particles->velocities[b] * 0.99f;
										particles->velocities[b] = particles->velocities[a] * 0.99f;
										++colls;
									}
								}
							}
						}
					}
				}
			}

			grid->reset();
		}

		const auto sim_end = std::chrono::high_resolution_clock::now();
		sim_times[sim_time_i] = std::chrono::duration_cast<std::chrono::nanoseconds>(sim_end - start).count() / 1e6f;
		sim_time_i = (sim_time_i + 1) % sim_times.size();
		float sim_avg = std::accumulate(sim_times.begin(), sim_times.end(), 0.f) / static_cast<float>(sim_times.size());

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
		title << "parsim :: frame "
		      << std::setprecision(4) << std::setw(6) << std::setfill('0')
		      << avg << "\tms :: sim "
		      << std::setprecision(4) << std::setw(6) << std::setfill('0')
		      << sim_avg << "\tms";
		glfwSetWindowTitle(window, title.str().c_str());
	}

	glfwTerminate();
	return 0;
}
