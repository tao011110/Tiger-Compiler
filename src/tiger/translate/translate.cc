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

Access *Access::allocLocal(Level *level, bool escape) {
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
    printf("ExExp UnEx");
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    printf("ExExp UnNx");
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    printf("begin ExExp UnCx");
    tree::CjumpStm* stm=new tree::CjumpStm(tree::NE_OP,exp_,new tree::ConstExp(0),nullptr,nullptr);
    temp::Label** trues=&stm->true_label_;
    temp::Label** falses=&stm->false_label_;

    return Cx(trues,falses,stm);
  }
};

// non-value expression
class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    printf("NxExp UnEx");
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override { 
    /* TODO: Put your lab5 code here */
    printf("NxExp UnNx");
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    printf("NxExp UnCx");
    // errormsg->Error(1, "wafwdada");
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
    printf("begin NxExp UnEx");
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    *(cx_.trues_)=t;
    *(cx_.falses_)=f;
    return new tree::EseqExp(
      new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
        new tree::EseqExp(cx_.stm_, 
          new tree::EseqExp(new tree::LabelStm(f),
            new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),new tree::ConstExp(0)),
              new tree::EseqExp(new tree::LabelStm(t), new tree::TempExp(r))))));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    printf("CxExp UnNx");
    temp::Label* label=temp::LabelFactory::NewLabel();
    *(cx_.trues_)=label;
    *(cx_.falses_)=label;
    return new tree::SeqStm(cx_.stm_,new tree::LabelStm(label)); // convert to exp without return value
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override { 
    /* TODO: Put your lab5 code here */
    printf("CxExp UnCx");
    return cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  temp::Label *main_label_ = temp::LabelFactory::NamedLabel("tigermain");
  std::list<bool> escapes;
  tr::Level *level = new tr::Level(frame::newFrame(main_label_, std::list<bool>()), nullptr);
  main_level_ .reset(level);
  tenv_ = std::make_unique<env::TEnv>();
  venv_ = std::make_unique<env::VEnv>();
  // fill after main level init!
  FillBaseTEnv();
  FillBaseVEnv();
  tr::ExpAndTy *tree = absyn_tree_.get()->Translate(venv_.get(), tenv_.get(), level, main_label_, errormsg_.get());
  tree::Stm *tmp = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), tree->exp_->UnEx());
  tmp = frame::procEntryExit1(level->frame_, tmp);
  
  frags->PushBack(new frame::ProcFrag(tmp, level->frame_));
}

tr::Level *newLevel(Level *parent, temp::Label *name, std::list<bool> formals){
  formals.push_front(true);
  frame::Frame *newFrame = frame::newFrame(name, formals);
  tr::Level *level = new tr::Level(newFrame, parent);

  return level;
}

