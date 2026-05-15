#include "GameObject.h"
#include "../textureManager/TextureManager.h"

namespace openfranko::src::gameObject {

GameObject::GameObject(const char *textureSheet, SDL_Renderer *renderer, int x,
                       int y) {
  m_renderer = renderer;
  m_objTexture =
      textureManager::TextureManager::LoadTexture(textureSheet, renderer);

  m_xpos = x;
  m_ypos = y;
}

void GameObject::update() {

  m_xpos++;
  m_ypos++;

  m_srcRect.h = 79;
  m_srcRect.w = 32;
  m_srcRect.x = 0;
  m_srcRect.y = 0;

  m_destRect.x = m_xpos;
  m_destRect.y = m_ypos;
  m_destRect.h = m_srcRect.h * 2;
  m_destRect.w = m_srcRect.w * 2;
}

void GameObject::render() {
  SDL_RenderCopy(m_renderer, m_objTexture, &m_srcRect, &m_destRect);
}

} // namespace openfranko::src::gameObject