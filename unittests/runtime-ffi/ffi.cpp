#include<boost/test/unit_test.hpp>
#include<gmp.h>
#include<mpfr.h>
#include<cstdint>
#include<cstdlib>
#include<cstring>
#include<vector>
#include<dlfcn.h>

#include "runtime/header.h"
#include "runtime/alloc.h"

#include "stdio.h"

#define KCHAR char
#define TYPETAG(type) "Lbl'hash'" #type "{}"
extern "C" {

  struct point {
    int x;
    int y;
  };

  struct point2 {
    struct point p;
  };

#define NUM_SYMBOLS 5
  const char * symbols[NUM_SYMBOLS] = {TYPETAG(struct), TYPETAG(uint), TYPETAG(sint), "inj{SortBytes{}}", "inj{SortFFIType{}}"};

  char * getTerminatedString(string * str);

  uint32_t getTagForSymbolName(const char *s) {
    for (int i = 0; i < NUM_SYMBOLS; i++) {
      if (0 == strcmp(symbols[i], s)) {
        return i + 1;
      }
    }

    return 0;
  }

  struct blockheader getBlockHeaderForSymbol(uint32_t tag) {
    return blockheader {tag};
  }

  void add_hash64(void*, uint64_t) {}
  mpz_ptr hook_FFI_address(string * fn);
  string * hook_FFI_call(mpz_t addr, struct list * args, struct list * types, block * ret);

  string * makeString(const KCHAR *, int64_t len = -1);

  struct list hook_LIST_element(block * value) {
    struct list l;
    l.a = (uint64_t)(koreAlloc(sizeof(std::vector<block *>)));
    ((std::vector<block *> *)l.a)->push_back(value);
    return l;
  }

  struct list hook_LIST_concat(struct list l1, struct list l2) {
    struct list l;
    l.a = (uint64_t)(koreAlloc(sizeof(std::vector<block *>)));
    ((std::vector<block *> *)l.a)->insert(((std::vector<block *> *)l.a)->end(),((std::vector<block *> *)l1.a)->begin(), ((std::vector<block *> *)l1.a)->end());
    ((std::vector<block *> *)l.a)->insert(((std::vector<block *> *)l.a)->end(),((std::vector<block *> *)l2.a)->begin(), ((std::vector<block *> *)l2.a)->end());
    return l;
  }

  size_t hook_LIST_size_long(struct list * l) {
    return ((std::vector<block *> *)l->a)->size();
  }

  block * hook_LIST_get(struct list * l, int idx) {
    return ((std::vector<block *> *) l->a)->at(idx);
  }

  mpz_ptr move_int(mpz_t i) {
    mpz_ptr result = (mpz_ptr)malloc(sizeof(__mpz_struct));
    *result = *i;
    return result;
  }

  floating *move_float(floating *i) {
    floating *result = (floating *)malloc(sizeof(floating));
    *result = *i;
    return result;
  }
}

BOOST_AUTO_TEST_SUITE(FfiTest)

BOOST_AUTO_TEST_CASE(address) {
  string * fn = makeString("timesTwo");
  mpz_ptr addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("utimesTwo");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("times");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("getX");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("increaseX");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("timesPoint");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("fakeFunction");
  addr = hook_FFI_address(fn);
  BOOST_CHECK_EQUAL(0, mpz_cmp_ui(addr, 0));
}

