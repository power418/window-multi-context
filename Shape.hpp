#ifndef __SHAPE_HPP__
#define __SHAPE_HPP__

#include <iostream>

class Shape
{
public:
    virtual float calculate_area() const
    {
        return 0;
    }

    virtual void ds_type() const
    {
        std::cout << "shape\n";
    }
};

class Rectangle : public Shape
{
public:
    Rectangle(float width, float height)
        : m_width(width), m_height(height)
    {}

    float calculate_area() const override
    {
        return m_width * m_height;
    }

    void ds_type() const override
    {
        std::cout << "rectangle (" << m_width << " x " << m_height << ")\n";
    }

private:
    float m_width;
    float m_height;
};

class Circle : public Shape
{
public:
    Circle(float radius)
        : m_radius(radius)
    {}

    float calculate_area() const override
    {
        return 3.14159f * m_radius * m_radius;
    }

    void ds_type() const override
    {
        std::cout << "circle (radius " << m_radius << ")\n";
    }

private:
    float m_radius;
};

class Triangle : public Shape
{
public:
    Triangle(float base, float height)
        : m_base(base), m_height(height)
    {}

    float calculate_area() const override
    {
        return 0.5f * m_base * m_height;
    }

    void ds_type() const override
    {
        std::cout << "triangle (base " << m_base << ", height " << m_height << ")\n";
    }

private:
    float m_base;
    float m_height;
};

#endif // __SHAPE_HPP__
