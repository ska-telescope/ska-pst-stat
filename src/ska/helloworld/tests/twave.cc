//! Boilerplate tests from the GoogleTests package

#include "ska/helloworld/wave.h" // The class I am testing
#include "gtest/gtest.h" // the googletest framework

namespace {

// The fixture for testing class Wave .
class WaveTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  WaveTest() {
     // You can do set-up work for each test here.
  }

  ~WaveTest() override {
     // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
     // Code here will be called immediately after the constructor (right
     // before each test).
  }

  void TearDown() override {
     // Code here will be called immediately after each test (right
     // before the destructor).
  }

  // Objects declared here can be used by all tests in the test suite for Foo.
};

// Tests that the Wave::greeting() method does what we expect.
TEST_F(WaveTest, MethodWave) {
  ska::helloworld::wave f;
  EXPECT_EQ(f.greeting(),"I am waving hello");
}

// Tests that Foo does Xyz.
// TEST_F(WaveTest, DoesXyz) {
  // Exercises the Xyz feature of Foo.
// }

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
