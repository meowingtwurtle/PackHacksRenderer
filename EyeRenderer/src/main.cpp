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

using namespace meowingtwurtle;
using namespace meowingtwurtle::engine;
using namespace meowingtwurtle::engine::graphics;
using namespace meowingtwurtle::engine::input;

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

int main() {
    try {
        using namespace std::chrono_literals;

        meowingtwurtle::engine::init();

        window window{"Twurtle Engine", 800, 800};
        auto renderContext = render_context(window);
        auto renderContextLock = renderContext.make_active_lock();
        auto engine = controller();

        textures::texture_manager textureManager;

        auto const& colorImage = textureManager.add_texture("color", textures::color_texture(16, 16, color_rgb{1, 0, 0}));

        auto const& [textureArray_, colorTexture_] = textures::make_texture_array_from_layers(colorImage);
        auto colorTextures = colorTexture_;

        auto const& colorTexture = colorTexture_;

        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

        auto shader = make_shader();
        auto vertexVecRenderer = vertex_renderer<vertex_2d>(shader);
        auto rendererLock = vertexVecRenderer.make_active_lock();
        auto vertices = std::vector<vertex_2d>{
            {
                glm::vec2{-0.5, -0.5},
                {0, 0},
                colorTexture.layer()
            },
            {
                glm::vec2{-0.5, 0.5},
                {0, 1},
                colorTexture.layer()
            },
            {
                glm::vec2{0.5, 0},
                {1, 1},
                colorTexture.layer()
            },
            
        };        

        while (true) {
            engine.tick();
            auto const& inputState = engine.inputs();
            auto const& inputChanges = engine.input_changes();

            renderContext.render([&] {
              vertexVecRenderer(vertices);
            });

            if (inputState.keyboard().key_is_down(input::keycode::kc_escape)) return 0;

            if (engine.quit_received()) return 0;
        }
    } catch (std::exception& e) {
        log::error << "Exception reached main! Message: " << e.what();
        return 1;
    } catch (...) {
        log::error("Exception reached main!");
        return 1;
    }
}
