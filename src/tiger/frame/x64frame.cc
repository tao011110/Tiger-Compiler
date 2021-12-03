#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override{
    printf("the offset is %d\n", offset);
    return new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, framePtr, new tree::ConstExp(offset)));
  }
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override{
    printf("framePtr is ");
    return new tree::TempExp(reg);
  }
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */

  public:
  temp::Label *name;
  std::list<frame::Access*> formals_;
  std::list<frame::Access*> locals_;
  tree::StmList *stms_;
  int offset = 0;

  X64Frame(){}

  Access *allocLocal(bool escape){
    frame::Access *access;
    if(escape){
      offset -= reg_manager->WordSize();
      access = new InFrameAccess(offset);
    }
    else{
      access = new InRegAccess(temp::TempFactory::NewTemp());
    }
    // locals_.push_back(access);

    return access;
  }

  tree::Exp *externalCall(std::string s, tree::ExpList *args){
    return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)), args);
  }
  
  tree::Exp *getFramePtr(){
    return new tree::TempExp(reg_manager->FramePointer());
  }
};
/* TODO: Put your lab5 code here */
frame::Frame *NewFrame(temp::Label *name, std::list<bool> formals){
  X64Frame *newFrame = new X64Frame();
  newFrame->name = name;
  newFrame->offset = 0;
  int count = 0;
  for(bool escape : formals){
    frame::Access* access = newFrame->allocLocal(escape);
    newFrame->formals_.push_back(access);
    std::list<temp::Temp *> argRegList = reg_manager->ArgRegs()->GetList();
    int argRegsNum = argRegList.size();
    std::list<temp::Temp *>::iterator reg = argRegList.begin();
    // View shift
    if(count < 6){
      tree::Stm *stm = new tree::MoveStm(access->ToExp(newFrame->getFramePtr()), new tree::TempExp(*(reg)));
      newFrame->stms_->Linear(stm);
      reg++;
    }
    else{
      tree::Stm *stm = new tree::MoveStm(access->ToExp(newFrame->getFramePtr()), 
      new tree::BinopExp(tree::BinOp::PLUS_OP, new tree::ConstExp((count - 5) * reg_manager->WordSize()), 
      newFrame->getFramePtr()));
      newFrame->stms_->Linear(stm);
    }
    count++;
  }

  return newFrame;
}

// I am not sure about this function!!
tree::Stm *procEntryExit1(frame::Frame *frame, tree::Stm *stm){
  std::list<tree::Stm *> stmList = frame->stms_->GetList();
  for(tree::Stm * tmp : stmList){
    stm = new tree::SeqStm(tmp, stm);
  }

  return stm;
}

assem::InstrList *procEntryExit2(assem::InstrList *body){
  body->Append(new assem::OperInstr("", new temp::TempList(), reg_manager->ReturnSink(), nullptr));

  return body;
}

//TODO: not finished
assem::Proc *procEntryExit3(frame::Frame *frame, assem::InstrList *body){
  char buf[100];
  sprintf(buf, "PROCEDURE%s\n", temp::LabelFactory::LabelString(frame->name).data());

  return new assem::Proc(std::string(buf), body, "END\n");
}
} // namespace frame