#include <iostream>
#include "message_bus.h"
using namespace purecpp;

void foo3(int a,  std::string& b, std::shared_ptr<int> c) {
  std::cout <<b<< " foo3\n";
  b = "test";
  *c = 30;
}

struct person{
  int id;
  std::string name;
};

void foo_person(const person& p){
  std::cout<<p.id<<", "<<p.name<<'\n';
}

std::string foo_return(int a){
  return std::to_string(a+2);
}

void test_msg_bus(){
  message_bus& bus = message_bus::get();
  std::string key = "test_three_args";
  bus.subscribe(key, &foo3);
  auto c = std::make_shared<int>(2);

  std::string mod_str = "b";
  bus.publish(key, 1, mod_str, c);
  std::cout <<mod_str<<"\n";

  std::string person_key = "test_person";
  bus.subscribe(person_key, &foo_person);
  person p{1, "tom"};
  bus.publish(person_key, p);

  std::string ret_key = "test_ret";
  //bus.subscribe(ret_key, foo_return);

  bus.publish<std::string>(ret_key, 1);

  std::string result = bus.publish<std::string>(ret_key, 1);
  bus.publish(ret_key, 1);
}

int main(){
  test_msg_bus();

  return 0;
}
