#ifndef _SIMPLEGL_H_
#define _SIMPLEGL_H_
// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <concepts>
#include <type_traits>
#include <string>
#include <ostream>
#include <vector>
#include <map>
#include <utility>
#include <typeinfo>
#include <typeindex>
#include <span>

namespace minigl
{
    class render_pipeline;

    enum class colors
    {
        red,
        white,
        yellow,
        purple,
        light_blue,
        orange,
        green,
        blue,
        cyan,
        black,
        violet,
        turqoise,
        lilac,
        dark_grey,
        dark_purple,
        bronze,
        dark_blue,
        forest_green,
        brown,
    };

    class color : public glm::vec4
    {
    public:
        constexpr color(float r, float g, float b, float a = 1) : glm::vec4{r, g, b, a} {};
        color(colors c);
        color(const color&) = default;
        float &red(void) { return glm::vec4::operator[](0); }
        float &green(void) { return glm::vec4::operator[](1); }
        float &blue(void) { return glm::vec4::operator[](2); }
        float &alpha(void) { return glm::vec4::operator[](3); }
        const float &red(void) const { return glm::vec4::operator[](0); }
        const float &green(void) const { return glm::vec4::operator[](1); }
        const float &blue(void) const { return glm::vec4::operator[](2); }
        const float &alpha(void) const { return glm::vec4::operator[](3); }
    };
    std::ostream &operator<<(std::ostream &os, const color &c);

    class pixels
    {
    public:
        int px;
        constexpr pixels(int x) : px{x} {};
    };
    constexpr pixels operator"" _px(unsigned long long arg)
    {
        return pixels(arg);
    }

    class texture
    {
    private:
        std::vector<color> tex_data;
        int w;
        int h;
    public:
        texture(const pixels &width, const pixels &height, const color &background);
        int width(void) const {return w;}
        int height(void) const {return h;}
        std::vector<color>::iterator data(void) {return tex_data.begin();} 
        std::span<color> operator[](int i);
        friend render_pipeline;
    };

    class window
    {
    private:
        GLFWwindow *window_ptr;
        bool window_ok = false;
        int window_width;
        int window_height;

