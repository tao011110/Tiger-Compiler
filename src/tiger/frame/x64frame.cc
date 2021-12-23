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
    // printf("the offset is %d\n", offset);
    tree::Exp * binopExp = new tree::BinopExp(tree::BinOp::PLUS_OP, framePtr, new tree::ConstExp(offset));
    // printf("end binopExp\n");
    tree::Exp *exp = new tree::MemExp(binopExp);
    // printf("end toExp\n");
    return exp;
  }
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override{
    // printf("framePtr is ");
    return new tree::TempExp(reg);
  }
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
  private:
  int reg_alloced = 1;

  public:
  X64Frame(){
    stms_ = new tree::StmList();
    offset -= reg_manager->WordSize();
  }

  virtual ~X64Frame(){
    // printf("\nwe delete frame\n");
    delete stms_;
  }

  Access *allocLocal(bool escape){
    frame::Access *access;
    int reg_num = reg_manager->ArgRegs()->GetList().size();
    if(!escape && reg_alloced < reg_num){
      access = new InRegAccess(temp::TempFactory::NewTemp());
      reg_alloced++;
    }
    else{
      // printf("now alloc offset is %d", offset);
      access = new InFrameAccess(offset);
      offset -= reg_manager->WordSize();
    }
    // locals_.push_back(access);

    return access;
  }

  
  tree::Exp *getFramePtr(){
    return new tree::TempExp(reg_manager->FramePointer());
  }

  std::string GetLabel(){
    return name->Name();
  }
};

/* TODO: Put your lab5 code here */
frame::Frame *newFrame(temp::Label *name, std::list<bool> formals){
  // printf("\nnew frame name is %s\n", name->Name().data());
  frame::X64Frame *newFrame = new frame::X64Frame();
  newFrame->name = name;
  newFrame->offset = -reg_manager->WordSize();
  std::list<temp::Temp *> argRegList = reg_manager->ArgRegs()->GetList();
  int argRegsNum = argRegList.size();
  std::list<temp::Temp *>::iterator reg = argRegList.begin();
  for(bool escape : formals){
    frame::Access* access = newFrame->allocLocal(escape);
    newFrame->formals_.push_back(access);
  }
  // printf("\n new frame offset is %d\n", newFrame->offset);

  return newFrame;
}

tree::Stm *procEntryExit1(frame::Frame *frame, tree::Stm *stm){
  // printf("\n begin procEntryExit1\n");
  int reg_num = reg_manager->ArgRegs()->GetList().size();
  int count = 0;
  tree::Stm* exp=new tree::ExpStm(new tree::ConstExp(0));
  std::list<temp::Temp *> regs= reg_manager->ArgRegs()->GetList();
  std::list<temp::Temp *>::iterator reg = regs.begin();
  for(auto formal:frame->formals_)
  {
    if(count < reg_num){
      // printf("\n in the reg\n");
      exp = new tree::SeqStm(exp, new tree::MoveStm(
        formal->ToExp(new tree::TempExp(reg_manager->FramePointer())),
          new tree::TempExp(*reg)));
      reg++;
    }
    else{
      // printf("\n in the frame\n");
      /* + reg_manager->WordSize() at the last is because of the rbp + 8 is the return address, 
          while -1 is because of the count calc
      */
      int view_offset=(frame->formals_.size() - 1 - count) * reg_manager->WordSize() + reg_manager->WordSize();
      exp = new tree::SeqStm(exp, new tree::MoveStm(
        formal->ToExp(new tree::TempExp(reg_manager->FramePointer())),
          new tree::MemExp(new tree::BinopExp(tree::PLUS_OP,
            new tree::TempExp(reg_manager->FramePointer()),new tree::ConstExp(view_offset)))));
    }
    count++;
  }

  return new tree::SeqStm(exp, stm);
}

assem::InstrList *procEntryExit2(assem::InstrList *body){
  body->Append(new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr));

  return body;
}

assem::Proc *procEntryExit3(frame::Frame *frame, assem::InstrList *body){
  std::string prolog = frame->name->Name() + std::string(":\n");
  std::string instr = std::string(".set ") + frame->name->Name() + std::string("_framesize, ") 
    + std::to_string(-frame->offset) + std::string("\n");
  prolog += instr;
  instr = std::string("subq $") + std::to_string(-frame->offset) + std::string(", %rsp\n");
  prolog += instr;

  std::string epilog = std::string("addq $") + std::to_string(-frame->offset) + std::string(", %rsp\n");
  epilog += std::string("retq\n");

  return new assem::Proc(prolog, body, epilog);
}

tree::Exp *externalCall(std::string s, tree::ExpList *args){
  // printf("external %s\n", s.data());
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)), args);
}

} // namespace frame
