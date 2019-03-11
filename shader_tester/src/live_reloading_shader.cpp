#include "live_reloading_shader.h"

#include "SFML/Graphics.hpp"

#include <iostream>
#include <thread>
#include "shader_tester.h"
#include <chrono>

#include "shader_header.h"

using namespace std::chrono_literals;

static auto startT = std::chrono::steady_clock::now();

LiveReloadingShader::LiveReloadingShader(const fs::path& _shaderPath)
    : shaderPath(_shaderPath)
    , fw(_shaderPath.parent_path().generic_string())
    , window(std::make_unique<sf::RenderWindow>(sf::VideoMode{ 512, 512 }, _shaderPath.generic_string(), sf::Style::Titlebar))
    , mousePos(.5, .5)
    , mouseEnabled(false)
    , windowClosed(false)
{
    window->setVerticalSyncEnabled(true);
    window->setFramerateLimit(0);
    texture.create(window->getSize().x, window->getSize().y);

    bool success = false;
    do {
        success = shader.loadFromMemory(shaderHeader + LoadFile(shaderPath.generic_string()), sf::Shader::Type::Fragment);
        if (!success) {
            std::cerr << "FAILED TO LOAD SHADER " << std::endl;
            std::this_thread::sleep_for(100ms);
            ClearCmd();
        }
    } while (!success);
    ClearCmd();
    std::cout << "LOADED: " << shaderPath.generic_string() << std::endl;
}

LiveReloadingShader::~LiveReloadingShader() {
    window->close();
}

void LiveReloadingShader::Tick() {
    window->clear();
    texture.clear();
    const auto& size = window->getSize();
    shader.setUniform("iTime", (std::chrono::steady_clock::now() - startT).count() * 1.f / std::chrono::steady_clock::period::den);
    shader.setUniform("iResolution", sf::Glsl::Vec2(size));
    shader.setUniform("iMouse", sf::Glsl::Vec2(mousePos.x * size.x, mousePos.y * size.y));

    sf::Sprite currentSprite(texture.getTexture());
    sf::RenderStates renderStates(&shader);
    renderStates.blendMode = { sf::BlendMode::One, sf::BlendMode::One };
    texture.draw(currentSprite, renderStates);
    texture.display();
    window->draw(currentSprite);

    window->display();

    sf::Event e;
    while (window->pollEvent(e)) {
        if (e.type == sf::Event::Closed) {
            windowClosed = true;
        }
        if (e.type == sf::Event::KeyReleased) {
            if (e.key.code == sf::Keyboard::Key::Escape) {
                windowClosed = true;
            }
        }
        if (e.type == sf::Event::Resized) {
            window->setView({ {e.size.width / 2.f, e.size.height / 2.f}, {1.f*e.size.width, 1.f*e.size.height} });
            texture.create(e.size.width, e.size.height);
        }
        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Button::Left)
            mouseEnabled = true;
        if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Button::Left)
            mouseEnabled = false;
        if (e.type == sf::Event::MouseMoved) {
            if (mouseEnabled)
                mousePos = { e.mouseMove.x * 1.f / window->getSize().x, e.mouseMove.y *  1.f / window->getSize().y };
        }
    }
    if (fw.CheckChanged()) {
        bool success = false;
        do {
            success = shader.loadFromMemory(shaderHeader + LoadFile(shaderPath.generic_string()), sf::Shader::Type::Fragment);
            if (!success) {
                std::cerr << "FAILED TO LOAD SHADER " << std::endl;
                std::this_thread::sleep_for(100ms);
                ClearCmd();
            }
        } while (!success);
        ClearCmd();
        std::cout << "LOADED" << std::endl;
    }
}