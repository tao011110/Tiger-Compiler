#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  frame::Access *frameAccess = level->frame_->allocLocal(escape);
  return new Access(level, frameAccess);
}

class Cx {
public:
  temp::Label **trues_;
  temp::Label **falses_;
  tree::Stm *stm_;

  Cx(temp::Label **trues, temp::Label **falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() const = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() const = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) const = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

// expression
class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() const override { 
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    tree::CjumpStm *stm = new tree::CjumpStm(tree::NE_OP, exp_, new tree::ConstExp(0), t, f);
    temp::Label** trues, **falses;
    *trues = stm->true_label_;
    *falses = stm->false_label_;
    return Cx(trues, falses, stm);
  }
};

// non-value expression
class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override { 
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    return Cx(nullptr, nullptr, nullptr);
  }
};

//conditional expression
class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(temp::Label** trues, temp::Label** falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    return new tree::EseqExp(
      new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
      new tree::EseqExp(cx_.stm_, new tree::EseqExp(
        new tree::LabelStm(f),
        new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),new tree::ConstExp(0)),
        new tree::EseqExp(new tree::LabelStm(t), new tree::TempExp(r))))));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override { 
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  FillBaseTEnv();
  FillBaseVEnv();
  temp::Label *main_label_ = temp::LabelFactory::NamedLabel("tigermain");
  absyn_tree_.get()->Translate(venv_.get(), tenv_.get(), main_level_.get(), main_label_, errormsg_.get());
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  if(entry && typeid(*entry) == typeid(env::VarEntry)){
    env::VarEntry* ventry = (env::VarEntry*)entry;
    tr::Access *access = ventry->access_;
    while(level != ventry->access_->level_){
      level = level->parent_;
    }
    tree::Exp *exp = ventry->access_->access_->ToExp(level->frame_->getFramePtr());

    return new tr::ExpAndTy(new tr::ExExp(exp), ventry->ty_->ActualTy());
  }
  else{
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin fieldvar");
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *ty = var->ty_->ActualTy();
  if(!ty || typeid(*ty) != typeid(type::RecordTy)){
    errormsg->Error(pos_, "not a record type");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  type::FieldList *fields = (static_cast<type::RecordTy *>(ty))->fields_;
  std::list<type::Field *> filed_list = fields->GetList();
  std::list<type::Field *>::iterator tmp = filed_list.begin();
  int offset = 0;
  while(tmp != filed_list.end()){
    if((*tmp)->name_ == sym_){
      tr::Exp *filedVar = new tr::ExExp(
        new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, var->exp_->UnEx(), new tree::ConstExp(offset * reg_manager->WordSize()))));
      return new tr::ExpAndTy(filedVar, (*tmp)->ty_);
    }
    tmp++;
    offset++;
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin SubscriptVar");
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *subscript = subscript_->Translate(venv, tenv, level, label, errormsg);
  if(typeid(var->ty_) != typeid(type::ArrayTy)){
    errormsg->Error(pos_, "array type required");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  tr::Exp *subscriptVar = new tr::ExExp(
    new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, var->exp_->UnEx(), 
    new tree::BinopExp(tree::MUL_OP, subscript->exp_->UnEx(), new tree::ConstExp(reg_manager->WordSize())))));
  return new tr::ExpAndTy(subscriptVar, var->ty_->ActualTy());
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin VarExp");
  if(typeid(*var_) == typeid(SimpleVar)){
    errormsg->Error(pos_, "it is a SimpleVar");
    return ((SimpleVar *)(var_))->Translate(venv, tenv, level, label, errormsg);
  }
  if(typeid(*var_) == typeid(FieldVar)){
    errormsg->Error(pos_, "it is a FieldVar");
    return ((FieldVar *)(var_))->Translate(venv, tenv, level, label,  errormsg);
  }
  if(typeid(*var_) == typeid(SubscriptVar)){
    errormsg->Error(pos_, "it is a SubscriptVar");
    return ((SubscriptVar *)(var_))->Translate(venv, tenv, level, label, errormsg);
  }
  errormsg->Error(pos_, "it is not a VarExp");

  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::NilTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin CallExp");
  env::FunEntry *entry = (env::FunEntry *)(venv->Look(func_));
  tr::Exp *exp;
  if(!entry){
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  else{
    printf("defined function %s", func_->Name().data());
  }
  std::list<type::Ty *> formals = entry->formals_->GetList();
  type::Ty *result_ty = entry->result_;
  std::list<Exp *> args = args_->GetList();
  tree::ExpList *expList = new tree::ExpList();
  auto args_it = args.begin();
  for(type::Ty *formal : formals){
    if(args_it == args.end()){
      errormsg->Error((*args_it)->pos_, "Formal parameters are more!");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    if(!formal->IsSameType((*args_it)->Translate(venv, tenv, level, label, errormsg)->ty_)){
      errormsg->Error((*args_it)->pos_, "para type mismatch");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    expList->Append((*args_it)->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx());
    args_it++;
  }
  if(args_it != args.end()){
    errormsg->Error((*args_it)->pos_, "too many params in function %s", func_->Name().data());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  errormsg->Error(pos_, "end CallExp");
  // it is the outermost
  if(entry->level_->parent_){
    exp = new tr::ExExp(level->frame_->externalCall(entry->label_->Name(), expList));
  }
  else{
    tree::Exp *sl = new tree::TempExp(reg_manager->FramePointer());
    tr::Level *tmp_level = level;
    while(level != entry->level_){
      //How to find static link?
      sl = new tree::MemExp(new tree::BinopExp(tree::MINUS_OP, sl, new tree::ConstExp(reg_manager->WordSize())));
      tmp_level = tmp_level->parent_;
    }
    expList->Insert(sl);
    exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(entry->label_), expList));
  }

  return new tr::ExpAndTy(exp, result_ty);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // TODO:nothing ok
  errormsg->Error(pos_, "begin opexp");
  tr::ExpAndTy *left = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *right = right_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp = nullptr;
  tree::CjumpStm *cjstm = nullptr;
  
  if(oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP
      || oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP){
    if(typeid(left->ty_) != typeid(type::IntTy)){
      errormsg->Error(left_->pos_, "integer required");
    }
    if(typeid(right->ty_) != typeid(type::IntTy)){
      errormsg->Error(right_->pos_, "integer required");
    }
    switch (oper_)
    {
    case absyn::PLUS_OP:
      exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::PLUS_OP, left->exp_->UnEx(), right->exp_->UnEx()));
      break;
    case absyn::MINUS_OP:
      exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::MINUS_OP, left->exp_->UnEx(), right->exp_->UnEx()));
      break;
    case absyn::TIMES_OP:
      exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::MUL_OP, left->exp_->UnEx(), right->exp_->UnEx()));
      break;
    case absyn::DIVIDE_OP:
      exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::DIV_OP, left->exp_->UnEx(), right->exp_->UnEx()));
      break;
    default:
      break;
    }
  }
  else{
    if(oper_ == absyn::GE_OP || oper_ == absyn::GT_OP
      || oper_ == absyn::LE_OP || oper_ == absyn::LT_OP){
      if(typeid(left->ty_) != typeid(type::IntTy) || typeid(left->ty_) != typeid(type::StringTy)){
        errormsg->Error(left_->pos_, "integer or string required");
      }
      if(typeid(right->ty_) != typeid(type::IntTy) || typeid(right->ty_) != typeid(type::StringTy)){
        errormsg->Error(right_->pos_, "integer or string required");
      }
      if(!(left->ty_)->IsSameType(right->ty_)){
        errormsg->Error(pos_, "same type required");
      }
      //where is patchList??? Is this OK?
      switch ((oper_))
      {
      case absyn::GE_OP:
        cjstm = new tree::CjumpStm(tree::RelOp::GE_OP, left->exp_->UnEx(), right->exp_->UnEx(), nullptr, nullptr);
        break;
      case absyn::GT_OP:
        cjstm = new tree::CjumpStm(tree::RelOp::GT_OP, left->exp_->UnEx(), right->exp_->UnEx(), nullptr, nullptr);
        break;
      case absyn::LE_OP:
        cjstm = new tree::CjumpStm(tree::RelOp::LE_OP, left->exp_->UnEx(), right->exp_->UnEx(), nullptr, nullptr);
        break;
      case absyn::LT_OP:
        cjstm = new tree::CjumpStm(tree::RelOp::LT_OP, left->exp_->UnEx(), right->exp_->UnEx(), nullptr, nullptr);
        break;
      default:
        break;
      }
      
      exp = new tr::CxExp(&(cjstm->true_label_), &(cjstm->false_label_), cjstm);
    }
    else{
      if(oper_ == absyn::EQ_OP || oper_ == absyn::NEQ_OP){
        if(typeid(left->ty_) != typeid(type::IntTy) || typeid(left->ty_) != typeid(type::StringTy)
        || typeid(left->ty_) != typeid(type::ArrayTy) || typeid(left->ty_) != typeid(type::RecordTy)){
          errormsg->Error(left_->pos_, "integer or string or array or record required");
          return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
        }
        if(typeid(right->ty_) != typeid(type::IntTy) || typeid(right->ty_) != typeid(type::StringTy)
        || typeid(right->ty_) != typeid(type::ArrayTy) || typeid(right->ty_) != typeid(type::RecordTy)){
          errormsg->Error(right_->pos_, "integer or string or array or record required");
          return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
        }
        if(!(left->ty_)->IsSameType((right->ty_))){
          if(typeid(left->ty_) == typeid(type::RecordTy) && typeid(right->ty_) == typeid(type::NilTy)){
            // do nothing?
          }
          else{
            errormsg->Error(pos_, "same type required");
            return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
          }
        }
        if(typeid(left->ty_) == typeid(type::IntTy)){
          switch (oper_)
          {
          case EQ_OP:
            cjstm = new tree::CjumpStm(tree::RelOp::EQ_OP, left->exp_->UnEx(), right->exp_->UnEx(), nullptr, nullptr);
            break;
          case NEQ_OP:
            cjstm = new tree::CjumpStm(tree::RelOp::NE_OP, left->exp_->UnEx(), right->exp_->UnEx(), nullptr, nullptr);
            break;
          default:
            break;
          }
        }
        else{
          if(typeid(left->ty_) == typeid(type::StringTy)){
            std::initializer_list<tree::Exp *> list = {left->exp_->UnEx(), right->exp_->UnEx()};
            tree::ExpList *expList = new tree::ExpList(list);

            exp = new tr::ExExp(
              new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel("string_equal")), expList));
          }
          else{
            if(typeid(left->ty_) == typeid(type::ArrayTy)){
              errormsg->Error(pos_, "do cmp the array");
            }
            else{
              if(typeid(left->ty_) == typeid(type::RecordTy)){
                errormsg->Error(pos_, "do cmp the record");
              }
            }
          }
          
          exp = new tr::CxExp(&(cjstm->true_label_), &(cjstm->false_label_), cjstm);
        }
      }
    }
  }
  return new tr::ExpAndTy(exp, type::IntTy::Instance());
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *typ_ty = tenv->Look(typ_);
  if(!typ_ty){
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  std::list<EField *> fieldsList = fields_->GetList();
  std::vector<tr::Exp*> expList;
  for(EField *field : fieldsList){
    tr::ExpAndTy *tmp = field->exp_->Translate(venv, tenv, level, label, errormsg);
    expList.push_back(tmp->exp_);
  }

  std::initializer_list<tree::Exp *> list = {new tree::ConstExp(expList.size() * reg_manager->WordSize())};
  tree::ExpList *exps = new tree::ExpList(list);
  tr::Exp *recordExp = new tr::ExExp(
    new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel("alloc_record")), exps));
  
  return new tr::ExpAndTy(recordExp, typ_ty);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin seqexp");
  ExpList *seq = seq_;
  if(!seq){
    errormsg->Error(pos_, "not a SeqExp");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  std::list<Exp *> exp_list = seq->GetList();
  std::list<Exp *>::iterator it = exp_list.begin();
  tr::Exp *exp = nullptr;
  type::Ty *ty;
  while(it != exp_list.end()){
    tr::ExpAndTy *tmp = (*it)->Translate(venv, tenv, level, label, errormsg);
    exp = new tr::ExExp(new tree::EseqExp(exp->UnNx(), tmp->exp_->UnEx()));
    ty = tmp->ty_->ActualTy();
    it++;
  }
  errormsg->Error(pos_, "end seqexp");

  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin assignexp");
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp = exp_->Translate(venv, tenv, level, label,errormsg);
  if(typeid(*var_) == typeid(absyn::SimpleVar)){
    env::EnvEntry *entry = venv->Look(((SimpleVar *)var_)->sym_);
    if(entry->readonly_){
      errormsg->Error(pos_, "loop variable can't be assigned");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
  }
  if(!var->ty_->IsSameType(exp->ty_)){
    errormsg->Error(pos_, "unmatched assign exp");
  }
  tr::Exp *assignExp = new tr::NxExp(new tree::MoveStm(var->exp_->UnEx(), exp->exp_->UnEx()));

  return new tr::ExpAndTy(assignExp, type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin ifexp");
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *then = then_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp;
  if(typeid(test->ty_) != typeid(type::IntTy)){
    errormsg->Error(pos_, "integer required");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  if(elsee_){
    tr::ExpAndTy *elsee = elsee_->Translate(venv, tenv, level, label, errormsg);
    if(elsee->ty_->IsSameType(then->ty_)){
      temp::Label *trues = temp::LabelFactory().NewLabel();
      temp::Label *falses = temp::LabelFactory().NewLabel();
      temp::Label *meets = temp::LabelFactory().NewLabel();
      temp::Temp *r = temp::TempFactory().NewTemp();
      std::vector<temp::Label *> *jumps;
      jumps->push_back(meets);
      exp = new tr::ExExp(new tree::EseqExp(test->exp_->UnCx(errormsg).stm_, new tree::EseqExp(new tree::LabelStm(trues),
        new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), then->exp_->UnEx()), 
          new tree::EseqExp(new tree::JumpStm(new tree::NameExp(meets), jumps), 
            new tree::EseqExp(new tree::LabelStm(falses), 
              new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), elsee->exp_->UnEx()), 
                new tree::EseqExp(new tree::JumpStm(new tree::NameExp(meets), jumps),
                  new tree::EseqExp(new tree::LabelStm(meets), new tree::TempExp(r))))))))));
    }
    else{
      errormsg->Error(pos_, "then exp and else exp type mismatch");
      return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
    }
  }
  else{
    if(typeid(then->ty_) != typeid(type::VoidTy)){
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    temp::Label *trues = temp::LabelFactory().NewLabel();
    temp::Label *falses = temp::LabelFactory().NewLabel();
    exp = new tr::NxExp(new tree::SeqStm(test->exp_->UnCx(errormsg).stm_, 
      new tree::SeqStm(new tree::LabelStm(trues), 
        new tree::SeqStm(then->exp_->UnNx(), new tree::LabelStm(falses)))));
  }
  return new tr::ExpAndTy(exp, then->ty_->ActualTy());
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin whileexp");
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp;
  temp::Label *l_test = temp::LabelFactory().NewLabel();
  temp::Label *l_body = temp::LabelFactory().NewLabel();
  temp::Label *l_done = temp::LabelFactory().NewLabel();
  std::vector<temp::Label *> *jumps;
  jumps->push_back(l_test);

  if(typeid(test->ty_) != typeid(type::IntTy)){
    errormsg->Error(test_->pos_, "should be integer exp!");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  if(typeid(body->ty_) != typeid(type::VoidTy)){
    errormsg->Error(body_->pos_, "while body must produce no value");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  exp = new tr::NxExp(new tree::SeqStm(new tree::LabelStm(l_test), 
    new tree::SeqStm(test->exp_->UnCx(errormsg).stm_, 
      new tree::SeqStm(new tree::LabelStm(l_body), 
        new tree::SeqStm(body->exp_->UnNx(),
          new tree::SeqStm(new tree::JumpStm(new tree::NameExp(l_test), jumps), 
            new tree::LabelStm(l_done)))))));

  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin forexp");
  tr::ExpAndTy *lo = lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *hi = hi_->Translate(venv, tenv, level, label, errormsg);
  temp::Label *l_test_end = temp::LabelFactory().NewLabel();
  temp::Label *l_body = temp::LabelFactory().NewLabel();
  temp::Label *l_done = temp::LabelFactory().NewLabel();
  std::vector<temp::Label *> *jumps;
  jumps->push_back(l_test_end);
  tr::Exp *exp;

  if(typeid(lo->ty_) != typeid(type::IntTy)){
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  if(typeid(hi->ty_) != typeid(type::IntTy)){
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  tr::Access *access = tr::Access::AllocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(access, lo->ty_, true));
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, label, errormsg);
  tree::Exp *frameExp = access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  exp = new tr::NxExp(new tree::SeqStm(new tree::MoveStm(frameExp, lo->exp_->UnEx()), 
    // new tree::SeqStm(new tree::LabelStm(l_test),
      new tree::SeqStm(new tree::CjumpStm(tree::LE_OP, frameExp, hi->exp_->UnEx(), l_body, l_done),
        new tree::SeqStm(new tree::LabelStm(l_body),
          new tree::SeqStm(body->exp_->UnNx(),
            new tree::SeqStm(new tree::CjumpStm(tree::LT_OP, frameExp, hi->exp_->UnEx(), l_test_end, l_done),
              new tree::SeqStm(new tree::LabelStm(l_test_end),
                new tree::SeqStm(new tree::MoveStm(frameExp, new tree::BinopExp(tree::BinOp::PLUS_OP, frameExp, new tree::ConstExp(1))),
                  new tree::SeqStm(new tree::JumpStm(new tree::NameExp(l_body), jumps),
                    new tree::LabelStm(l_done))))))))));

  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::vector<temp::Label *> *jumps;
  jumps->push_back(label);
  tr::Exp *exp = new tr::NxExp(new tree::JumpStm(new tree::NameExp(label), jumps));

  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin let exp");
  std::list<Dec *> decs = decs_->GetList();
  tree::Stm *stm = nullptr;
  tree::Exp *exp = nullptr;
  for(Dec *dec : decs){
    if(!stm){
      stm = dec->Translate(venv, tenv, level, label, errormsg)->UnNx();
    }
    else{
      stm = new tree::SeqStm(stm, dec->Translate(venv, tenv, level, label, errormsg)->UnNx());
    }
  }

  errormsg->Error(pos_, "parse body");
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, label, errormsg);
  if(!stm){
    exp = body->exp_->UnEx();
  }
  else{
    exp = new tree::EseqExp(stm, body->exp_->UnEx());
  }
  errormsg->Error(pos_, "end LetExp");

  return new tr::ExpAndTy(new tr::ExExp(exp), body->ty_);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin arrayexp");
  type::Ty *typ_ty = tenv->Look(typ_)->ActualTy();
  tr::ExpAndTy *size = size_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *init = init_->Translate(venv, tenv, level, label, errormsg);
  if(!typ_ty){
    errormsg->Error(pos_, "%s should be a defined type", typ_->Name().data());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  if(typeid(*typ_ty) != typeid(type::ArrayTy)){
    errormsg->Error(pos_, "%s should be an array type", typ_->Name().data());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  if(typeid(size->ty_) != typeid(type::IntTy)){
    errormsg->Error(size_->pos_, "should be an integer");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  if(!((type::ArrayTy *)typ_ty)->ty_->IsSameType(init->ty_)){
    errormsg->Error(pos_, "type mismatch");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  errormsg->Error(pos_, "end arrayexp");
  std::initializer_list<tree::Exp *> list = {size->exp_->UnEx(), init->exp_->UnEx()};
  tree::ExpList *expList = new tree::ExpList(list);
  tr::Exp *exp = new tr::ExExp(
    new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel("init_array")), expList));

  return new tr::ExpAndTy(exp, typ_ty->ActualTy());
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin FunctionDec");
  std::list<FunDec *> functions = functions_->GetList();
  for(FunDec *function : functions){
    errormsg->Error(pos_, "do first");
    
    type::TyList *formals = function->params_->MakeFormalTyList(tenv, errormsg);
    std::list<bool> formal_escape;
    std::list<Field *> fieldList = function->params_->GetList();
    for(Field * param : fieldList){
      formal_escape.push_back(param->escape_);
    }
    tr::Level *new_level = tr::newLevel(level, function->name_, formal_escape);
    if(function->result_){
      type::Ty *result_ty = tenv->Look(function->result_);
      venv->Enter(function->name_, new env::FunEntry(new_level, function->name_, formals, result_ty));
    }
    else{
      venv->Enter(function->name_, new env::FunEntry(new_level, function->name_, formals, type::VoidTy::Instance()));
    }
  }
  errormsg->Error(pos_, "finish first");

  for(FunDec *function : functions){
    int count = 0;
    errormsg->Error(pos_, "do second");
    type::TyList *formals = function->params_->MakeFormalTyList(tenv, errormsg);
    auto formal_it = formals->GetList().begin();
    auto param_it = (function->params_->GetList()).begin();
    venv->BeginScope();
    env::EnvEntry *fun = venv->Look(function->name_);
    std::list<frame::Access*> accessList = ((env::FunEntry*)fun)->level_->frame_->formals_;
    std::list<frame::Access*>::iterator access = accessList.begin();
    for(; param_it != (function->params_->GetList()).end(); param_it++){
      venv->Enter((*param_it)->name_, 
        new env::VarEntry(new tr::Access(((env::FunEntry*)fun)->level_, *access), *formal_it));
      formal_it++;
      access++;
    }
    errormsg->Error(pos_, "parse body");
    tr::ExpAndTy *body = function->body_->Translate(venv, tenv, level, label, errormsg);

    if(((env::FunEntry *)fun)->result_->ActualTy()->IsSameType(type::VoidTy::Instance())){
      if(!body->ty_->IsSameType(type::VoidTy::Instance())){
        errormsg->Error(pos_, "procedure returns value");
      }
      else{
        errormsg->Error(pos_, "did right procedure returns value");
      }
    }
    else{
      errormsg->Error(pos_, "should returns value");
    }
    venv->EndScope();
    tree::Stm *stm = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), body->exp_->UnEx());
    stm = frame::procEntryExit1(((env::FunEntry *)fun)->level_->frame_, stm);
    frags->PushBack(new frame::ProcFrag(body->exp_->UnNx(), ((env::FunEntry *)fun)->level_->frame_));
  }
  errormsg->Error(pos_, "end FunctionDec");
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin vardec");
  tr::ExpAndTy *init = init_->Translate(venv, tenv, level, label, errormsg);
  if(typ_){
    type::Ty *typ_ty = tenv->Look(typ_);
    if(!typ_ty){
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return nullptr;
    }
    if(!typ_ty->IsSameType(init->ty_)){
      if(typeid(*typ_ty) == typeid(type::RecordTy) && typeid(init->ty_) == typeid(type::NilTy)){
        venv->Enter(var_, new env::VarEntry(typ_ty));
      }
      else{
        errormsg->Error(pos_, "type mismatch");
        return nullptr;
      }
    }
    else{
      venv->Enter(var_, new env::VarEntry(typ_ty));
    }
  }
  else{
    if(typeid(init->ty_) == typeid(type::NilTy)){
      errormsg->Error(pos_, "init should not be nil without type specified");
      return nullptr;
    }
    venv->Enter(var_, new env::VarEntry(init->ty_));
  }
  tr::Access *access = tr::Access::AllocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(access, init->ty_));
  errormsg->Error(pos_, "end vardec");
  

  return new tr::NxExp(new tree::MoveStm(access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer())), init->exp_->UnEx()));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin typedec");
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
        return nullptr;
      }
    }
    tenv->Enter(nameAndTy->name_, new type::NameTy(nameAndTy->name_, nullptr));
  }
  /* At the second time, parse the body with the tenv */
  for(auto &nameAndTy : nameAndTyList){
    type::NameTy *type = static_cast<type::NameTy*>(tenv->Look(nameAndTy->name_));
    type->ty_ = nameAndTy->ty_->Translate(tenv, errormsg);
    tenv->Set(nameAndTy->name_, type);
  }
  for(auto &nameAndTy : nameAndTyList){
    type::Ty *name_ty = tenv->Look(nameAndTy->name_);
    type::Ty *tmp = name_ty;
    while(typeid(*tmp) == typeid(type::NameTy)){
      tmp = ((type::NameTy *)tmp)->ty_;
      if(((type::NameTy *)tmp) == ((type::NameTy *)name_ty)){
        errormsg->Error(pos_, "illegal type cycle");
        return nullptr;
      }
    }
  }
  errormsg->Error(pos_, "end typedec");

  //TODO: is this true?
  return nullptr;
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin namety");
  type::Ty *name_ty = tenv->Look(name_);
  if(!name_ty){
    errormsg->Error(pos_, "should be defined type");
    return type::VoidTy::Instance();
  }
  return new type::NameTy(name_, name_ty);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin recordty");
  type::FieldList *fields = record_->MakeFieldList(tenv, errormsg);
  errormsg->Error(pos_, "end recordty");
  
  return new type::RecordTy(fields);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  errormsg->Error(pos_, "begin arrayty");
  type::Ty *array_ty = tenv->Look(array_);
  if(!array_ty){
    errormsg->Error(pos_, "should be defined type!");
    return type::VoidTy::Instance();
  }
  return new type::ArrayTy(array_ty);
}

} // namespace absyn
