#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"
#include <iostream>
#include <vector>

namespace absyn {
  
int isInLoop = 0;

std::vector<sym::Symbol*> funcVec;

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  if(entry && typeid(*entry) == typeid(env::VarEntry)){
    return (static_cast<env::VarEntry *>(entry))->ty_->ActualTy();
  }
  else{
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
    return type::IntTy::Instance();
  }
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin fieldvar");
  type::Ty *ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(!ty || typeid(*ty) != typeid(type::RecordTy)){
    errormsg->Error(pos_, "not a record type");
    return type::IntTy::Instance();
  }
  type::FieldList *fields = (static_cast<type::RecordTy *>(ty))->fields_;
  std::list<type::Field *> filed_list = fields->GetList();
  std::list<type::Field *>::iterator tmp = filed_list.begin();
  while(tmp != filed_list.end()){
    if((*tmp)->name_ == sym_){
      return (*tmp)->ty_->ActualTy();
    }
    tmp++;
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
  return type::IntTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin SubscriptVar");
  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *subscript_ty = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if(typeid(*var_ty) != typeid(type::ArrayTy)){
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }
  if(typeid(*subscript_ty) != typeid(type::IntTy)){
    errormsg->Error(pos_, "it is not an integer");
    return type::IntTy::Instance();
  }

  return (static_cast<type::ArrayTy *>(var_ty))->ty_->ActualTy();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin VarExp");
  if(typeid(*var_) == typeid(SimpleVar)){
    return ((SimpleVar *)(var_))->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  if(typeid(*var_) == typeid(FieldVar)){
    return ((FieldVar *)(var_))->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  if(typeid(*var_) == typeid(SubscriptVar)){
    return ((SubscriptVar *)(var_))->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  //errormsg->Error(pos_, "it is not a VarExp");

  return type::IntTy::Instance();
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin CallExp");
  env::FunEntry *entry = (env::FunEntry *)(venv->Look(func_));
  if(!entry){
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return type::VoidTy::Instance();
  }
  else{
    printf("defined function %s", func_->Name().data());
  }
  std::list<type::Ty *> formals = entry->formals_->GetList();
  type::Ty *result_ty = entry->result_;
  std::list<Exp *> args = args_->GetList();
  auto args_it = args.begin();
  for(type::Ty *formal : formals){
    if(args_it == args.end()){
      errormsg->Error((*args_it)->pos_, "Formal parameters are more!");
      return type::IntTy::Instance();
    }
    if(!formal->IsSameType((*args_it)->SemAnalyze(venv, tenv, labelcount, errormsg))){
      errormsg->Error((*args_it)->pos_, "para type mismatch");
      return type::IntTy::Instance();
    }
    args_it++;
  }
  if(args_it != args.end()){
    errormsg->Error((*args_it)->pos_, "too many params in function %s", func_->Name().data());
    return type::IntTy::Instance();
  }
  //errormsg->Error(pos_, "end CallExp");
  return result_ty->ActualTy();
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin opexp");
  type::Ty *left_ty = left_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *right_ty = right_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP
      || oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP){
    if(typeid(*left_ty) != typeid(type::IntTy)){
      errormsg->Error(left_->pos_, "integer required");
    }
    if(typeid(*right_ty) != typeid(type::IntTy)){
      errormsg->Error(right_->pos_, "integer required");
    }
  }
  else{
    if(oper_ == absyn::GE_OP || oper_ == absyn::GT_OP
      || oper_ == absyn::LE_OP || oper_ == absyn::LT_OP){
      // if(typeid(*left_ty) != typeid(type::IntTy) || typeid(*left_ty) != typeid(type::StringTy)){
      //   errormsg->Error(left_->pos_, "integer or string required");
      // }
      // if(typeid(*right_ty) != typeid(type::IntTy) || typeid(*right_ty) != typeid(type::StringTy)){
      //   errormsg->Error(right_->pos_, "integer or string required");
      // }
      if(!left_ty->IsSameType(right_ty)){
        errormsg->Error(pos_, "same type required");
      }
    }
    else{
      if(oper_ == absyn::EQ_OP || oper_ == absyn::NEQ_OP){
        if(!left_ty->IsSameType(right_ty)){
          if(typeid(*left_ty) == typeid(type::RecordTy) && typeid(*right_ty) == typeid(type::NilTy)){
            
          }
          else{
            errormsg->Error(pos_, "same type required");
          }
        }
      }
    }
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin recordexp");
  type::Ty *typ_ty = tenv->Look(typ_);
  if(!typ_ty){
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::IntTy::Instance();
  }
  
  return typ_ty;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin seqexp");
  ExpList *seq = seq_;
  if(!seq){
    //errormsg->Error(pos_, "not a SeqExp");
    return type::VoidTy::Instance();
  }
  std::list<Exp *> exp_list = seq->GetList();
  std::list<Exp *>::iterator tmp = exp_list.begin();
  type::Ty *result;
  while(tmp != exp_list.end()){
    result = (*tmp)->SemAnalyze(venv, tenv, labelcount, errormsg);
    tmp++;
  }
  tmp--;
  //errormsg->Error(pos_, "end seqexp");

  return result;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin assignexp");
  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*var_) == typeid(absyn::SimpleVar)){
    env::EnvEntry *entry = venv->Look(((SimpleVar *)var_)->sym_);
    if(entry->readonly_){
      errormsg->Error(pos_, "loop variable can't be assigned");
      return type::IntTy::Instance();
    }
  }
  if(!var_ty->IsSameType(exp_ty)){
    errormsg->Error(pos_, "unmatched assign exp");
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin ifexp");
  type::Ty *test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*test_ty) != typeid(type::IntTy)){
    errormsg->Error(pos_, "integer required");
    return type::VoidTy::Instance();
  }
  if(elsee_){
    type::Ty *elsee_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(elsee_ty->IsSameType(then_ty)){
      return then_ty->ActualTy();
    }
    else{
      errormsg->Error(pos_, "then exp and else exp type mismatch");
      return type::VoidTy::Instance();
    }
  }
  else{
    if(typeid(*then_ty) != typeid(type::VoidTy)){
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
      return type::VoidTy::Instance();
    }
    return type::VoidTy::Instance();
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin whileexp");
  isInLoop++;
  type::Ty *test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*test_ty) != typeid(type::IntTy)){
    errormsg->Error(test_->pos_, "should be integer exp!");
  }
  if(typeid(*body_ty) != typeid(type::VoidTy)){
    errormsg->Error(body_->pos_, "while body must produce no value");
  }
  isInLoop--;
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin forexp");
  isInLoop++;
  type::Ty *lo_ty = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *hi_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*lo_ty) != typeid(type::IntTy)){
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  }
  if(typeid(*hi_ty) != typeid(type::IntTy)){
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }
  venv->Enter(var_, new env::VarEntry(lo_ty, true));
  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  isInLoop--;

  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(!isInLoop){
    errormsg->Error(pos_, "break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin let exp");
  std::vector<sym::Symbol*> curVec = funcVec;
  funcVec.clear();
  std::list<Dec *> decs = decs_->GetList();
  for(Dec *dec : decs){
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  //errormsg->Error(pos_, "parse body");
  type::Ty *result;
  result = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  //errormsg->Error(pos_, "end LetExp");
  funcVec = curVec;
  return result;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin arrayexp");
  type::Ty *typ_ty = tenv->Look(typ_)->ActualTy();
  type::Ty *size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(!typ_ty){
    errormsg->Error(pos_, "%s should be a defined type", typ_->Name().data());
  }
  if(typeid(*typ_ty) != typeid(type::ArrayTy)){
    errormsg->Error(pos_, "%s should be an array type", typ_->Name().data());
  }
  if(typeid(*size_ty) != typeid(type::IntTy)){
    errormsg->Error(size_->pos_, "should be an integer");
  }
  if(!((type::ArrayTy *)typ_ty)->ty_->IsSameType(init_ty)){
    errormsg->Error(pos_, "type mismatch");
  }
  //errormsg->Error(pos_, "end arrayexp");

  return typ_ty->ActualTy();
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin FunctionDec");
  std::list<FunDec *> functions = functions_->GetList();
  for(FunDec *function : functions){
    //errormsg->Error(pos_, "do first");
    funcVec.push_back(function->name_);
    
    type::TyList *formals = function->params_->MakeFormalTyList(tenv, errormsg);
    if(function->result_){
      type::Ty *result_ty = tenv->Look(function->result_);
      venv->Enter(function->name_, new env::FunEntry(formals, result_ty));
    }
    else{
      venv->Enter(function->name_, new env::FunEntry(formals, type::VoidTy::Instance()));
    }
  }
  //errormsg->Error(pos_, "finish first");
  for(FunDec *function : functions){
    int count = 0;
    for(sym::Symbol *tmp : funcVec){
      if(function->name_ == tmp){
        count++;
      }
      if(count >= 2){
        errormsg->Error(pos_, "two functions have the same name");
        return;
      }
    }
    //errormsg->Error(pos_, "do second");
    type::TyList *formals = function->params_->MakeFormalTyList(tenv, errormsg);
    auto formal_it = formals->GetList().begin();
    auto param_it = (function->params_->GetList()).begin();
    venv->BeginScope();
    for(; param_it != (function->params_->GetList()).end(); param_it++){
      // errormsg->Error(pos_, " entert the %s", (*param_it)->name_->Name().data());
      venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it));
      formal_it++;
    }
    //errormsg->Error(pos_, "parse body");
    type::Ty *ty = function->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    env::EnvEntry *fun = venv->Look(function->name_);
    if(((env::FunEntry *)fun)->result_->ActualTy()->IsSameType(type::VoidTy::Instance())){
      if(!ty->IsSameType(type::VoidTy::Instance())){
        errormsg->Error(pos_, "procedure returns value");
      }
    }
    // else{
    //   errormsg->Error(pos_, "should returns value");
    // }
    venv->EndScope();
  }
  //errormsg->Error(pos_, "end FunctionDec");
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typ_){
    type::Ty *typ_ty = tenv->Look(typ_);
    if(!typ_ty){
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return;
    }
    if(!typ_ty->IsSameType(init_ty)){
      if(typeid(*typ_ty) == typeid(type::RecordTy) && typeid(*init_ty) == typeid(type::NilTy)){
        venv->Enter(var_, new env::VarEntry(typ_ty));
      }
      else{
        errormsg->Error(pos_, "type mismatch");
        return;
      }
    }
    else{
      venv->Enter(var_, new env::VarEntry(typ_ty));
    }
  }
  else{
    if(typeid(*init_ty) == typeid(type::NilTy)){
      errormsg->Error(pos_, "init should not be nil without type specified");
      return;
    }
    venv->Enter(var_, new env::VarEntry(init_ty));
  }
  //errormsg->Error(pos_, "end vardec");
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin typedec");
   std::list<NameAndTy*> nameAndTyList = types_->GetList();
  /* At the first time, parse the header and write it into tenv */
  for(auto &nameAndTy : nameAndTyList){
    int count = 0;
    for(auto &tmp : nameAndTyList){
      if(nameAndTy->name_ == tmp->name_){
        count++;
      }
      if(count >= 2){
        errormsg->Error(pos_, "two types have the same name");
        return;
      }
    }
    tenv->Enter(nameAndTy->name_, new type::NameTy(nameAndTy->name_, nullptr));
  }
  /* At the second time, parse the body with the tenv */
  for(auto &nameAndTy : nameAndTyList){
    type::NameTy *type = static_cast<type::NameTy*>(tenv->Look(nameAndTy->name_));
    type->ty_ = nameAndTy->ty_->SemAnalyze(tenv, errormsg);
    tenv->Set(nameAndTy->name_, type);
  }
  for(auto &nameAndTy : nameAndTyList){
    type::Ty *name_ty = tenv->Look(nameAndTy->name_);
    type::Ty *tmp = name_ty;
    while(typeid(*tmp) == typeid(type::NameTy)){
      tmp = ((type::NameTy *)tmp)->ty_;
      if(((type::NameTy *)tmp) == ((type::NameTy *)name_ty)){
        errormsg->Error(pos_, "illegal type cycle");
        return;
      }
    }
  }
  //errormsg->Error(pos_, "end typedec");
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin namety");
  type::Ty *name_ty = tenv->Look(name_);
  if(!name_ty){
    errormsg->Error(pos_, "should be defined type");
    return type::VoidTy::Instance();
  }
  return new type::NameTy(name_, name_ty);
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin recordty");
  type::FieldList *fields = record_->MakeFieldList(tenv, errormsg);
  //errormsg->Error(pos_, "end recordty");
  
  return new type::RecordTy(fields);
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  //errormsg->Error(pos_, "begin arrayty");
  type::Ty *array_ty = tenv->Look(array_);
  if(!array_ty){
    errormsg->Error(pos_, "should be defined type!");
    return type::VoidTy::Instance();
  }
  return new type::ArrayTy(array_ty);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
