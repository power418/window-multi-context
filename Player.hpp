#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include <iostream>
#include <string>

#include "GameObject.hpp"

class GameCharacter : public GameObject
{
public:
    GameCharacter(const std::string& name, float health)
        : GameObject(name), m_character_health(health) 
    {}

    virtual ~GameCharacter() { std::cout << "class GameCharacter has been destroyed!\n"; }

    void ds_info() const override
    {
        GameObject::ds_info();
        std::cout << "health: " << m_character_health << "\n";
    }

protected:
    float m_character_health = 0.0;
}; 

class PlayerCharacter : public GameCharacter
{
public: 
    PlayerCharacter(const std::string& name, float health, int experience)
        : GameCharacter(name, health), m_player_experience(experience)
    {}

    void ds_info() const override
    {
        GameCharacter::ds_info();
        std::cout << "experience: " << m_player_experience << "\n";
    }

private:
    int m_player_experience = 0;
};

#endif // __PLAYER_HPP__
