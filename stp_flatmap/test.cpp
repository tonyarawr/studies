#include "flat_map.hpp"
#include <string>
#include <algorithm>
#include "gtest/gtest.h"


TEST(FlatMapTEST, CopyConstruct)
{
    flatMap<std::string,int> test; 
    test.insert("oranges",10);
    test.insert("apples",20);
    flatMap<std::string,int> test2(test);
}

TEST(FlatMapTEST, MoveConstruct)
{
    flatMap<std::string,int> test; 
    test.insert("oranges",10);
    test.insert("apples",20);
    flatMap<std::string,int> test2(std::move(test));
}

TEST(FlatMapTEST, CopyAssign)
{
    flatMap<std::string,int> test; 
    test.insert("oranges",10);
    test.insert("apples",20);
    flatMap<std::string,int> test2;
    test2 = test; 
}

TEST(FlatMapTEST, MoveAssign)
{
    flatMap<std::string,int> test; 
    test.insert("oranges",10);
    test.insert("apples",20);
    flatMap<std::string,int> test2;
    test2 = std::move(test); 
}

TEST(FlatMapTEST, Iterators)
{
    flatMap<std::string,int> test; 
    test.insert("oranges",10);
    test.insert("apples",20);
    auto it = test.begin();
    auto end = test.end();
    
    for (; it != end; ++it) {
        auto pair = *it; 
        
        if (pair.first == "oranges") {
            pair.second = 30; 
        }
    }
}

TEST(FlatMapTEST, MethodFind)
{
     flatMap<std::string,int> test; 
     test.insert("oranges",10);
     test.insert("apples",20);
     test.insert("bananas", 30);
     auto result = std::find_if(test.begin(), test.end(),
         [](const auto& p) 
     { return p.first == "apples"; });
     EXPECT_NE(result,test.end());
}

TEST(FlatMapTEST, reverseIterators)
{
    flatMap<std::string,int> test; 
    test.insert("oranges",10);
    test.insert("apples",20);
    auto it = test.rbegin();
    auto end = test.rend();
    
    for (; it != end; ++it) {
        auto pair = *it; 
        
        if (pair.first == "oranges") {
            pair.second = 30; 
        }
    }
}

TEST(FlatMapTEST, emptyTest)
{
    flatMap<std::string,int> test; 
    EXPECT_TRUE(test.empty());
}

TEST(FlatMapTEST, operatorTest2)
{
    flatMap<std::string,int> test; 
    test.insert("oranges",10);
    test.insert("apples",20);
    test["bananas"];
    EXPECT_EQ(test.get_size(), 3);
}

TEST(FlatMapTEST, containsTest)
{
    flatMap<std::string,int> test; 
    test.insert("oranges",10);
    test.insert("apples",20);
    test["bananas"];
    EXPECT_EQ(test.contains("bananas"),true);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}