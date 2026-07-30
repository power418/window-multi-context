#ifndef __GAME_OBJECT_HPP__
#define __GAME_OBJECT_HPP__

#include <iostream>
#include <string>

class GameObject
{
public:
    GameObject(const std::string& name)
        : m_object_name(name)
    {}

    virtual ~GameObject()
    {
        std::cout << "game object is destroyed!\n";
    }

    virtual void ds_info() const 
    {
        std::cout << "name: " << m_object_name << "\n";
    }

protected:
    std::string m_object_name;
};

class Enemy : public GameObject
{
public:
    Enemy(const std::string& name, float health, float damage)
        : GameObject(name), m_enemy_health(health), m_enemy_damage(damage)
    {}

    void ds_info() const override
    {
        GameObject::ds_info();
        std::cout << "enemy health: " << m_enemy_health 
                  << ", enemy damage: " << m_enemy_damage << "\n";
    }

private:
    float m_enemy_health;
    float m_enemy_damage;
};

#endif // __GAME_OBJECT_HPP__