    public:
        bool ok(void);
        int width(void) const {return window_width;}
        int height(void) const {return window_height;}
        window(pixels width, pixels height, std::string name);
        ~window();
        window &operator=(const window &) = delete;
        window(const window &) = delete;
        void set_background_color(color c);
        template <typename C, typename... R>
        requires std::invocable<C, R...>
        void render(C oper, R &&...args)
        {
            do
            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, window_width, window_height);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                oper(std::forward<R>(args)...);
                // Swap buffers
                glfwSwapBuffers(window_ptr);
                glfwPollEvents();
            } while (glfwGetKey(window_ptr, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
                     glfwWindowShouldClose(window_ptr) == 0);
        }
        void render();
    };

    void pipeline_connect(std::string r_name, render_pipeline &receiver,
        std::string s_name, render_pipeline &transmitter,
        pixels width = 0_px, pixels height = 0_px);

    template <typename T>
    concept valid_shader_io =
        std::same_as<T, typename glm::vec2> ||
        std::same_as<T, typename glm::vec3> ||
        std::same_as<T, typename glm::vec4> ||
        std::same_as<T, typename glm::mat3> ||
        std::same_as<T, typename glm::mat4> ||
        std::same_as<T, int> ||
        std::same_as<T, color> ||
        std::same_as<T, float>;
    template <typename T>
    concept valid_uniform =
        std::same_as<T, texture> ||
        std::same_as<T, typename glm::vec2> ||
        std::same_as<T, typename glm::vec3> ||
        std::same_as<T, typename glm::vec4> ||
        std::same_as<T, typename glm::mat3> ||
        std::same_as<T, typename glm::mat4> ||
        std::same_as<T, int> ||
        std::same_as<T, color> ||
        std::same_as<T, float>;
    template <typename T>
    concept valid_vertex_attribute =
        std::same_as<T, float> ||
        std::same_as<T, typename glm::vec2> ||
        std::same_as<T, typename glm::vec3> ||
        std::same_as<T, typename glm::vec4> ||
        std::same_as<T, color>;
    enum class shader_types
    {
        vertex,
        fragment,
    };
    class shader
    {
    private:
        std::map<std::string, std::type_index> input_map;
        std::map<std::string, std::type_index> output_map;
        std::map<std::string, std::type_index> uniform_map;
        std::map<std::string, std::type_index> attribute_map;
        std::string shader_body;
        shader_types type;

    public:
        shader(shader_types t) : type{t}, shader_body{""} {}
        template <typename T>
        requires valid_shader_io<T>
        void add_input(std::string name)
        {
            input_map.insert({name, std::type_index(typeid(T))});
        }
        template <typename T>
        requires valid_shader_io<T>
        void add_output(std::string name)
        {
            output_map.insert({name, std::type_index(typeid(T))});
        }
        template <typename T>
        requires valid_uniform<T>
        void add_uniform(std::string name)
        {
            uniform_map.insert({name, std::type_index(typeid(T))});
        }
        template <typename T>
        requires valid_vertex_attribute<T>
        void add_attribute(std::string name)
        {
            attribute_map.insert({name, std::type_index(typeid(T))});
        }
        void define_shader(std::string body);
        friend render_pipeline;
    };

    class render_pipeline
    {
    private:
        struct attribute_node
        {
            std::type_index type_id;
            GLuint array_num;
            GLuint buffer_id;
            GLsizei shape_size;
        };
        struct uniform_node
        {
            std::type_index type_id;
            GLuint uniform_id;
            GLuint texture_num;
            GLuint texture_id;
        };
        struct output_node
        {
            std::type_index type_id;
            int tex_width;
            int tex_height;
            GLuint tex_layout_pos;
            GLuint framebuf_id;
            GLuint depthbuf_id;
        };
        int window_width;
        int window_height;
        bool is_ok = false;
        bool connected = false;
        GLsizei min_verticies;
        GLuint vao_id;
        GLuint shader_program_id;
        std::vector<output_node> active_outputs;
        std::vector<GLuint> texture_ids;
        std::vector<GLuint> framebuffer_ids;
        std::vector<GLuint> depthbuffer_ids;
        std::map<std::string, output_node> output_map;
        std::map<std::string, uniform_node> uniform_map;
        std::map<std::string, attribute_node> attribute_map;
        std::string generate_shader_includes(const shader &s);
        void generate_uniforms(const shader &s);
        void generate_attributes(const shader &s);
        void generate_io(const shader &s);

    public:
        render_pipeline() = delete;
        render_pipeline(const window& target_window, const shader &vert_shader, const shader &frag_shader);
        render_pipeline(const render_pipeline&) = delete;
        render_pipeline(render_pipeline&& old);
        ~render_pipeline();
        bool ok(void);
        template <typename T>
            requires valid_uniform<T>
        void update_uniform(std::string name, const T &new_value)
        {
            if (!is_ok) return;
            auto node = uniform_map.at(name);
            if (node.type_id != std::type_index(typeid(T))) {
                throw std::runtime_error("uniform type does not match");
            }
            glUseProgram(shader_program_id);
            if constexpr (std::same_as<T, texture>) {
                glBindTexture(GL_TEXTURE_2D, node.texture_id);
                glTexImage2D(GL_TEXTURE_2D,
                    0, GL_RGBA, new_value.width(), new_value.height(),
                    0, GL_RGBA, GL_FLOAT, new_value.tex_data.data());
                glUniform1i(node.uniform_id, node.texture_num);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            } else if constexpr (std::same_as<T, typename glm::mat4>) {
                glUniformMatrix4fv(node.uniform_id, 1, GL_FALSE, &new_value[0][0]);
            } else if constexpr (std::same_as<T, typename glm::mat3>) {
                glUniformMatrix3fv(node.uniform_id, 1, GL_FALSE, &new_value[0][0]);
            } else if constexpr (std::same_as<T, typename glm::vec2>) {
                glUniform2f(node.uniform_id, new_value[0], new_value[1]);
            } else if constexpr (std::same_as<T, typename glm::vec3>) {
                glUniform3f(node.uniform_id, new_value[0], new_value[1], new_value[2]);
            } else if constexpr (std::same_as<T, typename glm::vec4>) {
                glUniform4f(node.uniform_id, new_value[0], new_value[1], new_value[2], new_value[3]);
            } else if constexpr (std::same_as<T, int>) {
                glUniform1i(node.uniform_id, new_value);
            } else if constexpr (std::same_as<T, float>) {
                glUniform1f(node.uniform_id, new_value);
            } else if constexpr (std::same_as<T, color>) {
                glUniform4f(node.uniform_id, new_value[0], new_value[1], new_value[2], new_value[3]);
            }
        }
        template <typename T>
            requires valid_vertex_attribute<T>
        void update_vertex_attr(std::string attr_name, const std::vector<T> &new_value)
        {
            if (!is_ok) return;
            auto node = attribute_map.at(attr_name);
            if (node.type_id != std::type_index(typeid(T))) {
                throw std::runtime_error("attribute type does not match");
            }
            int vertex_size;
            if constexpr (std::same_as<T, float>) {
                vertex_size = 1;
            } else {
                vertex_size = T::length();
            }
            if (new_value.size() == 0) {
                return;
            }
            if (new_value.size() < min_verticies || min_verticies == 0) {
                min_verticies = new_value.size();
            }
            glBindVertexArray(vao_id);
            glUseProgram(shader_program_id);
            glEnableVertexAttribArray(node.array_num);
            glBindBuffer(GL_ARRAY_BUFFER, node.buffer_id);
            glBufferData(GL_ARRAY_BUFFER, min_verticies * sizeof(T), new_value.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(
                node.array_num,
                vertex_size,
                GL_FLOAT,
                GL_FALSE,
                0,
                (void *)0);
        }
        void render(void);
        void render_textures(void);
        friend void pipeline_connect(std::string r_name, render_pipeline &receiver,
        std::string s_name, render_pipeline &transmitter,
            pixels width, pixels height);
    };
}

#endif // _SIMPLEGL_H_