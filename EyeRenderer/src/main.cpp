#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <GL/glew.h>
#include <glm/ext.hpp>

#include <meowingtwurtle/engine/input/controller.hpp>
#include <meowingtwurtle/engine/input/input_state.hpp>
#include <meowingtwurtle/engine/input/keycodes.hpp>
#include <meowingtwurtle/engine/low_level/graphics/gl_wrappers/texture_raii.hpp>
#include <meowingtwurtle/engine/low_level/graphics/render_context.hpp>
#include <meowingtwurtle/engine/low_level/graphics/shader.hpp>
#include <meowingtwurtle/engine/low_level/init.hpp>
#include <meowingtwurtle/engine/low_level/window.hpp>
#include <meowingtwurtle/engine/render_objects/graphics/default_vertex.hpp>
#include <meowingtwurtle/engine/render_objects/graphics/object.hpp>
#include <meowingtwurtle/engine/textures/graphics/color_texture.hpp>
#include <meowingtwurtle/engine/textures/graphics/texture_binder.hpp>
#include <meowingtwurtle/engine/textures/graphics/texture_fs.hpp>
#include <meowingtwurtle/engine/textures/graphics/texture_manager.hpp>
#include <meowingtwurtle/engine/utilities/graphics/camera.hpp>
#include <meowingtwurtle/units/default_units.hpp>
#include <meowingtwurtle/units/units.hpp>

static_assert(std::numeric_limits<float>::is_iec559);
static_assert(std::numeric_limits<double>::is_iec559);
static_assert(std::numeric_limits<long double>::is_iec559);

namespace e = meowingtwurtle::engine;
namespace g = e::graphics;
namespace t = g::textures;

namespace {
    namespace {
        constexpr const char* const DEFAULT_VERTEX_SHADER = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;
            layout (location = 2) in int aLayerNum;

            out vec2 texCoord;
            flat out int layerNum;
            out vec2 fragPos;

            void main()
            {
                gl_Position = vec4(aPos, 0.0, 1.0);
                texCoord = aTexCoord;
                layerNum = aLayerNum;
                fragPos = aPos;
            }
        )";
        constexpr const char* const DEFAULT_FRAGMENT_SHADER = R"(
            #version 330 core
            out vec4 FragColor;

            in vec2 texCoord;
            flat in int layerNum;
            in vec2 fragPos;

            uniform sampler2DArray textures;

            void main()
            {
                vec4 color = texture(textures, vec3(texCoord, layerNum));
                if (color.a == 0.0) discard;
                FragColor = color;
            }
        )";
    }

    struct vertex_2d {
        glm::vec2 screenPos;
        glm::vec2 uvPos;
        g::texture_array_index texLayer;
    };

    g::shader<vertex_2d> make_shader() {
        return g::shader<vertex_2d>(
            DEFAULT_VERTEX_SHADER,
            DEFAULT_FRAGMENT_SHADER,
            {
                {
                    {0},
                    g::shader_input::vec2_type,
                    g::shader_input_storage_type::floating_point,
                    {offsetof(vertex_2d, screenPos)},
                    {sizeof(vertex_2d)}
                },
                {
                    {1},
                    g::shader_input::vec2_type,
                    g::shader_input_storage_type::floating_point,
                    {offsetof(vertex_2d, uvPos)},
                    {sizeof(vertex_2d)}
                },
                {
                    {2},
                    g::shader_input::int_type,
                    g::shader_input_storage_type::signed_int,
                    {offsetof(vertex_2d, texLayer)},
                    {sizeof(vertex_2d)}
                }

            }
        );
    }
}

namespace fs = meowingtwurtle::fs;

struct graphics_engine {
public:
    graphics_engine(graphics_engine const&) = delete;
    graphics_engine& operator=(graphics_engine const&) = delete;
    
    void tick() noexcept {
        m_controller.tick();
    }
    
    void render() const noexcept {
        m_renderContext.render([&, this]{m_renderer(m_vertices); });
    }
    
    void clear() noexcept {
        m_vertices.clear();
    }
    
    auto load_texture(fs::path const& _path) {
        auto const& tex = [&]() -> t::texture const&  {
            if (not m_textures.has_texture(_path.string())) {
                m_textureNames.push_back(_path);
                return t::load_texture_file(m_textures, _path);
            }

            return m_textures.get_texture(_path.string());
        }();
        
        return t::bind_texture_array_layer(m_textureArray, g::texture_array_index{m_texturesLoaded++}, tex); 
    }
    
    void add_image(t::texture_rectangle const& _texture, int x, int y) {
        auto lowX = ((float(x - (_texture.x_dimension() * texture_width / 2)) / float(screen_width)) - 0.5);
        auto lowY = ((float(y - (_texture.y_dimension() * texture_height / 2)) / float(screen_height)) - 0.5);
        auto highX = ((float(x + (_texture.x_dimension() * texture_width / 2)) / float(screen_width)) - 0.5);
        auto highY = ((float(y + (_texture.y_dimension() * texture_height / 2)) / float(screen_height)) - 0.5);
        
        g::decompose_render_object_to<vertex_2d>(
            g::render_object_rectangle<>{
                g::location_quad{
                    {
                        glm::vec3{lowX, lowY, 0}
                    },
                    {
                        glm::vec3{lowX, highY, 0}
                    },
                    {
                        glm::vec3{highX, highY, 0}
                    },
                    {
                        glm::vec3{highX, lowY, 0}
                    }
                },
                _texture
            }.use_vertex<vertex_2d>([](g::default_vertex const& _vertex){
                return vertex_2d{_vertex.location.value, _vertex.texture.coord, _vertex.texture.layer};
            }),
                std::back_inserter(m_vertices)
        );
    }
    
    bool should_quit() const noexcept {
        return (m_controller.inputs().keyboard().key_is_down(e::input::keycode::kc_escape)) || (m_controller.quit_received());
    }
    
    graphics_engine() = default;

    static constexpr int texture_width = 512;
    static constexpr int texture_height = 512;
    static constexpr int screen_width = 800;
    static constexpr int screen_height = 800;
    
private:
    struct do_init {
        do_init() {
            e::init();
        }
    };
    
    do_init initter;
    g::window m_window{"Eye Renderer", screen_width, screen_height};
    e::controller m_controller;
    std::vector<fs::path> m_textureNames;
    t::texture_manager m_textures;
    std::vector<vertex_2d> m_vertices;
    g::render_context m_renderContext{m_window};
    g::render_context_active_lock m_renderContextLock = m_renderContext.make_active_lock();
    g::shader<vertex_2d> m_shader = make_shader();
    g::vertex_renderer<vertex_2d> m_renderer{m_shader};
    t::unique_texture_array m_textureArray = t::make_texture_array(texture_width, texture_height, 32);
    int m_texturesLoaded = 0;
};

int main() {
    graphics_engine engine;
    auto const& texture = engine.load_texture("texture/cross.png");
    
    for (int x = 0; x < 3; ++x) { 
        for (int y = 0; y < 3; ++y) {
            engine.add_image(texture, (x - 1) * engine.screen_width + 400, (y - 1) * engine.screen_height + 400);
        }
    }
    
    while (not engine.should_quit()) {
        engine.tick();
        engine.render();
    }
}
