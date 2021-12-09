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
    printf("framePtr is ");
    return new tree::TempExp(reg);
  }
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
  public:
  X64Frame(){
    stms_ = new tree::StmList();
    offset -= reg_manager->WordSize();
  }

  virtual ~X64Frame(){
    printf("\nwe delete frame\n");
    delete stms_;
  }

  Access *allocLocal(bool escape){
    frame::Access *access;
    if(escape){
      printf("escape!!!");
      offset -= reg_manager->WordSize();
      access = new InFrameAccess(offset);
      printf("now offset is %d", offset);
    }
    else{
      access = new InRegAccess(temp::TempFactory::NewTemp());
    }
    // locals_.push_back(access);

    return access;
  }

  tree::Exp *externalCall(std::string s, tree::ExpList *args){
    printf("external %s\n", s.data());
    return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)), args);
  }
  
  tree::Exp *getFramePtr(){
    return new tree::TempExp(reg_manager->FramePointer());
  }

  std::string GetLabel(){
    return name->Name();
  }
};


/* TODO: Put your lab5 code here */
frame::Frame *NewFrame(temp::Label *name, std::list<bool> formals){
  printf("\nnew frame name is %s\n", name->Name().data());
  frame::X64Frame *newFrame = new frame::X64Frame();
  newFrame->name = name;
  newFrame->offset = -reg_manager->WordSize();
  int count = 0;
  std::list<temp::Temp *> argRegList = reg_manager->ArgRegs()->GetList();
  int argRegsNum = argRegList.size();
  std::list<temp::Temp *>::iterator reg = argRegList.begin();
  for(bool escape : formals){
    frame::Access* access = newFrame->allocLocal(escape);
    newFrame->formals_.push_back(access);
    // View shift
    if(count < argRegsNum){
      tree::Stm *stm = new tree::MoveStm(access->ToExp(newFrame->getFramePtr()), new tree::TempExp(*(reg)));
      assert(stm != nullptr);
      newFrame->stms_->Linear(stm);
      reg++;
    }
    else{
      tree::Stm *stm = new tree::MoveStm(access->ToExp(newFrame->getFramePtr()), 
        new tree::BinopExp(tree::BinOp::PLUS_OP, newFrame->getFramePtr(), 
          new tree::ConstExp((count - 5) * reg_manager->WordSize())));
      newFrame->stms_->Linear(stm);
    }
    count++;
  }
  printf("\nnew frame offset is %d\n", newFrame->offset);

  return newFrame;
}

// I am not sure about this function!!
tree::Stm *procEntryExit1(frame::Frame *frame, tree::Stm *stm){
  printf("\n begin procEntryExit1\n");
  std::list<tree::Stm *> stmList = frame->stms_->GetList();
  for(tree::Stm * tmp : stmList){
    printf("parse stm");
    if(!stm){
      stm = new tree::SeqStm(tmp, stm);
    }
    else{
      stm = tmp;
    }
  }
  printf("end procEntryExit1\n");

  return stm;
}

assem::InstrList *procEntryExit2(assem::InstrList *body){
  body->Append(new assem::OperInstr("", new temp::TempList(), reg_manager->ReturnSink(), nullptr));

  return body;
}

//TODO: not finished
assem::Proc *procEntryExit3(frame::Frame *frame, assem::InstrList *body){
  std::string prolog = frame->name->Name() + std::string(":\n");
  std::string instr = std::string(".set ") + frame->name->Name() + std::string("_framesize, ") 
    + std::to_string(-frame->offset) + std::string("\n");
  prolog += instr;
  // instr = std::string("subq $") + frame->name->Name() + std::string("_framesize, %rsp\n");
  instr = std::string("subq $") + std::to_string(-frame->offset) + std::string(", %rsp\n");
  prolog += instr;

  // std::string epilog = std::string("addq $") + frame->name->Name() + std::string("_framesize, %rsp\n");
  std::string epilog = std::string("addq $") + std::to_string(-frame->offset) + std::string(", %rsp\n");
  epilog += std::string("retq\n");

  return new assem::Proc(prolog, body, epilog);
}
} // namespace frame