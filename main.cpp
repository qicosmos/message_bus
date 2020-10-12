#include <iostream>
#include "message_bus.h"
using namespace purecpp;

message_bus& bus = message_bus::get();

void foo3(int a,  std::string& b, std::shared_ptr<int> c) {
  std::cout <<b<< " foo3\n";
  b = "test";
  *c = 30;
}

struct person{
  int id;
  std::string name;

  void foo(int a){
    std::cout<<a<<'\n';
  }

  std::string foo_ret(int a){
    return std::to_string(a+2);
  }

  int foo_three(int& a, std::string& b, int c){
    a = 42;
    b = "it is a test";
    return c+a;
  }
};

void foo_person(const person& p){
  std::cout<<p.id<<", "<<p.name<<'\n';
}

std::string foo_return(int a){
  return std::to_string(a+2);
}

void test_msg_bus(){
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
  bus.subscribe(ret_key, foo_return);

//  bus.publish(ret_key, 1);//will assert false

  std::string result = bus.publish<std::string>(ret_key, 1);
  std::cout <<result<<"\n";
}

void test_msg_bus1(){
  std::string key = "key1";
  std::string key2 = "key2";
  std::string key3 = "key3";
  person p{1, "tom"};
  bus.subscribe(key, &person::foo, &p);
  bus.subscribe(key2, &person::foo_ret, &p);
  bus.subscribe(key3, &person::foo_three, &p);

  bus.publish(key, 1);
  std::string ret = bus.publish<std::string>(key2, 1);

  int mod_int = 1;
  std::string mod_str = "n";
  int result = bus.publish<int>(key3, mod_int, mod_str, 1);

  std::cout<<ret<<'\n';
  std::cout<<result<<'\n';
}

int main(){
  test_msg_bus();
  test_msg_bus1();

  return 0;
}
