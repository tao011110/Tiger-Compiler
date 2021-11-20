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
  private:
    X64RegManager x64RegManager;

  public:
  temp::Label *name;
  std::list<frame::Access*> formals_;
  std::list<frame::Access*> locals_;
  tree::SeqStm *stms_;
  int offset = 0;

  Access *allocLocal(bool escape){
    frame::Access *access;
    if(escape){
      access = new InFrameAccess(offset);
      offset--;
    }
    else{
      access = new InRegAccess(temp::TempFactory::NewTemp());
    }
    locals_.push_back(access);

    return access;
  }

  tree::Exp *externalCall(std::string s, tree::ExpList *args){
    return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)), args);
  }
  
  tree::Exp *getFramePtr(){
    return new tree::TempExp(x64RegManager.FramePointer());
  }
};
/* TODO: Put your lab5 code here */
frame::Frame NewFrame(temp::Label *name, std::list<bool> formals, frame::RegManager *reg_manager){

}
tree::Stm *procEntryExit1(frame::Frame frame, tree::Stm *stm){
  
}
} // namespace frame