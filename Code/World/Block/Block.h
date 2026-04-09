#pragma once

#include "World/Block/BlockProperties.h"

class Block
{
public:
    Block(const BlockProperties &properties);

    const BlockProperties &GetProperties() const { return m_Properties; }

public:
    const BlockProperties m_Properties;
};