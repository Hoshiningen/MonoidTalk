#pragma once

#include "bakery.h"

#include <gtest/gtest.h>


class DatabaseTests : public ::testing::Test
{
protected:

    static void SetUpTestSuite()
    {

    }

    static void TearDownTestSuite()
    {

    }

protected:

    static bakery::Database m_small;
};