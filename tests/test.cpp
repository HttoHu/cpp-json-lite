#include "../src/json_parser.hpp"
#include <fstream>
int tot_assert = 0;
int failed_assert_cnt = 0;
template <typename T, typename U>
void CHECK_EQ(T a, U b)
{
  tot_assert++;
  if (a != b)
  {
    std::cerr << "\tCHECK failed, \"" << a << "\" not equal to \"" << b << "\"" << std::endl;
    failed_assert_cnt++;
    return;
  }
}
template <typename T, typename U>
void CHECK_NE(T a, U b)
{
  tot_assert++;
  if (a == b)
  {
    std::cerr << "\tCHECK failed, \"" << a << "\" not equal to \"" << b << "\"" << std::endl;
    failed_assert_cnt++;
    return;
  }
}

void test_unicode()
{
  std::cout << "Running test: lexer test: test_unicode\n";
  JSON json(R"(
    "\u0013\u4f60\u597D"
  )");
  CHECK_EQ(json.get_str(), "\u0013\u4f60\u597D");
  CHECK_EQ(JSON("\"\\u5730goodboy\"").get_str(), "\u5730goodboy");
  CHECK_EQ(JSON(R"("hello \u5730 world\\")").get_str(), "hello \u5730 world\\");
}

int main()
{
  test_unicode();
  std::cout << "==============================================\n";
  std::cout << "total assert: " << tot_assert << "\n";
  std::cout << "success assert: " << tot_assert - failed_assert_cnt << "\n";
  std::cout << "failed assert: " << failed_assert_cnt << "\n";
  std::cout << "test result: " << (failed_assert_cnt ? "FAILED" : "SUCCESS") << std::endl;
  return 0;
}