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
    // errormsg->Error(1, "www");
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    tree::CjumpStm *stm = new tree::CjumpStm(tree::NE_OP, exp_, new tree::ConstExp(0), t, f);
    printf("ExExp UnCx");
    return Cx(&t, &f, stm);
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
    printf("NxExp UnEx");
    if(!f){
      printf("\nnullptr 1\n");
    }
    if(!t){
      printf("\nnullptr 2\n");
    }
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
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override { 
    /* TODO: Put your lab5 code here */
    // errormsg->Error(1, "wwwaaaaaaaw");
    printf("CxExp UnCx");
    return cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  temp::Label *main_label_ = temp::LabelFactory::NamedLabel("tigermain");
  std::list<bool> escapes;
  frame::Frame *newFrame = frame::NewFrame(main_label_ , escapes);
  printf("get the offset %d", newFrame->offset);
  printf("\nbefore name is %s\n", newFrame->name->Name().data());
  main_level_ = std::make_unique<Level>(newFrame, nullptr);
  printf("\nname is %s\n", newFrame->name->Name().data());
  tenv_ = std::make_unique<env::TEnv>();
  venv_ = std::make_unique<env::VEnv>();
  // fill after main level init!
  FillBaseTEnv();
  FillBaseVEnv();
  tr::ExpAndTy *tree = absyn_tree_.get()->Translate(venv_.get(), tenv_.get(), main_level_.get(), main_label_, errormsg_.get());
  tree::Stm *tmp = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), tree->exp_->UnEx());
  tree::Stm *wrap = frame::procEntryExit1(newFrame, tmp);
  
  frags->PushBack(new frame::ProcFrag(wrap, newFrame));
}

