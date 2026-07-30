#ifndef __WEAPON_HPP__
#define __WEAPON_HPP__

#include <iostream>
#include <string>

class Weapon
{
public:
    Weapon(const std::string& name, float damage) 
        : m_weapon_name(name), m_weapon_damage(damage)
    {}
    
    virtual ~Weapon()
    {
        std::cout << "class of Weapon has been destroyed!\n";
    }
    
protected:
    std::string m_weapon_name;
    float m_weapon_damage;
};

class Sword : public Weapon
{
public:
    Sword(const std::string& name, float damage)
        : Weapon(name, damage)
    {}

    void ds_sword_info() const
    {
        std::cout << "sword name: " << m_weapon_name << ", damage: " << m_weapon_damage << "\n";
    }
};

class MagicStaff : public Weapon
{
public:
    MagicStaff(const std::string& name, float damage)
        : Weapon(name, damage)
    {}

    void ds_staff_info() const
    {
        std::cout << "staff name: " << m_weapon_name << ", damage: " << m_weapon_damage << "\n";
    }
};

class Wizard : public Sword, public MagicStaff
{
public:
    Wizard(const std::string& name, float sword_damage, float staff_damage)
        : Sword("Excalibur", sword_damage), MagicStaff("Fire Staff", staff_damage), m_character_name(name)
    {}

    void ds_info() const 
    {
        std::cout << "wizard name: " << m_character_name << std::endl;
        ds_sword_info();
        ds_staff_info();
    }
private:
    std::string m_character_name;
};

#endif // __WEAPON_HPP__