tr::Level *outermost(){
  temp::Label *label = temp::LabelFactory::NamedLabel(std::string("tigermain"));
  std::list<bool> formals;
  Level *level = newLevel(nullptr, label, formals);

  return level;
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
  // errormsg->Error(pos_, "begin simple var and sym is %s", sym_->Name().data());
  env::EnvEntry *entry = venv->Look(sym_);
  if(entry && typeid(*entry) == typeid(env::VarEntry)){
    // errormsg->Error(pos_, "do varEntry");
    env::VarEntry* ventry = (env::VarEntry*)entry;
    tr::Access *access = ventry->access_;
    tree::Exp *exp = new tree::TempExp(reg_manager->FramePointer());
    while(level != ventry->access_->level_){
      exp = level->frame_->formals_.front()->ToExp(exp);
      level= level->parent_;
    }
    return new tr::ExpAndTy(new tr::ExExp(access->access_->ToExp(exp)), ventry->ty_->ActualTy());
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
  tr::ExpAndTy* var = var_->Translate(venv, tenv, level, label, errormsg);
  if(typeid(*(var->ty_)) != typeid(type::RecordTy)){
    errormsg->Error(this->pos_, "not a record type");
  }
  std::list<type::Field*> field_list = (static_cast<type::RecordTy*>(var->ty_))->fields_->GetList();
  std::list<type::Field *>::iterator tmp = field_list.begin();
  int offset=0;
  while(tmp != field_list.end()){
    //errormsg->Error(pos_, "parse a field %s", (*tmp)->name_->Name().data());
    if((*tmp)->name_->Name() == sym_->Name()){
      tr::Exp *filedVar = new tr::ExExp(new tree::MemExp(
        new tree::BinopExp(tree::PLUS_OP, var->exp_->UnEx(), new tree::ConstExp(offset * reg_manager->WordSize()))));
      return new tr::ExpAndTy(filedVar, (*tmp)->ty_->ActualTy());
    }
    tmp++;
    offset++;
  }
  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *subscript = subscript_->Translate(venv, tenv, level, label, errormsg);
  if(typeid(var->ty_) != typeid(type::ArrayTy)){
    // errormsg->Error(pos_, "array type required");
  }
  if(typeid(((type::ArrayTy*)var->ty_)->ty_->ActualTy()) != typeid(type::RecordTy)){
    // errormsg->Error(pos_, "not a record");
  }
  tr::Exp *subscriptVar = new tr::ExExp(
    new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, var->exp_->UnEx(), 
      new tree::BinopExp(tree::MUL_OP, subscript->exp_->UnEx(), new tree::ConstExp(reg_manager->WordSize())))));
  return new tr::ExpAndTy(subscriptVar, ((type::ArrayTy*)var->ty_)->ty_->ActualTy());
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return this->var_->Translate(venv,tenv,level,label,errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin nil exp");
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  printf("begin int exp %d", val_);
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)), type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *label_tmp = temp::LabelFactory::NewLabel();
  frame::StringFrag *str_frag = new frame::StringFrag(label_tmp, str_);
  frags->PushBack(str_frag);
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(label_tmp)), type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  printf("callexp %s\n", func_->Name().data());
  env::FunEntry *entry = (env::FunEntry *)(venv->Look(func_));
  tr::Exp *exp;
  if(!entry){
    errormsg->Error(pos_, "undefined function ");
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    // return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  else{
    //errormsg->Error(pos_, "defined function %s", func_->Name().data());
  }
  std::list<type::Ty *> formals = entry->formals_->GetList();
  type::Ty *result_ty = entry->result_;
  std::list<Exp *> args = args_->GetList();
  tree::ExpList *expList = new tree::ExpList();
  auto args_it = args.begin();
  for(type::Ty *formal : formals){
    if(args_it == args.end()){
      //errormsg->Error((*args_it)->pos_, "Formal parameters are more!");
    }
    if(!formal->IsSameType((*args_it)->Translate(venv, tenv, level, label, errormsg)->ty_)){
      //errormsg->Error((*args_it)->pos_, "para type mismatch");
    }
    expList->Append((*args_it)->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx());
    args_it++;
  }
  // it is the outermost
  if(entry->level_->parent_ == nullptr){
    exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(
      temp::LabelFactory::NamedLabel(temp::LabelFactory::LabelString(func_))), expList));
  }
  else{
    tree::Exp *sl = new tree::TempExp(reg_manager->FramePointer());
    while(level != entry->level_->parent_){
      sl = level->frame_->formals_.front()->ToExp(sl);
      level = level->parent_;
    }
    expList->Insert(sl);
    exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(entry->label_), expList));
  }

  return new tr::ExpAndTy(exp, entry->result_);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *left = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *right = right_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp = nullptr;
  tree::CjumpStm *cjstm = nullptr;
  
  if(oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP
      || oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP){
    if(!left->ty_->IsSameType(type::IntTy::Instance())){
      // errormsg->Error(left_->pos_, "integer required");
    }
    if(!right->ty_->IsSameType(type::IntTy::Instance())){
      // errormsg->Error(right_->pos_, "integer required");
    }
    tree::Exp *l = left->exp_->UnEx();
    tree::Exp *r = right->exp_->UnEx();
    switch (oper_)
    {
    case absyn::PLUS_OP:
      //errormsg->Error(right_->pos_, "plus");
      exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::PLUS_OP, l, r));
      break;
    case absyn::MINUS_OP:
      //errormsg->Error(right_->pos_, "minus");
      exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::MINUS_OP, l, r));
      break;
    case absyn::TIMES_OP:
      //errormsg->Error(right_->pos_, "times");
      exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::MUL_OP, l, r));
      break;
    case absyn::DIVIDE_OP:
      //errormsg->Error(right_->pos_, "divide");
      exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::DIV_OP, l, r));
      break;
    default:
      break;
    }
  }
  else{
    if(oper_ == absyn::GE_OP || oper_ == absyn::GT_OP
      || oper_ == absyn::LE_OP || oper_ == absyn::LT_OP){
      switch (oper_)
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
      temp::Label **trues = &(cjstm->true_label_);
      temp::Label **falses = &(cjstm->false_label_);
      exp = new tr::CxExp(trues, falses, cjstm);
    }
    else{
      if(oper_ == absyn::EQ_OP || oper_ == absyn::NEQ_OP){
        if(typeid(*(left->ty_)) == typeid(type::IntTy)){
          switch (oper_)
          {
          case EQ_OP:
            //errormsg->Error(pos_, "at 1");
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
          if(typeid(*(left->ty_)) == typeid(type::StringTy)){
            //errormsg->Error(pos_, "is string");
            std::initializer_list<tree::Exp *> list = {left->exp_->UnEx(), right->exp_->UnEx()};
            tree::ExpList *expList = new tree::ExpList(list);
            cjstm = new tree::CjumpStm(tree::EQ_OP, 
              new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel("string_equal")), expList),
                new tree::ConstExp(1), nullptr, nullptr);
          }
          else{
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
        }
        temp::Label **trues = &(cjstm->true_label_);
        temp::Label **falses = &(cjstm->false_label_);
        exp = new tr::CxExp(trues, falses, cjstm);
      }
    }
  }
  return new tr::ExpAndTy(exp, type::IntTy::Instance());
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin record exp %s", typ_->Name().data());
  type::Ty *typ_ty = tenv->Look(typ_)->ActualTy();
  if(!typ_ty){
    //errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
  }
  tree::ExpList *expList = new tree::ExpList();
  std::list<type::Field *> fieldsList = ((type::RecordTy *)typ_ty)->fields_->GetList();
  std::list<absyn::EField *> eFieldList = fields_->GetList();
  std::list<type::Field *>::iterator field_it = fieldsList.begin();
  std::list<absyn::EField *>::iterator eField_it = eFieldList.begin();
  for(; field_it != fieldsList.end() && eField_it != eFieldList.end(); field_it++){
    tr::ExpAndTy *tmp = (*eField_it)->exp_->Translate(venv, tenv, level, label, errormsg);
    expList->Append(tmp->exp_->UnEx());
    eField_it++;
  }
  temp::Temp *reg = temp::TempFactory::NewTemp();
  int size = expList->GetList().size();
  tree::ExpList *exps = new tree::ExpList();
  exps->Append(new tree::ConstExp(size * reg_manager->WordSize()));
  tree::Stm *stm = new tree::MoveStm(new tree::TempExp(reg), 
    new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel("alloc_record")), exps));
  int count = 0;
  for(auto exp : expList->GetList()){
    stm = new tree::SeqStm(stm, new tree::MoveStm(
      new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, new tree::TempExp(reg), 
        new tree::ConstExp(count * reg_manager->WordSize()))), exp));
    count++;
  }

  return new tr::ExpAndTy(new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(reg))), typ_ty->ActualTy());
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  ExpList *seq = seq_;
  if(!seq){
    //errormsg->Error(pos_, "not a SeqExp");
  }
  std::list<Exp *> exp_list = seq->GetList();
  std::list<Exp *>::iterator it = exp_list.begin();
  tr::Exp *exp = nullptr;
  type::Ty *ty;
  while(it != exp_list.end()){
    tr::ExpAndTy *tmp = (*it)->Translate(venv, tenv, level, label, errormsg);
    if(!exp){
      exp = new tr::ExExp(tmp->exp_->UnEx());
    }
    else{
      exp = new tr::ExExp(new tree::EseqExp(exp->UnNx(), tmp->exp_->UnEx()));
    }
    ty = tmp->ty_->ActualTy();
    it++;
  }

  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp = exp_->Translate(venv, tenv, level, label,errormsg);
  if(typeid(*var_) == typeid(absyn::SimpleVar)){
    env::EnvEntry *entry = venv->Look(((SimpleVar *)var_)->sym_);
    if(entry->readonly_){
      //errormsg->Error(pos_, "loop variable can't be assigned");
    }
  }
  if(!var->ty_->IsSameType(exp->ty_)){
    //errormsg->Error(pos_, "unmatched assign exp");
  }
  tr::Exp *assignExp = new tr::NxExp(new tree::MoveStm(var->exp_->UnEx(), exp->exp_->UnEx()));

  return new tr::ExpAndTy(assignExp, type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *then = then_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp;
  if(typeid(test->ty_) != typeid(type::IntTy)){
    //errormsg->Error(pos_, "integer required");
  }
  tr::Cx cx = test->exp_->UnCx(errormsg);
  temp::Label *trues = temp::LabelFactory::NewLabel();
  temp::Label *falses = temp::LabelFactory::NewLabel();
  temp::Label *meets = temp::LabelFactory().NewLabel();
  temp::Temp *r = temp::TempFactory().NewTemp();
  *(cx.trues_) = trues;
  *(cx.falses_) = falses;

  if(elsee_){
    tr::ExpAndTy *elsee = elsee_->Translate(venv, tenv, level, label, errormsg);
    if(elsee->ty_->IsSameType(then->ty_)){

      std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>;
      jumps->push_back(meets);
      exp = new tr::ExExp(new tree::EseqExp(cx.stm_, 
        new tree::EseqExp(new tree::LabelStm(trues),
          new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), then->exp_->UnEx()), 
            new tree::EseqExp(new tree::JumpStm(new tree::NameExp(meets), jumps), 
              new tree::EseqExp(new tree::LabelStm(falses), 
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), elsee->exp_->UnEx()), 
                  new tree::EseqExp(new tree::JumpStm(new tree::NameExp(meets), jumps),
                    new tree::EseqExp(new tree::LabelStm(meets), new tree::TempExp(r))))))))));
    }
    else{
      //errormsg->Error(pos_, "then exp and else exp type mismatch");
    }
  }
  else{
    if(typeid(then->ty_) != typeid(type::VoidTy)){
      //errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
    }
    exp = new tr::NxExp(new tree::SeqStm(cx.stm_, 
      new tree::SeqStm(new tree::LabelStm(trues), 
        new tree::SeqStm(then->exp_->UnNx(), new tree::LabelStm(falses)))));
  }

  return new tr::ExpAndTy(exp, then->ty_->ActualTy());
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* test = test_->Translate(venv, tenv, level, label, errormsg);
  temp::Label *l_done = temp::LabelFactory::NewLabel();
  tr::ExpAndTy* body = body_->Translate(venv, tenv, level, l_done, errormsg);
  temp::Label* l_test = temp::LabelFactory::NewLabel();
  temp::Label* l_body = temp::LabelFactory::NewLabel();
  tr::Cx cx = test->exp_->UnCx(errormsg);
  *(cx.trues_) = l_body; 
  *(cx.falses_) = l_done; 
  std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>;
  jumps->push_back(l_test); 

  if(typeid(test->ty_) != typeid(type::IntTy)){
    //errormsg->Error(test_->pos_, "should be integer exp!");
  }
  if(typeid(body->ty_) != typeid(type::VoidTy)){
    //errormsg->Error(body_->pos_, "while body must produce no value");
    // return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }

  tr::Exp *exp= new tr::NxExp(
    new tree::SeqStm(new tree::LabelStm(l_test),
      new tree::SeqStm(cx.stm_,
        new tree::SeqStm(new tree::LabelStm(l_body),
          new tree::SeqStm(body->exp_->UnNx(),
            new tree::SeqStm(new tree::JumpStm(new tree::NameExp(l_test), jumps),
              new tree::LabelStm(l_done)))))));

  return new tr::ExpAndTy(exp,type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *lo = lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *hi = hi_->Translate(venv, tenv, level, label, errormsg);
  //errormsg->Error(hi_->pos_, "end high");
  tr::Exp *exp;
  if(!lo->ty_->IsSameType(type::IntTy::Instance())){
    // errormsg->Error(lo_->pos_, "lo for exp's range type is not integer");
  }
  if(!hi->ty_->IsSameType(type::IntTy::Instance())){
    // errormsg->Error(hi_->pos_, "hi for exp's range type is not integer");
  }
  tr::Access *access = tr::Access::allocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(access, lo->ty_, true));
  temp::Label *l_done = temp::LabelFactory().NewLabel();
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, l_done, errormsg);
  tree::Exp *frameExp = access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));

  temp::Label *l_test = temp::LabelFactory().NewLabel();
  temp::Label *l_body = temp::LabelFactory().NewLabel();
  std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>;
  jumps->push_back(l_test);

  exp = new tr::NxExp(new tree::SeqStm(new tree::MoveStm(frameExp, lo->exp_->UnEx()), 
    new tree::SeqStm(new tree::LabelStm(l_test),
      new tree::SeqStm(new tree::CjumpStm(tree::LE_OP, frameExp, hi->exp_->UnEx(), l_body, l_done),
        new tree::SeqStm(new tree::LabelStm(l_body),
          new tree::SeqStm(body->exp_->UnNx(),
                new tree::SeqStm(new tree::MoveStm(frameExp, new tree::BinopExp(tree::BinOp::PLUS_OP, frameExp, new tree::ConstExp(1))),
                  new tree::SeqStm(new tree::JumpStm(new tree::NameExp(l_test), jumps),
                    new tree::LabelStm(l_done)))))))));
    

  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>;
  jumps->push_back(label);
  tr::Exp *exp = new tr::NxExp(new tree::JumpStm(new tree::NameExp(label), jumps));

  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
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
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, label, errormsg);
  if(!stm){
    exp = body->exp_->UnEx();
  }
  else{
    exp = new tree::EseqExp(stm, body->exp_->UnEx());
  }

  return new tr::ExpAndTy(new tr::ExExp(exp), body->ty_);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *typ_ty = tenv->Look(typ_)->ActualTy();
  tr::ExpAndTy *size = size_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *init = init_->Translate(venv, tenv, level, label, errormsg);
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
  std::list<FunDec *> functions = functions_->GetList();
  for(FunDec *function : functions){
    //errormsg->Error(pos_, "do first");
    type::TyList *formals = function->params_->MakeFormalTyList(tenv, errormsg);
    std::list<bool> formal_escape;
    std::list<Field *> fieldList = function->params_->GetList();
    for(Field * param : fieldList){
      formal_escape.push_back(param->escape_);
    }
    tr::Level *new_level = tr::newLevel(level, function->name_, formal_escape);
    if(function->result_){
      type::Ty *result_ty = tenv->Look(function->result_);
      printf("enter the function %s\n", function->name_->Name().data());
      venv->Enter(function->name_, new env::FunEntry(new_level, function->name_, formals, result_ty));
    }
    else{
      printf("enter the function %s\n", function->name_->Name().data());
      venv->Enter(function->name_, new env::FunEntry(new_level, function->name_, formals, type::VoidTy::Instance()));
    }
  }

  for(FunDec *function : functions){
    env::EnvEntry *fun = venv->Look(function->name_);
    type::TyList *formals = ((env::FunEntry*)fun)->formals_;
    auto formal_it = formals->GetList().begin();
    auto param_it = (function->params_->GetList()).begin();
    venv->BeginScope();
    std::list<frame::Access*> accessList = ((env::FunEntry*)fun)->level_->frame_->formals_;
    std::list<frame::Access*>::iterator access_it = accessList.begin();
    // Be careful with this!!!!! Skip the staticlink!
    access_it++;
    for(; param_it != (function->params_->GetList()).end(); param_it++){
      venv->Enter((*param_it)->name_, 
        new env::VarEntry(new tr::Access(((env::FunEntry*)fun)->level_, *access_it), *formal_it));
      formal_it++;
      access_it++;
    }
    tr::ExpAndTy *body = function->body_->Translate(venv, tenv, ((env::FunEntry*)fun)->level_, ((env::FunEntry*)fun)->label_, errormsg);

    if(((env::FunEntry *)fun)->result_->ActualTy()->IsSameType(type::VoidTy::Instance())){
      if(!body->ty_->IsSameType(type::VoidTy::Instance())){
        //errormsg->Error(pos_, "procedure returns value");
      }
      else{
        //errormsg->Error(pos_, "did right procedure returns value");
      }
    }
    else{
      //errormsg->Error(pos_, "should returns value");
    }
    venv->EndScope();
    tree::Stm *stm = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), body->exp_->UnEx());
    stm = frame::procEntryExit1(((env::FunEntry *)fun)->level_->frame_, stm);
    frags->PushBack(new frame::ProcFrag(stm, ((env::FunEntry *)fun)->level_->frame_));
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *init = init_->Translate(venv, tenv, level, label, errormsg);
  if(typeid(*(init->ty_)) == typeid(type::RecordTy)){
    //errormsg->Error(pos_, "var record@!");
  }
  type::Ty *typ_ty;
  if(!typ_){
    // errormsg->Error(pos_, "!typ_");
    if(typeid(init->ty_) == typeid(type::NilTy)){
      //errormsg->Error(pos_, "init should not be nil without type specified");
      return nullptr;
    }
    //errormsg->Error(pos_, "do enter");
    typ_ty = init->ty_->ActualTy();
  }
  else{
    typ_ty = tenv->Look(typ_);
    if(!typ_ty){
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return nullptr;
    }
    if(!typ_ty->IsSameType(init->ty_)){
      if(typeid(*typ_ty) == typeid(type::RecordTy) && typeid(init->ty_) == typeid(type::NilTy)){
        // venv->Enter(var_, new env::VarEntry(typ_ty));
      }
      else{
        //errormsg->Error(pos_, "type mismatch");
        return nullptr;
      }
    }
    else{
      // venv->Enter(var_, new env::VarEntry(typ_ty));
    }
  }
  tr::Access *access = tr::Access::allocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(access, typ_ty));
  tree::Exp *dst = access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  tree::Exp *src = init->exp_->UnEx();

  return new tr::NxExp(new tree::MoveStm(dst, src));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<NameAndTy*> nameAndTyList = types_->GetList();
  /* At the first time, parse the header and write it into tenv */
  for(auto &nameAndTy : nameAndTyList){
    int count = 0;
    for(auto &tmp : nameAndTyList){
      if(nameAndTy->name_ == tmp->name_){
        count++;
      }
      if(count >= 2){
        //errormsg->Error(pos_, "two types have the same name");
        return nullptr;
      }
    }
    tenv->Enter(nameAndTy->name_, new type::NameTy(nameAndTy->name_, nullptr));
  }
  /* At the second time, parse the body with the tenv */
  for(auto &nameAndTy : nameAndTyList){
    type::NameTy *type = static_cast<type::NameTy*>(tenv->Look(nameAndTy->name_));
    type->ty_ = nameAndTy->ty_->Translate(tenv, errormsg);
    if(typeid(*(type->ty_)) == typeid(type::RecordTy)){
      //errormsg->Error(pos_, "woc is record");
    }
    tenv->Set(nameAndTy->name_, type);
  }
  for(auto &nameAndTy : nameAndTyList){
    type::Ty *name_ty = tenv->Look(nameAndTy->name_);
    type::Ty *tmp = name_ty;
    while(typeid(*tmp) == typeid(type::NameTy)){
      tmp = ((type::NameTy *)tmp)->ty_;
      if(((type::NameTy *)tmp) == ((type::NameTy *)name_ty)){
        //errormsg->Error(pos_, "illegal type cycle");
        return nullptr;
      }
    }
  }
  //errormsg->Error(pos_, "end typedec");

  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin namety");
  type::Ty *name_ty = tenv->Look(name_);
  if(!name_ty){
    //errormsg->Error(pos_, "should be defined type");
    return type::VoidTy::Instance();
  }
  return new type::NameTy(name_, name_ty);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::FieldList *fields = record_->MakeFieldList(tenv, errormsg);
  
  return new type::RecordTy(fields);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *array_ty = tenv->Look(array_);
  if(!array_ty){
    //errormsg->Error(pos_, "should be defined type!");
    return type::VoidTy::Instance();
  }
  return new type::ArrayTy(array_ty);
}

} // namespace absyn