tr::Level *newLevel(Level *parent, temp::Label *name, std::list<bool> formals){
  formals.push_front(true);
  frame::Frame *newFrame = frame::NewFrame(name, formals);
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
    tr::Level *tmp = level;
    tree::Exp *exp = new tree::TempExp(reg_manager->FramePointer());
    while(tmp != ventry->access_->level_){
      exp = new tree::MemExp(new tree::BinopExp(tree::BinOp::MINUS_OP, exp, new tree::ConstExp(reg_manager->WordSize())));
      tmp = tmp->parent_;
    }
    
    // errormsg->Error(pos_, "end simple var");

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
  //errormsg->Error(pos_, "begin fieldvar %s", sym_->Name().data());
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *ty = var->ty_->ActualTy();
  if(!ty || typeid(*ty) != typeid(type::RecordTy)){
    //errormsg->Error(pos_, "not a record type");
    // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  type::FieldList *fields = (static_cast<type::RecordTy *>(ty))->fields_;
  std::list<type::Field *> filed_list = fields->GetList();
  std::list<type::Field *>::iterator tmp = filed_list.begin();
  int offset = 0;
  while(tmp != filed_list.end()){
    //errormsg->Error(pos_, "parse a field %s", (*tmp)->name_->Name().data());
    if((*tmp)->name_ == sym_){
      tr::Exp *filedVar = new tr::ExExp(new tree::MemExp(
        new tree::BinopExp(tree::PLUS_OP, var->exp_->UnEx(), new tree::ConstExp(offset * reg_manager->WordSize()))));
      return new tr::ExpAndTy(filedVar, (*tmp)->ty_);
    }
    if(!var->exp_->UnEx()){
      printf("woccc55555");
      assert(var->exp_->UnEx());
    }
    tmp++;
    offset++;
  }
  //errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // errormsg->Error(pos_, "begin SubscriptVar");
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  // errormsg->Error(pos_, "begin Subscrip 2 ");
  tr::ExpAndTy *subscript = subscript_->Translate(venv, tenv, level, label, errormsg);
  if(typeid(var->ty_) != typeid(type::ArrayTy)){
    //errormsg->Error(pos_, "array type required");
    // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  if(typeid(((type::ArrayTy*)var->ty_)->ty_->ActualTy()) != typeid(type::RecordTy)){
    //errormsg->Error(pos_, "not a record");
  }
  tr::Exp *subscriptVar = new tr::ExExp(
    new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, var->exp_->UnEx(), 
      new tree::BinopExp(tree::MUL_OP, subscript->exp_->UnEx(), new tree::ConstExp(reg_manager->WordSize())))));
    if(!var->exp_->UnEx()){
      printf("woccc55555  var->exp_->UnEx()");
      assert(var->exp_->UnEx());
    }
    if(!subscript->exp_->UnEx()){
      printf("\nwoccc55555  subscript->exp_->UnEx()\n");
      assert(subscript->exp_->UnEx());
    }
  return new tr::ExpAndTy(subscriptVar, ((type::ArrayTy*)var->ty_)->ty_->ActualTy());
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // errormsg->Error(pos_, "begin VarExp");
  if(typeid(*var_) == typeid(SimpleVar)){
    // errormsg->Error(pos_, "it is a SimpleVar");
    return ((SimpleVar *)(var_))->Translate(venv, tenv, level, label, errormsg);
  }
  if(typeid(*var_) == typeid(FieldVar)){
    // errormsg->Error(pos_, "it is a FieldVar");
    return ((FieldVar *)(var_))->Translate(venv, tenv, level, label,  errormsg);
  }
  if(typeid(*var_) == typeid(SubscriptVar)){
    // errormsg->Error(pos_, "it is a SubscriptVar");
    return ((SubscriptVar *)(var_))->Translate(venv, tenv, level, label, errormsg);
  }
  // errormsg->Error(pos_, "it is not a VarExp");

  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin nil exp");
  // TODO: I am not sure about the nullptr
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
  printf("begin string exp %s\n", str_.data());
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(temp::LabelFactory::NamedLabel(str_))), type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin CallExp %s", func_->Name().data());
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
      // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    if(!formal->IsSameType((*args_it)->Translate(venv, tenv, level, label, errormsg)->ty_)){
      //errormsg->Error((*args_it)->pos_, "para type mismatch");
      // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    //errormsg->Error((*args_it)->pos_, "begin a formal");
    expList->Append((*args_it)->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx());
    //errormsg->Error((*args_it)->pos_, "end a formal");
    args_it++;
  }
  //errormsg->Error(pos_, "end parse formal");
  if(args_it != args.end()){
    //errormsg->Error(pos_, "too many params in function %s", func_->Name().data());
  }
  // it is the outermost
  if(entry->level_->parent_ == nullptr){
    printf("\nouter most  %s\n", func_->Name().data());
    exp = new tr::ExExp(level->frame_->externalCall(func_->Name(), expList));
    assert(exp);
  }
  else{
    tree::Exp *sl = new tree::TempExp(reg_manager->FramePointer());
    tr::Level *tmp_level = level;
    while(tmp_level != entry->level_ && !tmp_level){
      sl = new tree::MemExp(new tree::BinopExp(tree::MINUS_OP, sl, new tree::ConstExp(reg_manager->WordSize())));
      tmp_level = tmp_level->parent_;
    }
    expList->Insert(sl);
    exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(entry->label_), expList));
  }
  //errormsg->Error(pos_, "end CallExp");

  return new tr::ExpAndTy(exp, result_ty);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // TODO:nothing ok
  // errormsg->Error(pos_, "begin opexp %d", oper_);
  tr::ExpAndTy *left = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *right = right_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp = nullptr;
  tree::CjumpStm *cjstm = nullptr;
  
  if(oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP
      || oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP){
    if(!left->ty_->IsSameType(type::IntTy::Instance())){
      errormsg->Error(left_->pos_, "integer required");
    }
    if(!right->ty_->IsSameType(type::IntTy::Instance())){
      errormsg->Error(right_->pos_, "integer required");
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
    }if(!l){
      printf("woccc55555  l");
      assert(l);
    }
  }
  else{
    if(oper_ == absyn::GE_OP || oper_ == absyn::GT_OP
      || oper_ == absyn::LE_OP || oper_ == absyn::LT_OP){
      if(typeid(left->ty_) != typeid(type::IntTy) || typeid(left->ty_) != typeid(type::StringTy)){
        //errormsg->Error(left_->pos_, "integer or string required");
      }
      if(typeid(right->ty_) != typeid(type::IntTy) || typeid(right->ty_) != typeid(type::StringTy)){
        //errormsg->Error(right_->pos_, "integer or string required");
      }
      if(!(left->ty_)->IsSameType(right->ty_)){
        //errormsg->Error(pos_, "same type required");
      }
      //where is patchList??? Is this OK?
      temp::Label *trues = temp::LabelFactory::NewLabel();
      temp::Label *falses = temp::LabelFactory::NewLabel();
      switch ((oper_))
      {
      case absyn::GE_OP:
        cjstm = new tree::CjumpStm(tree::RelOp::GE_OP, left->exp_->UnEx(), right->exp_->UnEx(), trues, falses);
        break;
      case absyn::GT_OP:
        cjstm = new tree::CjumpStm(tree::RelOp::GT_OP, left->exp_->UnEx(), right->exp_->UnEx(), trues, falses);
        break;
      case absyn::LE_OP:
        cjstm = new tree::CjumpStm(tree::RelOp::LE_OP, left->exp_->UnEx(), right->exp_->UnEx(), trues, falses);
        break;
      case absyn::LT_OP:
        cjstm = new tree::CjumpStm(tree::RelOp::LT_OP, left->exp_->UnEx(), right->exp_->UnEx(), trues, falses);
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
          //errormsg->Error(left_->pos_, "integer or string or array or record required");
          // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
        }
        if(typeid(right->ty_) != typeid(type::IntTy) || typeid(right->ty_) != typeid(type::StringTy)
        || typeid(right->ty_) != typeid(type::ArrayTy) || typeid(right->ty_) != typeid(type::RecordTy)){
          //errormsg->Error(right_->pos_, "integer or string or array or record required");
          // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
        }
        if(!(left->ty_)->IsSameType((right->ty_))){
          if(typeid(left->ty_) == typeid(type::RecordTy) && typeid(right->ty_) == typeid(type::NilTy)){
            // do nothing?
          }
          else{
            //errormsg->Error(pos_, "same type required");
            // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
          }
        }
        if(typeid(left->ty_) == typeid(type::IntTy)){
          temp::Label *trues = temp::LabelFactory::NewLabel();
          temp::Label *falses = temp::LabelFactory::NewLabel();
          switch (oper_)
          {
          case EQ_OP:
            //errormsg->Error(pos_, "at 1");
            cjstm = new tree::CjumpStm(tree::RelOp::EQ_OP, left->exp_->UnEx(), right->exp_->UnEx(), trues, falses);
            break;
          case NEQ_OP:
            cjstm = new tree::CjumpStm(tree::RelOp::NE_OP, left->exp_->UnEx(), right->exp_->UnEx(), trues, falses);
            break;
          default:
            break;
          }
        }
        else{
          if(typeid(left->ty_) == typeid(type::StringTy)){
            //errormsg->Error(pos_, "is string");
            std::initializer_list<tree::Exp *> list = {left->exp_->UnEx(), right->exp_->UnEx()};
            tree::ExpList *expList = new tree::ExpList(list);

            exp = new tr::ExExp(
              new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel("string_equal")), expList));
          }
          else{
            if(typeid(left->ty_) == typeid(type::ArrayTy)){
              //errormsg->Error(pos_, "do cmp the array");
            }
            else{
              if(typeid(left->ty_) == typeid(type::RecordTy)){
                //errormsg->Error(pos_, "do cmp the record");
              }
              else{
                //errormsg->Error(pos_, "is nothing");
                temp::Label *trues = temp::LabelFactory::NewLabel();
                temp::Label *falses = temp::LabelFactory::NewLabel();
                switch (oper_)
                {
                case EQ_OP:
                  cjstm = new tree::CjumpStm(tree::RelOp::EQ_OP, left->exp_->UnEx(), right->exp_->UnEx(), trues, falses);
                  break;
                case NEQ_OP:
                  cjstm = new tree::CjumpStm(tree::RelOp::NE_OP, left->exp_->UnEx(), right->exp_->UnEx(), trues, falses);
                  break;
                default:
                  break;
                }
              }
            }
          }
          
          exp = new tr::CxExp(&(cjstm->true_label_), &(cjstm->false_label_), cjstm);
        }
      }
    }
  }
  //errormsg->Error(pos_, "end opexp");

  return new tr::ExpAndTy(exp, type::IntTy::Instance());
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin record exp %s", typ_->Name().data());
  type::Ty *typ_ty = tenv->Look(typ_);
  if(!typ_ty){
    //errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
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
  
  return new tr::ExpAndTy(recordExp, typ_ty->ActualTy());
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin seqexp");
  ExpList *seq = seq_;
  if(!seq){
    //errormsg->Error(pos_, "not a SeqExp");
    // return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  std::list<Exp *> exp_list = seq->GetList();
  std::list<Exp *>::iterator it = exp_list.begin();
  tr::Exp *exp = nullptr;
  type::Ty *ty;
  while(it != exp_list.end()){
    //errormsg->Error((*it)->pos_, "do translate");
    tr::ExpAndTy *tmp = (*it)->Translate(venv, tenv, level, label, errormsg);
    //errormsg->Error((*it)->pos_, "end translate");
    if(!exp){
      exp = new tr::ExExp(tmp->exp_->UnEx());
    }
    else{
      exp = new tr::ExExp(new tree::EseqExp(exp->UnNx(), tmp->exp_->UnEx()));
    }
    //errormsg->Error((*it)->pos_, "end parse exp");
    ty = tmp->ty_->ActualTy();
    it++;
  }
  //errormsg->Error(pos_, "end seqexp");

  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin assignexp");
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp = exp_->Translate(venv, tenv, level, label,errormsg);
  //errormsg->Error(pos_, "end parse exp");
  if(typeid(*var_) == typeid(absyn::SimpleVar)){
    env::EnvEntry *entry = venv->Look(((SimpleVar *)var_)->sym_);
    if(entry->readonly_){
      //errormsg->Error(pos_, "loop variable can't be assigned");
      // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
  }
  if(!var->ty_->IsSameType(exp->ty_)){
    //errormsg->Error(pos_, "unmatched assign exp");
  }
  //errormsg->Error(pos_, "gegin parse as");
  tr::Exp *assignExp = new tr::NxExp(new tree::MoveStm(var->exp_->UnEx(), exp->exp_->UnEx()));
  //errormsg->Error(pos_, "end assignexp");

  return new tr::ExpAndTy(assignExp, type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin ifexp");
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *then = then_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp;
  if(typeid(test->ty_) != typeid(type::IntTy)){
    //errormsg->Error(pos_, "integer required");
    // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  if(elsee_){
    //errormsg->Error(pos_, "begin else");
    tr::ExpAndTy *elsee = elsee_->Translate(venv, tenv, level, label, errormsg);
    //errormsg->Error(pos_, "end else translate");
    if(elsee->ty_->IsSameType(then->ty_)){
      tr::Cx cx = test->exp_->UnCx(errormsg);
      //errormsg->Error(pos_, "after cx");
      temp::Label *trues = temp::LabelFactory::NewLabel();
      *(test->exp_->UnCx(errormsg).trues_) = trues;

      temp::Label *falses = temp::LabelFactory::NewLabel();
      *(test->exp_->UnCx(errormsg).falses_) = falses;

      temp::Label *meets = temp::LabelFactory().NewLabel();
      temp::Temp *r = temp::TempFactory().NewTemp();
      std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>;
      jumps->push_back(meets);

    if(!trues){
      printf("\nnullptr 3\n");
    }
    if(!falses){
      printf("\nnullptr 4\n");
    }
    if(!meets){
      printf("\nnullptr 5\n");
    }
      exp = new tr::ExExp(new tree::EseqExp(cx.stm_, 
        new tree::EseqExp(new tree::LabelStm(trues),
          new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), then->exp_->UnEx()), 
            new tree::EseqExp(new tree::JumpStm(new tree::NameExp(meets), jumps), 
              new tree::EseqExp(new tree::LabelStm(falses), 
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), elsee->exp_->UnEx()), 
                  new tree::EseqExp(new tree::JumpStm(new tree::NameExp(meets), jumps),
                    new tree::EseqExp(new tree::LabelStm(meets), new tree::TempExp(r))))))))));
      //errormsg->Error(pos_, "end parse else");
    }
    else{
      //errormsg->Error(pos_, "then exp and else exp type mismatch");
      // return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
    }
  }
  else{
    //errormsg->Error(pos_, "no else");
    if(typeid(then->ty_) != typeid(type::VoidTy)){
      //errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
      // return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    temp::Label *trues = temp::LabelFactory().NewLabel();
    temp::Label *falses = temp::LabelFactory().NewLabel();
    if(!trues){
      printf("\nnullptr 6\n");
    }
    if(!falses){
      printf("\nnullptr 7\n");
    }
    exp = new tr::NxExp(new tree::SeqStm(test->exp_->UnCx(errormsg).stm_, 
      new tree::SeqStm(new tree::LabelStm(trues), 
        new tree::SeqStm(then->exp_->UnNx(), new tree::LabelStm(falses)))));
  }
  //errormsg->Error(pos_, "end ifexp");

  return new tr::ExpAndTy(exp, then->ty_->ActualTy());
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin whileexp");
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp;
  temp::Label *l_test = temp::LabelFactory().NewLabel();
  temp::Label *l_body = temp::LabelFactory().NewLabel();
  temp::Label *l_done = temp::LabelFactory().NewLabel();
  std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>;
  jumps->push_back(l_test);

  if(typeid(test->ty_) != typeid(type::IntTy)){
    //errormsg->Error(test_->pos_, "should be integer exp!");
    // return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  if(typeid(body->ty_) != typeid(type::VoidTy)){
    //errormsg->Error(body_->pos_, "while body must produce no value");
    // return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
    if(!l_test){
      printf("\nnullptr 8\n");
    }
    if(!l_body){
      printf("\nnullptr 9\n");
    }
    if(!l_done){
      printf("\nnullptr 10\n");
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
  //errormsg->Error(pos_, "begin forexp");
  tr::ExpAndTy *lo = lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *hi = hi_->Translate(venv, tenv, level, label, errormsg);
  //errormsg->Error(hi_->pos_, "end high");
  temp::Label *l_test_end = temp::LabelFactory().NewLabel();
  temp::Label *l_body = temp::LabelFactory().NewLabel();
  temp::Label *l_done = temp::LabelFactory().NewLabel();
  std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>;
  //errormsg->Error(hi_->pos_, "do push");
  jumps->push_back(l_test_end);
  //errormsg->Error(hi_->pos_, "end push");
  tr::Exp *exp;
  if(!lo->ty_->IsSameType(type::IntTy::Instance())){
    errormsg->Error(lo_->pos_, "lo for exp's range type is not integer");
    // return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  if(!hi->ty_->IsSameType(type::IntTy::Instance())){
    errormsg->Error(hi_->pos_, "hi for exp's range type is not integer");
    // return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  //errormsg->Error(hi_->pos_, "do alloc");
  tr::Access *access = tr::Access::AllocLocal(level, escape_);
  //errormsg->Error(hi_->pos_, "end alloc");
  venv->Enter(var_, new env::VarEntry(access, lo->ty_, true));
  //errormsg->Error(hi_->pos_, "end enter");
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, label, errormsg);
  //errormsg->Error(hi_->pos_, "end body");
  tree::Exp *frameExp = access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  if(!frameExp){
      printf("woccc55555  frameExp");
      assert(frameExp);
    }
    if(!l_body){
      printf("\nnullptr 1\n");
    }
    if(!l_test_end){
      printf("\nnullptr 12\n");
    }
    if(!l_done){
      printf("\nnullptr 13\n");
    }
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
  //errormsg->Error(pos_, "begin break exp");
  std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>;
  jumps->push_back(label);
  tr::Exp *exp = new tr::NxExp(new tree::JumpStm(new tree::NameExp(label), jumps));

  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin let exp");
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

  //errormsg->Error(pos_, "parse body");
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, label, errormsg);
  if(!stm){
    exp = body->exp_->UnEx();
  }
  else{
    exp = new tree::EseqExp(stm, body->exp_->UnEx());
  }
  //errormsg->Error(pos_, "end LetExp");

  return new tr::ExpAndTy(new tr::ExExp(exp), body->ty_);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin arrayexp  %s", typ_->Name().data());
  type::Ty *typ_ty = tenv->Look(typ_)->ActualTy();
  tr::ExpAndTy *size = size_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *init = init_->Translate(venv, tenv, level, label, errormsg);
  if(!typ_ty){
    //errormsg->Error(pos_, "%s should be a defined type", typ_->Name().data());
  }
  if(typeid(*typ_ty) != typeid(type::ArrayTy)){
    //errormsg->Error(pos_, "%s should be an array type", typ_->Name().data());
  }
  if(!size->ty_->IsSameType(type::IntTy::Instance())){
    //errormsg->Error(size_->pos_, "should be an integer");
  }
  if(!((type::ArrayTy *)typ_ty)->ty_->IsSameType(init->ty_)){
    //errormsg->Error(pos_, "type mismatch");
  }

  if(typeid(*(init->ty_)) == typeid(type::RecordTy)){
    //errormsg->Error(pos_, "record@!");
  }
  //errormsg->Error(pos_, "end arrayexp");
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
  //errormsg->Error(pos_, "begin voidexp");
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // errormsg->Error(pos_, "begin FunctionDec");
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
    if(!(new_level->frame_->stms_)){
      printf("\nnullptr afer new level!\n");
    }
    else{
      printf("\nOK afer new level!\n");
    }
    if(function->result_){
      type::Ty *result_ty = tenv->Look(function->result_);
      printf("enter the function %s\n", function->name_->Name().data());
      venv->Enter(function->name_, new env::FunEntry(new_level, function->name_, formals, result_ty));
      env::FunEntry* f = (env::FunEntry*)(venv->Look(function->name_));
      if(f){
        printf("we now find \n");
        printf("we now find %s\n", f->label_->Name().data());
      }
      else{
        printf("??? \n");
      }
    }
    else{
      printf("enter the function %s\n", function->name_->Name().data());
      venv->Enter(function->name_, new env::FunEntry(new_level, function->name_, formals, type::VoidTy::Instance()));
      env::FunEntry* f = (env::FunEntry*)(venv->Look(function->name_));
      if(f){
        printf("we now find \n");
        printf("we now find %s\n", f->label_->Name().data());
      }
      else{
        printf("??? \n");
      }
    }
  }
  //errormsg->Error(pos_, "finish first");

  for(FunDec *function : functions){
    int count = 0;
    //errormsg->Error(pos_, "do second");
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
      //errormsg->Error(pos_, "now push in %s", (*param_it)->name_->Name().data());
      access++;
    }
    // errormsg->Error(pos_, "parse body of %s", function->name_->Name().data());
    tr::ExpAndTy *body = function->body_->Translate(venv, tenv, ((env::FunEntry*)fun)->level_, label, errormsg);

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
    frags->PushBack(new frame::ProcFrag(body->exp_->UnNx(), ((env::FunEntry *)fun)->level_->frame_));
  }
  //errormsg->Error(pos_, "end FunctionDec");

  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // errormsg->Error(pos_, "begin vardec");
    // errormsg->Error(pos_, "var is %s", (var_->Name()).data());
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
    // errormsg->Error(pos_, "type is %s", (typ_->Name()).data());
    //errormsg->Error(pos_, "tenv->Look(typ_)");
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
  //errormsg->Error(pos_, "begin alloc");
  tr::Access *access = tr::Access::AllocLocal(level, escape_);
  // errormsg->Error(pos_, "do enter");
  venv->Enter(var_, new env::VarEntry(access, typ_ty));
  //errormsg->Error(pos_, "end vardec");
  tree::Exp *dst = access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  //errormsg->Error(pos_, "end dst");
  tree::Exp *src = init->exp_->UnEx();
  //errormsg->Error(pos_, "end src");

  return new tr::NxExp(new tree::MoveStm(dst, src));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
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

  //TODO: is this true?
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
  //errormsg->Error(pos_, "begin recordty");
  type::FieldList *fields = record_->MakeFieldList(tenv, errormsg);
  //errormsg->Error(pos_, "end recordty");
  
  return new type::RecordTy(fields);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //errormsg->Error(pos_, "begin arrayty");
  type::Ty *array_ty = tenv->Look(array_);
  if(!array_ty){
    //errormsg->Error(pos_, "should be defined type!");
    return type::VoidTy::Instance();
  }
  // errormsg->Error(pos_, "end arrayty");
  return new type::ArrayTy(array_ty);
}

} // namespace absyn