BOOST_AUTO_TEST_CASE(call) {
  /* int timesTwo(int x) */
  int x = -3;
  string * xargstr = makeString((char *) &x, sizeof(int)); 

  block * xarg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  xarg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(xarg->children, &xargstr, sizeof(string *));

  struct list args = hook_LIST_element(xarg);
  block * type_sint = (block *)((((uint64_t)getTagForSymbolName(TYPETAG(sint))) << 32) | 1);

  block * argtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  argtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}}"));
  memcpy(argtype->children, &type_sint, sizeof(block *));

  struct list types = hook_LIST_element(argtype);

  string * fn = makeString("timesTwo");
  mpz_ptr addr = hook_FFI_address(fn);

  string * bytes = hook_FFI_call(addr, &args, &types, type_sint);

  BOOST_CHECK(bytes != NULL);

  int ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x * 2);

  /* unsigned int utimesTwo(unsigned int x) */
  x = 4;
  xargstr = makeString((char *) &x, sizeof(int)); 

  memcpy(xarg->children, &xargstr, sizeof(string *));

  args = hook_LIST_element(xarg);
  block * type_uint = (block *)((((uint64_t)getTagForSymbolName(TYPETAG(uint))) << 32) | 1);
  memcpy(argtype->children, &type_uint, sizeof(block *));

  types = hook_LIST_element(argtype);

  fn = makeString("utimesTwo");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_uint);

  BOOST_CHECK(bytes != NULL);

  ret = *(unsigned int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x * 2);

  /* int times(int x, int y) */
  int y = 4;
  string * yargstr = makeString((char *) &y, sizeof(int));

  memcpy(argtype->children, &type_sint, sizeof(block *));

  block * yarg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  yarg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(yarg->children, &yargstr, sizeof(string *));

  struct list yargs = hook_LIST_element(yarg);

  args = hook_LIST_concat(args, yargs);
  types = hook_LIST_concat(types, types);

  fn = makeString("times");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x * y);

  /* struct point constructPoint(int x, int y) */
  block * structType = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  structType->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName(TYPETAG(struct)));

  struct list * structFields = static_cast<struct list *>(koreAlloc(sizeof(struct list)));
  *structFields = hook_LIST_element(argtype);
  *structFields = hook_LIST_concat(*structFields, *structFields);

  memcpy(structType->children, &structFields, sizeof(struct list *));

  fn = makeString("constructPoint");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, structType);

  struct point p = *(struct point *) bytes->data;
  BOOST_CHECK_EQUAL(p.x, x);
  BOOST_CHECK_EQUAL(p.y, y);

  /* int getX(void) */
  fn = makeString("getX");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, 1);

  /* void increaseX(void) */
  fn = makeString("increaseX");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);

  /* int getX(void) */
  fn = makeString("getX");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, 2);

  /* struct point {
   *  int x;
   *  int y;
   * }
   *
   * int timesPoint(struct point p) */
  p = {.x = 2, .y = 5};
  string * pargstr = makeString((char *) &p, sizeof(struct point));

  block * parg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  parg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(parg->children, &pargstr, sizeof(string *));

  args = hook_LIST_element(parg);

  block * new_argtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  new_argtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}}"));
  memcpy(new_argtype->children, &structType, sizeof(block *));
  types = hook_LIST_element(new_argtype);

  fn = makeString("timesPoint");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, p.x * p.y);

  /* struct point2 {
   *  struct point p;
   * }
   *
   * int timesPoint2(struct point2 p) */
  struct point2 p2 = {.p = p};
  string * pargstr2 = makeString((char *) &p2, sizeof(struct point2));

  memcpy(parg->children, &pargstr2, sizeof(string *));

  args = hook_LIST_element(parg);

  block * structType2 = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  structType2->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName(TYPETAG(struct)));

  block * structArgType = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  structArgType->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}}"));
  memcpy(structArgType->children, &structType, sizeof(block *));

  struct list * structFields2 = static_cast<struct list *>(koreAlloc(sizeof(struct list)));
  *structFields2 = hook_LIST_element(structArgType);

  memcpy(structType2->children, &structFields2, sizeof(struct list *));
  
  memcpy(new_argtype->children, &structType2, sizeof(block *));
  types = hook_LIST_element(new_argtype);

  fn = makeString("timesPoint2");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, p2.p.x * p2.p.y);

  /* Make sure there is no double free */
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, p2.p.x * p2.p.y);
}

BOOST_AUTO_TEST_SUITE_END()