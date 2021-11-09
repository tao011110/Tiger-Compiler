#include "straightline/slp.h"
#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int argsNum1 = stm1->MaxArgs();
  int argsNum2 = stm2->MaxArgs();
  if (argsNum1 >= argsNum2) {
    return argsNum1;
  } else {
    return argsNum2;
  }
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  t = stm1->Interp(t);
  return stm2->Interp(t);
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *intTable = exp->Interp(t);
  return intTable->t->Update(id, intTable->i);
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exps->MaxArgs();
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *intTable = exps->Interp(t);
  return intTable->t;
}

int A::IdExp::MaxArgs() const { return 0; }

int A::NumExp::MaxArgs() const { return 0; }

int A::OpExp::MaxArgs() const {
  int argsNum1 = left->MaxArgs();
  int argsNum2 = right->MaxArgs();
  if (argsNum1 >= argsNum2) {
    return argsNum1;
  } else {
    return argsNum2;
  }
}

int A::EseqExp::MaxArgs() const {
  int argsNum1 = stm->MaxArgs();
  int argsNum2 = exp->MaxArgs();
  if (argsNum1 >= argsNum2) {
    return argsNum1;
  } else {
    return argsNum2;
  }
}

int A::PairExpList::MaxArgs() const {
  int argsNum1 = exp->MaxArgs();
  int argsNum2 = tail->MaxArgs();
  int expsNum = NumExps();
  int argsNum = 0;
  if (argsNum1 >= argsNum2) {
    argsNum = argsNum1;
  } else {
    argsNum = argsNum2;
  }
  if (expsNum >= argsNum) {
    return expsNum;
  } else {
    return argsNum;
  }
}

int A::PairExpList::NumExps() const { return 1 + tail->NumExps(); }

int A::LastExpList::MaxArgs() const { return exp->MaxArgs(); }

int A::LastExpList::NumExps() const { return 1; }

IntAndTable *A::IdExp::Interp(Table *t) const {
  int value = t->Lookup(id);
  IntAndTable *intTable = new IntAndTable(value, t);
  return intTable;
}

IntAndTable *A::NumExp::Interp(Table *t) const {
  IntAndTable *intTable = new IntAndTable(num, t);
  return intTable;
}

IntAndTable *A::OpExp::Interp(Table *t) const {
  IntAndTable *leftIntTable = left->Interp(t);
  t = leftIntTable->t;
  IntAndTable *rightIntTable = right->Interp(t);
  t = rightIntTable->t;
  switch (oper) {
  case 0: {
    int result = leftIntTable->i + rightIntTable->i;
    return new IntAndTable(result, t);
  }
  case 1: {
    int result = leftIntTable->i - rightIntTable->i;
    return new IntAndTable(result, t);
  }
  case 2: {
    int result = leftIntTable->i * rightIntTable->i;
    return new IntAndTable(result, t);
  }
  case 3: {
    int result = leftIntTable->i / rightIntTable->i;
    return new IntAndTable(result, t);
  }
  }
}

IntAndTable *A::EseqExp::Interp(Table *t) const {
  t = stm->Interp(t);
  IntAndTable *intTable = exp->Interp(t);
  return new IntAndTable(intTable->i, intTable->t);
}

IntAndTable *A::PairExpList::Interp(Table *t) const {
  IntAndTable *intTable = exp->Interp(t);
  std::cout << intTable->i <<" ";
  return new IntAndTable(intTable->i, tail->Interp(intTable->t)->t);
}

IntAndTable *A::LastExpList::Interp(Table *t) const {
  IntAndTable *intTable = exp->Interp(t);
  std::cout << intTable->i << std::endl;
  return new IntAndTable(intTable->i, intTable->t);
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
} // namespace A
