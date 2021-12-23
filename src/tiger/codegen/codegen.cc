#include "tiger/codegen/codegen.h"

#include <sstream>
#include <iostream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  fs_= frame_->name->Name() + "_framesize";
  assem::InstrList *instr_list = new assem::InstrList();
  std::list<tree::Stm *> stm_list = traces_->GetStmList()->GetList();
  std::vector<temp::Temp *> tmp_store;
  
  // save callee-saved regs
  for(auto reg: reg_manager->CalleeSaves()->GetList()){
    temp::Temp *tmp = temp::TempFactory::NewTemp();
    tmp_store.push_back(tmp);
    instr_list->Append(new assem::MoveInstr(std::string("movq `s0, `d0"), new temp::TempList(tmp), new temp::TempList(reg)));
  }

  for(auto stm : stm_list){
    stm->Munch(*instr_list, fs_);
  }

  // restore save callee-saved regs
  int i = 0;
  for(auto reg: reg_manager->CalleeSaves()->GetList()){
    instr_list->Append(new assem::MoveInstr(std::string("movq `s0, `d0"), new temp::TempList(reg), new temp::TempList(tmp_store[i])));
    i++;
  }

  assem_instr_ = std::make_unique<AssemInstr>(instr_list);
  frame::procEntryExit2(assem_instr_->GetInstrList());
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  assem::LabelInstr *instr = new assem::LabelInstr(label_->Name(), label_);
  instr_list.Append(instr);
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Label* jump_label = exp_->name_;
  std::string instr = std::string("jmp ") + temp::LabelFactory::LabelString(jump_label);
  instr_list.Append(new assem::OperInstr(instr, nullptr, nullptr, new assem::Targets(jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* left = left_->Munch(instr_list, fs);
  temp::Temp* right = right_->Munch(instr_list, fs);
  temp::TempList *src = new temp::TempList(right);
  src->Append(left);
  //compare right with left, set, then jump
  instr_list.Append(new assem::OperInstr(std::string("cmpq `s0, `s1"), nullptr, src, nullptr));

  std::string instr_str;
  switch(op_)
  {
    case tree::RelOp::EQ_OP: 
      instr_str = std::string("je"); 
      break;
    case tree::RelOp::NE_OP: 
      instr_str = std::string("jne"); 
      break;
    case tree::RelOp::LT_OP: 
      instr_str = std::string("jl"); 
      break;
    case tree::RelOp::GT_OP: 
      instr_str = std::string("jg"); 
      break;
    case tree::RelOp::LE_OP: 
      instr_str = std::string("jle"); 
      break;
    case tree::RelOp::GE_OP: 
      instr_str = std::string("jge"); 
      break;
    default:
      break;
  }
  instr_str += std::string(" `j0");
  std::vector<temp::Label *> *targets = new std::vector<temp::Label *>();
  targets->push_back(true_label_);
  instr_list.Append(new assem::OperInstr(instr_str, nullptr, nullptr, new assem::Targets(targets)));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(typeid(*dst_) == typeid(tree::TempExp)){
    // LOAD
    temp::TempList *src = new temp::TempList(src_->Munch(instr_list, fs));
    temp::TempList *dst = new temp::TempList(dst_->Munch(instr_list, fs));
    instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"), dst, src));
  }
  else{
    // STORE
    temp::TempList *src = new temp::TempList(src_->Munch(instr_list, fs));
    // use the (tree::MemExp*) to make the dst_ a MemExp
    src->Append(((tree::MemExp*)dst_)->exp_->Munch(instr_list, fs));
    instr_list.Append(new assem::OperInstr(std::string("movq `s0, (`s1)"), nullptr, src, nullptr));
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list, fs); 
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* reg = temp::TempFactory::NewTemp();
  temp::Temp* left = left_->Munch(instr_list, fs);
  temp::Temp* right = right_->Munch(instr_list, fs);
  temp::Temp *rdx = reg_manager->Registers()->NthTemp(3);
  temp::Temp *rax = reg_manager->ReturnValue();
  switch(op_)
  {
    case tree::PLUS_OP:
    {
      temp::TempList *dst = new temp::TempList(reg);
      instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"), dst, new temp::TempList(left)));
      temp::TempList *src = new temp::TempList(right);
      src->Append(reg);
      instr_list.Append(new assem::OperInstr(std::string("addq `s0, `d0"), dst, src, nullptr));
      break;
    }
    case tree::MINUS_OP:
    {
      temp::TempList *dst = new temp::TempList(reg);
      instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"), dst, new temp::TempList(left)));
      temp::TempList *src = new temp::TempList(right);
      src->Append(reg);
      instr_list.Append(new assem::OperInstr(std::string("subq `s0, `d0"), dst, src, nullptr));
      break;
    }
    case tree::MUL_OP:
    {
      instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"), new temp::TempList(rax), new temp::TempList(left)));
      temp::TempList *dst = new temp::TempList(rax);
      dst->Append(rdx);
      // be careful with cqto here 
      instr_list.Append(new assem::OperInstr(std::string("cqto"), dst, new temp::TempList(rax), nullptr));
      temp::TempList *src = new temp::TempList(right);
      src->Append(rax);
      src->Append(rdx);
      // be careful with imulq here 
      instr_list.Append(new assem::OperInstr(std::string("imulq `s0"), dst, src, nullptr));
      instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"), new temp::TempList(reg),new temp::TempList(rax)));
      break;
    }
    case tree::DIV_OP:
    {
      instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"),new temp::TempList(rax),new temp::TempList(left)));
      temp::TempList *dst = new temp::TempList(rax);
      dst->Append(rdx);
      // be careful with cqto here 
      instr_list.Append(new assem::OperInstr(std::string("cqto"), dst, new temp::TempList(rax), nullptr));
      temp::TempList *src = new temp::TempList(right);
      src->Append(rax);
      src->Append(rdx);
      instr_list.Append(new assem::OperInstr(std::string("idivq `s0"), dst, src, nullptr));
      instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"),new temp::TempList(reg),new temp::TempList(rax)));
      break;
    }
    default:
      break;
  }
  return reg;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *reg = temp::TempFactory::NewTemp();
  temp::TempList *dst = new temp::TempList(reg);
  temp::Temp *exp = exp_->Munch(instr_list, fs);
  temp::TempList *src = new temp::TempList(exp);
  instr_list.Append(new assem::OperInstr("movq (`s0), `d0", dst, src, nullptr));
  return reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //notice the rsp
  if(temp_==reg_manager->FramePointer())
  {
    temp::Temp *reg = temp::TempFactory::NewTemp();
    std::string instr_str= std::string("leaq ") + std::string(fs) + std::string("(`s0), `d0");
    instr_list.Append(new assem::OperInstr(instr_str, new temp::TempList(reg), new temp::TempList(reg_manager->StackPointer()), nullptr));
    return reg;
  }
  else{
    return temp_;
  }
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *address = temp::TempFactory::NewTemp();
  std::string instr_str = std::string("leaq ") + name_->Name() + std::string("(%rip), `d0");
  instr_list.Append(new assem::OperInstr(instr_str, new temp::TempList(address), nullptr, nullptr));

  return address;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp  *reg = temp::TempFactory::NewTemp();
  std::string instr_str= std::string("movq $") + std::to_string(consti_)+ std::string(", `d0");
  instr_list.Append(new assem::OperInstr(instr_str, new temp::TempList(reg), nullptr, nullptr));

  return reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *ret = temp::TempFactory::NewTemp();
  temp::TempList *alloced_args = args_->MunchArgs(instr_list, fs);
  std::string instr_str = std::string("call ") + ((tree::NameExp*)fun_)->name_->Name();
  int args_num = alloced_args->GetList().size();
  int reg_num = reg_manager->ArgRegs()->GetList().size();

  // //save caller-saved regs
  // std::vector<temp::Temp *> tmp_store;
  // for(auto reg: reg_manager->CallerSaves()->GetList()){
  //   temp::Temp *tmp = temp::TempFactory::NewTemp();
  //   tmp_store.push_back(tmp);
  //   instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"), new temp::TempList(tmp), new temp::TempList(reg)));
  // }

  instr_list.Append(new assem::OperInstr(instr_str, reg_manager->CallerSaves(), alloced_args, nullptr));

  // pop the args on the stack
  if(reg_num < args_num){
    std::string instr = std::string("addq $") + std::to_string((args_num - reg_num) * reg_manager->WordSize()) + std::string(", `d0");
    instr_list.Append(new assem::OperInstr(instr, new temp::TempList(reg_manager->StackPointer()), nullptr, nullptr));
  }

  // parse the return value
  instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"), 
    new temp::TempList(ret), new temp::TempList(reg_manager->ReturnValue())));

  // // restore save caller-saved regs
  // int i = 0;
  // for(auto reg: reg_manager->CallerSaves()->GetList()){
  //   instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"), new temp::TempList(reg), new temp::TempList(tmp_store[i])));
  //   i++;
  // }

  return ret;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::list<tree::Exp *> args_list = GetList();
  std::list<temp::Temp *> regs = reg_manager->ArgRegs()->GetList();
  temp::TempList *alloced_args = new temp::TempList();

  int regs_num = regs.size();
  int count = 0;

  for(auto arg:args_list)
  {
    temp::Temp *tmp = arg->Munch(instr_list, fs);
    if(count < regs_num){
      instr_list.Append(new assem::MoveInstr(std::string("movq `s0, `d0"), 
        new temp::TempList(reg_manager->ArgRegs()->NthTemp(count)), new temp::TempList(tmp)));
    }
    else{
      instr_list.Append(new assem::OperInstr(std::string("subq $8, %rsp"), 
        new temp::TempList(reg_manager->StackPointer()), nullptr, nullptr));
      instr_list.Append(new assem::OperInstr(std::string("movq `s0, (%rsp)"),
        new temp::TempList(reg_manager->StackPointer()), new temp::TempList(tmp), nullptr));
    }
    count++;
    // whether in the reg or in the frame
    alloced_args->Append(tmp);
  }
  return alloced_args;
}

} // namespace tree
