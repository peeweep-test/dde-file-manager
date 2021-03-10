/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     liuyangming<liuyangming@uniontech.com>
 *
 * Maintainer: liuyangming<liuyangming@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "controllers/dfmtagcrumbcontroller.h"

using DFM_NAMESPACE::DFMTagCrumbController;
using DFM_NAMESPACE::CrumbData;

namespace  {
    class TestDFMTagCrumbController : public testing::Test
    {
    public:
        virtual void SetUp() override
        {
            m_pController = new DFMTagCrumbController;
        }

        virtual void TearDown() override
        {
            delete m_pController;
            m_pController = nullptr;
        }

        DFMTagCrumbController *m_pController;
    };
}

TEST_F(TestDFMTagCrumbController, supportedUrl)
{
    ASSERT_NE(m_pController, nullptr);

    DUrl url;
    url.setScheme(TAG_SCHEME);
    EXPECT_TRUE(m_pController->supportedUrl(url));
    url.setScheme(FILE_SCHEME);
    EXPECT_FALSE(m_pController->supportedUrl(url));
}

TEST_F(TestDFMTagCrumbController, seprateUrl)
{
    ASSERT_NE(m_pController, nullptr);

    DUrl url;
    url.setScheme(TAG_SCHEME);
    QList<CrumbData> list = m_pController->seprateUrl(url);
    EXPECT_TRUE(!list.empty());
}
