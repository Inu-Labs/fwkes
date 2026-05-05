#pragma once

#include "window.hpp"

#include <SDL3/SDL.h>

namespace ui {
    class Canvas : public Window {
      public:
        Canvas(SDL_Texture *texture = nullptr);

        void set_texture(SDL_Texture *texture) { m_texture = texture; }

      private:
        void main() override;
        void pre_main() override;
        void post_main() override;

        SDL_Texture *m_texture;
    };
}
