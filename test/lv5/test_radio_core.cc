#include <gtest/gtest.h>
#include <iv/scoped_ptr.h>
#include <iv/lv5/radio/core.h>
#include <iv/lv5/radio/scope.h>
#include <iv/lv5/jsstring.h>

TEST(RadioCoreCase, MainTest) {
  iv::core::ScopedPtr<iv::lv5::radio::Core> core(new iv::lv5::radio::Core);
  for (std::size_t i = 0; i < 100; ++i) {
    iv::lv5::radio::Cell* cell = core->Allocate<iv::lv5::JSString>();
    EXPECT_TRUE(cell);
  }
}

TEST(RadioCoreCase, BlockAddrTest) {
  iv::core::ScopedPtr<iv::lv5::radio::Core> core(new iv::lv5::radio::Core);
  iv::lv5::radio::Cell* cell1 = core->Allocate<iv::lv5::JSString>();
  iv::lv5::radio::Cell* cell2 = core->Allocate<iv::lv5::JSString>();
  EXPECT_EQ(cell1->block(), cell2->block());
}

TEST(RadioCoreCase, ColoringWhiteTest) {
  iv::core::ScopedPtr<iv::lv5::radio::Core> core(new iv::lv5::radio::Core);
  iv::lv5::radio::Cell* cell = core->Allocate<iv::lv5::JSString>();
  EXPECT_EQ(iv::lv5::radio::Color::CLEAR, cell->color());
  cell->Coloring(iv::lv5::radio::Color::WHITE);
  core->Mark(nullptr);
  EXPECT_EQ(iv::lv5::radio::Color::WHITE, cell->color());
  core->Collect(nullptr);
  EXPECT_EQ(iv::lv5::radio::Color::CLEAR, cell->color());
}

TEST(RadioCoreCase, ColoringBlackTest) {
  iv::core::ScopedPtr<iv::lv5::radio::Core> core(new iv::lv5::radio::Core);
  iv::lv5::radio::Cell* cell = nullptr;
  {
    const iv::lv5::radio::Scope scope(core.get());
    cell = core->Allocate<iv::lv5::JSString>();
    EXPECT_EQ(iv::lv5::radio::Color::CLEAR, cell->color());
    // construct
    cell->Coloring(iv::lv5::radio::Color::WHITE);
    core->ChainToScope(cell);
    EXPECT_EQ(iv::lv5::radio::Color::WHITE, cell->color());
    core->Mark(nullptr);
    EXPECT_EQ(iv::lv5::radio::Color::BLACK, cell->color());
  }
  core->Collect(nullptr);
  EXPECT_EQ(iv::lv5::radio::Color::WHITE, cell->color());
}

TEST(RadioCoreCase, ColoringScopedTest) {
  iv::core::ScopedPtr<iv::lv5::radio::Core> core(new iv::lv5::radio::Core);
  iv::lv5::radio::Cell* cell = nullptr;
  {
    const iv::lv5::radio::Scope scope(core.get());
    cell = core->Allocate<iv::lv5::JSString>();
    EXPECT_EQ(iv::lv5::radio::Color::CLEAR, cell->color());
    // construct
    cell->Coloring(iv::lv5::radio::Color::WHITE);
    core->ChainToScope(cell);
    EXPECT_EQ(iv::lv5::radio::Color::WHITE, cell->color());
  }
  core->Mark(nullptr);
  EXPECT_EQ(iv::lv5::radio::Color::WHITE, cell->color());
  core->Collect(nullptr);
  EXPECT_EQ(iv::lv5::radio::Color::CLEAR, cell->color());
}

TEST(RadioCoreCase, ColoringScopedEscapeTest) {
  iv::core::ScopedPtr<iv::lv5::radio::Core> core(new iv::lv5::radio::Core);
  iv::lv5::radio::Cell* cell = nullptr;
  {
    iv::lv5::radio::Scope scope(core.get());
    cell = core->Allocate<iv::lv5::JSString>();
    EXPECT_EQ(iv::lv5::radio::Color::CLEAR, cell->color());
    // construct
    cell->Coloring(iv::lv5::radio::Color::WHITE);
    core->ChainToScope(cell);
    EXPECT_EQ(iv::lv5::radio::Color::WHITE, cell->color());
    cell = scope.Close(cell);
  }
  core->Mark(nullptr);
  EXPECT_EQ(iv::lv5::radio::Color::BLACK, cell->color());
  core->Collect(nullptr);
  EXPECT_EQ(iv::lv5::radio::Color::WHITE, cell->color());
}
