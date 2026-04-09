#pragma once

class Properties
{
public:
    Properties SetAir(bool isAir)
    {
        m_IsAir = isAir;
        return *this;
    }

    bool IsAir() const { return m_IsAir; }

private:
    bool m_IsAir = false;
};

class Block
{
public:
    Block(const Properties &properties);

    const Properties &GetProperties() const { return m_Properties; }

public:
    const Properties m_Properties;
};